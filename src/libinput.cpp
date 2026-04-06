/** @file libinput.cpp
 *  @brief Scene update and input-processing implementations.
 *
 *  Each scene has a private static handler; `update_scene()` dispatches to
 *  the correct one.  No raylib functions are used here — all input arrives
 *  pre-packaged in an `InputState`.
 */

#include "libinput.h"
#include "events.h"
#include "gamedata.h"
#include "mprint.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Chance each day that an active job search produces an offer.
static constexpr int JOB_SEARCH_OFFER_CHANCE_PERCENT = 20;
/// Starter hourly wage for the first grocery-store job offer.
static constexpr int GROCERY_CLERK_HOURLY_WAGE_CENTS = 850;
/// Length of a simple part-time shift in hours.
static constexpr int PART_TIME_SHIFT_HOURS = 4;
/// Purchase price for the first used home computer.
static constexpr int STARTER_COMPUTER_COST_CENTS = 30000;
/// Hidden experience required for each computer-oriented level.
static constexpr int COMPUTER_SKILL_XP_PER_LEVEL = 5;

static void queue_computer_use_preference_prompt(GameState& gs);
static int computer_use_preference_choice_index(const GameState& gs);

/**
 * @brief Queue a one-shot UI sound effect for the renderer/audio layer.
 * @param gs   Mutable game state receiving the event.
 * @param sfx  Requested UI sound effect.
 */
static void queue_ui_sfx(GameState& gs, UiSfx sfx) {
    gs.ui_sfx = sfx;
    gs.ui_sfx_event_id++;
}

/**
 * @brief Enqueue a gameplay decision popup.
 * @param gs Mutable game state receiving the decision.
 * @param ev Decision to queue.
 */
static void queue_decision(GameState& gs, const DecisionEvent& ev) {
    gs.decision_queue.push_back(ev);
}

/**
 * @brief Convert the player's current cash to whole cents.
 * @param gs Current game state.
 * @return Rounded whole-cent cash value.
 */
static int player_cash_cents(const GameState& gs) {
    return static_cast<int>(gs.player.cash * 100.0f + 0.5f);
}

/**
 * @brief Return the level-up threshold for a specific skill.
 * @param id Skill identifier.
 * @return Hidden experience required for one level.
 */
static int skill_xp_per_level(SkillId id) {
    switch (id) {
    case SkillId::SOCIALIZING:
        return 1;
    case SkillId::COMPUTER_BASICS:
    case SkillId::HACKING:
    case SkillId::GAMING:
    case SkillId::SOCIAL_MEDIA:
    case SkillId::PROGRAMMING:
        return COMPUTER_SKILL_XP_PER_LEVEL;
    }
    return 1;
}

/**
 * @brief Unlock an achievement once and enqueue its gameplay popup.
 * @param gs Mutable game state.
 * @param id Achievement to unlock.
 * @return True when the achievement was newly unlocked this call.
 */
static bool unlock_achievement(GameState& gs, AchievementId id) {
    const int index = achievement_index(id);
    if (gs.achievements_unlocked[index]) return false;

    gs.achievements_unlocked[index] = true;
    gs.achievement_popup_queue.push_back(id);
    return true;
}

/**
 * @brief Add hidden experience to one skill and apply any resulting level-ups.
 * @param gs Mutable game state.
 * @param id Skill identifier receiving progress.
 * @param xp Amount of hidden experience to add.
 */
static void add_skill_xp(GameState& gs, SkillId id, int xp) {
    if (xp <= 0) return;

    SkillProgress& skill = gs.player.skills[skill_index(id)];
    skill.xp += xp;
    const int xp_per_level = skill_xp_per_level(id);

    while (skill.xp >= xp_per_level) {
        skill.xp -= xp_per_level;
        skill.level++;
    }
}

/**
 * @brief Set the persistent home-computer use preference.
 * @param gs Mutable game state.
 * @param pref New preferred computer activity.
 */
static void set_computer_use_preference(GameState& gs, ComputerUsePreference pref) {
    gs.access.computer_use_preference = pref;
}

/**
 * @brief Apply the consequence of a confirmed decision choice.
 * @param gs Mutable game state.
 * @param effect Effect selected by the player.
 */
static void apply_decision_effect(GameState& gs, DecisionEffect effect) {
    switch (effect) {
    case DecisionEffect::START_JOB_SEARCH:
        gs.employment.searching_for_job = true;
        gs.employment.last_job_search_roll_day =
            static_cast<int>(gs.elapsed_game_hours / 24LL) - 1;
        break;
    case DecisionEffect::DECLINE_JOB_SEARCH:
    case DecisionEffect::DECLINE_GROCERY_STORE_JOB:
    case DecisionEffect::DECLINE_STARTER_COMPUTER:
        break;
    case DecisionEffect::ACCEPT_GROCERY_STORE_JOB:
        gs.employment.searching_for_job = false;
        gs.employment.employed = true;
        gs.employment.current_job = JobId::GROCERY_STORE_CLERK;
        gs.employment.hourly_wage_cents = GROCERY_CLERK_HOURLY_WAGE_CENTS;
        gs.employment.shifts_worked_this_week = 0;
        gs.employment.missed_shifts_this_week = 0;
        gs.employment.last_paycheck_cents = 0;
        break;
    case DecisionEffect::BUY_STARTER_COMPUTER:
        gs.player.cash -= static_cast<float>(STARTER_COMPUTER_COST_CENTS) / 100.0f;
        if (gs.player.cash < 0.0f) gs.player.cash = 0.0f;
        gs.access.owns_computer = true;
        queue_computer_use_preference_prompt(gs);
        break;
    case DecisionEffect::SET_COMPUTER_USE_PRACTICE:
        set_computer_use_preference(gs, ComputerUsePreference::PRACTICE);
        break;
    case DecisionEffect::SET_COMPUTER_USE_HACKING:
        set_computer_use_preference(gs, ComputerUsePreference::HACKING);
        break;
    case DecisionEffect::SET_COMPUTER_USE_GAMING:
        set_computer_use_preference(gs, ComputerUsePreference::GAMING);
        break;
    case DecisionEffect::SET_COMPUTER_USE_SOCIAL_MEDIA:
        set_computer_use_preference(gs, ComputerUsePreference::SOCIAL_MEDIA);
        break;
    case DecisionEffect::SET_COMPUTER_USE_PROGRAMMING:
        set_computer_use_preference(gs, ComputerUsePreference::PROGRAMMING);
        break;
    case DecisionEffect::NONE:
        break;
    }

    if (effect == DecisionEffect::BUY_STARTER_COMPUTER
        || effect == DecisionEffect::DECLINE_STARTER_COMPUTER) {
        gs.access.last_computer_offer_cash_cents = player_cash_cents(gs);
    }
}

/**
 * @brief Adjust a volume percentage in 10-point steps, clamped to [0, 100].
 * @param volume Mutable volume percentage.
 * @param in     Input snapshot for this frame.
 * @return True when the value changed.
 */
static bool adjust_volume(int& volume, const InputState& in) {
    int prev = volume;
    if (in.left_repeat && volume > 0)   volume -= 10;
    if (in.right_repeat && volume < 100) volume += 10;
    return volume != prev;
}

/**
 * @brief Move a menu selection up or down, wrapping at the ends.
 * @param selection  Current selection index (mutated in place).
 * @param count      Total number of options.
 * @param in         Input snapshot for this frame.
 * @return True when the selection changed.
 */
static bool navigate_menu(int& selection, int count, const InputState& in) {
    bool changed = false;
    if (in.up_pressed) {
        selection = (selection - 1 + count) % count;
        changed = true;
    }
    if (in.down_pressed) {
        selection = (selection + 1) % count;
        changed = true;
    }
    return changed;
}

/**
 * @brief Transition to a new scene, saving the current one for back navigation.
 *        Resets menu_selection to 0.
 */
static void transition(GameState& gs, Scene next) {
    minfo("scene transition: %d -> %d", (int)gs.current_scene, (int)next);
    gs.previous_scene = gs.current_scene;
    gs.current_scene  = next;
    gs.menu_selection = 0;
}

/**
 * @brief Reset gameplay time progression to the character's birth.
 * @param gs Mutable game state.
 */
static void reset_gameplay_clock(GameState& gs) {
    gs.gameplay_paused = false;
    gs.gameplay_time_accumulator = 0.0f;
    gs.elapsed_game_hours = 0;
    gs.play_session_seconds = 0.0f;
    gs.last_processed_game_day = -1;
    gs.achievement_popup_visible = false;
    gs.achievement_popup_seconds_remaining = 0.0f;
    gs.next_news_event_idx = 0;
    gs.news_feed.clear();
    gs.decision_queue.clear();
    gs.decision_popup_visible = false;
    gs.active_decision = DecisionEvent{};
    gs.decision_selection = 0;
    gs.age_16_job_prompt_shown = false;
    gs.access = AccessState{};
    gs.wellbeing = WellbeingState{};
    gs.employment = EmploymentState{};
    gs.player.cash = 0.0f;
    gs.player.skills = {};
}

/**
 * @brief Advance the news-event feed up to the current in-game date.
 *
 * Iterates `ALL_EVENTS` from `next_news_event_idx` forward, appending any
 * newly reached events to `news_feed`.  The feed is capped at 30 entries
 * (oldest entry dropped when full).  Stops at the first event whose date has
 * not yet been reached.
 *
 * @param gs Mutable game state.
 */
static void update_news_events(GameState& gs) {
    const GameDateTime dt = gameplay_datetime(gs);
    while (gs.next_news_event_idx < NEWS_EVENT_COUNT) {
        const NewsEvent& ev = ALL_EVENTS[gs.next_news_event_idx];
        if (!should_trigger_news_event(ev, dt)) break;
        gs.news_feed.push_back(gs.next_news_event_idx);
        if (static_cast<int>(gs.news_feed.size()) > 30)
            gs.news_feed.erase(gs.news_feed.begin());
        gs.next_news_event_idx++;
    }
}

/**
 * @brief Clamp the custom-character day selection to the selected month/year.
 * @param gs Mutable game state.
 */
static void clamp_custom_birth_day(GameState& gs) {
    const int selected_year = YEAR_MIN + gs.year_selection;
    const int max_day = days_in_month(selected_year, gs.month_selection);
    if (gs.day_selection > max_day) gs.day_selection = max_day;
    if (gs.day_selection < 1) gs.day_selection = 1;
}

/**
 * @brief Evaluate achievement rules that should fire when a new life starts.
 * @param gs Mutable game state.
 */
static void award_starting_achievements(GameState& gs) {
    unlock_achievement(gs, AchievementId::BORN);
    if (gs.player.year_of_birth < 1970)
        unlock_achievement(gs, AchievementId::BORN_BEFORE_UNIX_EPOCH);
    if (gs.player.year_of_birth < 2000)
        unlock_achievement(gs, AchievementId::BORN_BEFORE_Y2K);
    if (gs.player.year_of_birth < 2038)
        unlock_achievement(gs, AchievementId::BORN_BEFORE_2038);
}

/**
 * @brief Evaluate gameplay-driven achievement rules tied to calendar progression.
 * @param gs Mutable game state.
 */
static void update_gameplay_achievements(GameState& gs) {
    const GameDateTime dt = gameplay_datetime(gs);
    if (gs.player.year_of_birth < 1970 && dt.year >= 1970)
        unlock_achievement(gs, AchievementId::SURVIVED_UNIX_EPOCH);
    if (gs.player.year_of_birth < 2000 && dt.year >= 2000)
        unlock_achievement(gs, AchievementId::SURVIVED_Y2K);
    if (gs.player.year_of_birth < 2038 && dt.year >= 2038)
        unlock_achievement(gs, AchievementId::SURVIVED_2038);
}

/**
 * @brief Advance the gameplay achievement popup queue and active modal timer.
 * @param gs Mutable game state.
 * @param in Input snapshot containing `frame_seconds`.
 */
static void update_achievement_popup(GameState& gs, const InputState& in) {
    if (gs.achievement_popup_visible) {
        if (in.frame_seconds > 0.0f)
            gs.achievement_popup_seconds_remaining -= in.frame_seconds;
        if (gs.achievement_popup_seconds_remaining <= 0.0f)
            gs.achievement_popup_visible = false;
    }

    if (!gs.achievement_popup_visible && !gs.achievement_popup_queue.empty()) {
        gs.active_achievement_popup = gs.achievement_popup_queue.front();
        gs.achievement_popup_queue.erase(gs.achievement_popup_queue.begin());
        gs.achievement_popup_visible = true;
        gs.achievement_popup_seconds_remaining = ACHIEVEMENT_POPUP_DURATION;
    }
}

/**
 * @brief Promote the next queued decision to the active popup slot.
 * @param gs Mutable game state.
 */
static void update_decision_popup(GameState& gs) {
    if (gs.decision_popup_visible || gs.decision_queue.empty()) return;

    gs.active_decision = gs.decision_queue.front();
    gs.decision_queue.erase(gs.decision_queue.begin());
    gs.decision_popup_visible = true;
    gs.decision_selection =
        (gs.active_decision.title == "Computer Use")
        ? computer_use_preference_choice_index(gs)
        : 0;
}

/**
 * @brief Return whether a queued or active decision already has the supplied title.
 * @param gs Current game state.
 * @param title Decision title to search for.
 * @return True when the decision is already active or queued.
 */
static bool has_pending_decision_title(const GameState& gs, const char* title) {
    if (gs.decision_popup_visible && gs.active_decision.title == title) return true;
    for (const DecisionEvent& ev : gs.decision_queue) {
        if (ev.title == title) return true;
    }
    return false;
}

/**
 * @brief Return the option index matching the current computer-use preference.
 * @param gs Current game state.
 * @return Choice index inside the computer-use preference modal.
 */
static int computer_use_preference_choice_index(const GameState& gs) {
    switch (gs.access.computer_use_preference) {
    case ComputerUsePreference::NONE:         return 0;
    case ComputerUsePreference::PRACTICE:     return 0;
    case ComputerUsePreference::HACKING:      return 1;
    case ComputerUsePreference::GAMING:       return 2;
    case ComputerUsePreference::SOCIAL_MEDIA: return 3;
    case ComputerUsePreference::PROGRAMMING:  return 4;
    }
    return 0;
}

/**
 * @brief Return the age-16 part-time job prompt definition.
 * @return Decision modal offering to start job searching.
 */
static DecisionEvent make_age_16_job_prompt() {
    DecisionEvent ev;
    ev.title = "Part-Time Work";
    ev.prompt =
        "You are 16 now. You can start looking for a part-time job.\n"
        "Do you want to begin searching?";
    ev.choices.push_back(DecisionChoice{"Yes", DecisionEffect::START_JOB_SEARCH});
    ev.choices.push_back(DecisionChoice{"No", DecisionEffect::DECLINE_JOB_SEARCH});
    return ev;
}

/**
 * @brief Return the first grocery-store job offer definition.
 * @return Decision modal offering entry-level grocery clerk work.
 */
static DecisionEvent make_grocery_job_offer() {
    DecisionEvent ev;
    ev.title = "Job Offer";
    ev.prompt =
        "A local grocery store needs a part-time clerk.\n"
        "They offer simple work, steady hours, and $8.50 per hour.\n"
        "Do you accept the position?";
    ev.choices.push_back(DecisionChoice{"Accept", DecisionEffect::ACCEPT_GROCERY_STORE_JOB});
    ev.choices.push_back(DecisionChoice{"Decline", DecisionEffect::DECLINE_GROCERY_STORE_JOB});
    return ev;
}

/**
 * @brief Return the first home-computer purchase opportunity definition.
 * @return Decision modal offering a used starter computer.
 */
static DecisionEvent make_starter_computer_offer() {
    DecisionEvent ev;
    ev.title = "Used Computer";
    ev.prompt =
        "You finally have enough cash for a used home computer.\n"
        "It is not powerful, but it is yours, and it costs $300.00.\n"
        "Do you buy it?";
    ev.choices.push_back(DecisionChoice{"Buy", DecisionEffect::BUY_STARTER_COMPUTER});
    ev.choices.push_back(DecisionChoice{"Not Yet", DecisionEffect::DECLINE_STARTER_COMPUTER});
    return ev;
}

/**
 * @brief Return the persistent computer-use preference chooser definition.
 * @return Decision modal for selecting the default way the player uses a computer.
 */
static DecisionEvent make_computer_use_preference_prompt() {
    DecisionEvent ev;
    ev.title = "Computer Use";
    ev.prompt =
        "How do you mainly want to use your computer?\n"
        "This becomes your default daily focus until you change it.";
    ev.choices.push_back(DecisionChoice{"Practice", DecisionEffect::SET_COMPUTER_USE_PRACTICE});
    ev.choices.push_back(DecisionChoice{"Hacking", DecisionEffect::SET_COMPUTER_USE_HACKING});
    ev.choices.push_back(DecisionChoice{"Gaming", DecisionEffect::SET_COMPUTER_USE_GAMING});
    ev.choices.push_back(DecisionChoice{"Social Media", DecisionEffect::SET_COMPUTER_USE_SOCIAL_MEDIA});
    ev.choices.push_back(DecisionChoice{"Programming", DecisionEffect::SET_COMPUTER_USE_PROGRAMMING});
    return ev;
}

/**
 * @brief Queue the age-16 job-search decision the first time it becomes valid.
 * @param gs Mutable game state.
 */
static void queue_age_16_job_prompt_if_needed(GameState& gs) {
    if (gs.age_16_job_prompt_shown) return;
    if (gameplay_age_years(gs) < 16) return;

    queue_decision(gs, make_age_16_job_prompt());
    gs.age_16_job_prompt_shown = true;
}

/**
 * @brief Handle navigation and confirmation inside the active decision popup.
 * @param gs Mutable game state.
 * @param in Input snapshot for this frame.
 */
static void handle_active_decision(GameState& gs, const InputState& in) {
    const int choice_count = static_cast<int>(gs.active_decision.choices.size());
    if (choice_count <= 0) {
        gs.decision_popup_visible = false;
        return;
    }

    if (navigate_menu(gs.decision_selection, choice_count, in))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);

    if (in.enter_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        apply_decision_effect(gs, gs.active_decision.choices[gs.decision_selection].effect);
        gs.decision_popup_visible = false;
        gs.active_decision = DecisionEvent{};
        gs.decision_selection = 0;
        update_decision_popup(gs);
    }
}

/**
 * @brief Roll once per in-game day for a first job offer while unemployed.
 * @param gs Mutable game state.
 */
static void update_job_search(GameState& gs) {
    if (!gs.employment.searching_for_job) return;
    if (gs.employment.employed) return;
    if (gs.decision_popup_visible) return;

    const int current_day = gameplay_day_index(gs);
    if (current_day <= gs.employment.last_job_search_roll_day) return;

    gs.employment.last_job_search_roll_day = current_day;
    if ((rand() % 100) < JOB_SEARCH_OFFER_CHANCE_PERCENT)
        queue_decision(gs, make_grocery_job_offer());
}

/**
 * @brief Queue a starter-computer purchase offer once the player can newly afford one.
 * @param gs Mutable game state.
 */
static void update_computer_purchase_offer(GameState& gs) {
    if (gs.access.owns_computer) return;
    if (gs.decision_popup_visible) return;
    if (has_pending_decision_title(gs, "Used Computer")) return;

    const int cash_cents = player_cash_cents(gs);
    if (cash_cents < STARTER_COMPUTER_COST_CENTS) return;
    if (cash_cents <= gs.access.last_computer_offer_cash_cents) return;

    queue_decision(gs, make_starter_computer_offer());
}

/**
 * @brief Queue the computer-use preference chooser when the player needs or requests it.
 * @param gs Mutable game state.
 */
static void queue_computer_use_preference_prompt(GameState& gs) {
    if (has_pending_decision_title(gs, "Computer Use")) return;
    queue_decision(gs, make_computer_use_preference_prompt());
}

/**
 * @brief Return the current percent chance that the player successfully works a shift.
 * @param gs Current game state.
 * @return Attendance chance clamped to [0, 100].
 */
static int work_attendance_chance_percent(const GameState& gs) {
    int chance = (gs.wellbeing.mental_health + gs.wellbeing.physical_health) / 2;
    if (chance < 0) chance = 0;
    if (chance > 100) chance = 100;
    return chance;
}

/**
 * @brief Process one employed day for shift accumulation and weekly payday rules.
 * @param gs Mutable game state.
 * @param day_index Zero-based in-game day index to process.
 */
static void process_employment_day(GameState& gs, int day_index) {
    if (!gs.employment.employed) return;

    const long long day_hours = static_cast<long long>(day_index) * 24LL;
    const GameDateTime dt = game_datetime_from_hours(gs.player.year_of_birth,
                                                     gs.player.birth_month,
                                                     gs.player.birth_day,
                                                     day_hours);
    const int weekday = day_of_week(dt.year, dt.month, dt.day);
    const bool is_weekday = (weekday >= 1 && weekday <= 5);
    const bool is_friday = (weekday == 5);

    if (is_weekday) {
        const int attendance_chance = work_attendance_chance_percent(gs);
        if ((rand() % 100) < attendance_chance) {
            gs.employment.shifts_worked_this_week++;
            add_skill_xp(gs, SkillId::SOCIALIZING, 1);
        } else {
            gs.employment.missed_shifts_this_week++;
        }
    }

    if (is_friday) {
        const int paycheck_cents =
            gs.employment.shifts_worked_this_week *
            PART_TIME_SHIFT_HOURS *
            gs.employment.hourly_wage_cents;
        gs.employment.last_paycheck_cents = paycheck_cents;
        if (paycheck_cents > 0)
            gs.player.cash += static_cast<float>(paycheck_cents) / 100.0f;
        gs.employment.shifts_worked_this_week = 0;
        gs.employment.missed_shifts_this_week = 0;
    }
}

/**
 * @brief Process daily computer practice for players who own a computer.
 * @param gs Mutable game state.
 */
static void process_computer_use_day(GameState& gs) {
    if (!gs.access.owns_computer) return;
    switch (gs.access.computer_use_preference) {
    case ComputerUsePreference::NONE:
        break;
    case ComputerUsePreference::PRACTICE:
        add_skill_xp(gs, SkillId::COMPUTER_BASICS, 1);
        break;
    case ComputerUsePreference::HACKING:
        add_skill_xp(gs, SkillId::HACKING, 1);
        break;
    case ComputerUsePreference::GAMING:
        add_skill_xp(gs, SkillId::GAMING, 1);
        break;
    case ComputerUsePreference::SOCIAL_MEDIA:
        add_skill_xp(gs, SkillId::SOCIAL_MEDIA, 1);
        break;
    case ComputerUsePreference::PROGRAMMING:
        add_skill_xp(gs, SkillId::PROGRAMMING, 1);
        break;
    }
}

/**
 * @brief Process once-per-day gameplay systems for each newly reached game day.
 * @param gs Mutable game state.
 */
static void update_daily_gameplay(GameState& gs) {
    const int current_day = gameplay_day_index(gs);
    if (gs.last_processed_game_day < 0) {
        gs.last_processed_game_day = current_day;
        return;
    }
    if (current_day <= gs.last_processed_game_day) return;

    for (int day = gs.last_processed_game_day + 1; day <= current_day; day++) {
        process_employment_day(gs, day);
        process_computer_use_day(gs);
    }
    gs.last_processed_game_day = current_day;
}

/**
 * @brief Queue the computer-use preference chooser when a computer exists but no default use is set.
 * @param gs Mutable game state.
 */
static void update_computer_use_preference_prompt(GameState& gs) {
    if (!gs.access.owns_computer) return;
    if (gs.access.computer_use_preference != ComputerUsePreference::NONE) return;
    if (gs.decision_popup_visible) return;

    queue_computer_use_preference_prompt(gs);
}

/**
 * @brief Return the current gameplay-speed ladder value in hours per real second.
 * @param gs Current game state.
 * @return Rounded gameplay hours advanced per real second.
 */
static int gameplay_hours_per_second(const GameState& gs) {
    return static_cast<int>((1.0f / gs.seconds_per_game_hour) + 0.5f);
}

/**
 * @brief Write a discrete gameplay-speed ladder value back into the state.
 * @param gs Mutable game state.
 * @param hours_per_second Gameplay hours that should pass per real second.
 */
static void set_gameplay_hours_per_second(GameState& gs, int hours_per_second) {
    gs.seconds_per_game_hour = 1.0f / static_cast<float>(hours_per_second);
}

/**
 * @brief Slow down gameplay time progression along the discrete speed ladder.
 * @param gs Mutable game state.
 * @return True when the time scale changed.
 */
static bool slow_down_gameplay_time(GameState& gs) {
    const int current = gameplay_hours_per_second(gs);
    if (current <= 1) return false;

    int next = current;
    if (current == 24) next = 16;
    else if (current == 8760) next = 5760;
    else if (current == 5760) next = 2880;
    else if (current == 2880) next = 1440;
    else if (current == 1440) next = 720;
    else if (current >= 720) next = 384;
    else next = current / 2;

    set_gameplay_hours_per_second(gs, next);
    return true;
}

/**
 * @brief Speed up gameplay time progression along the discrete speed ladder.
 * @param gs Mutable game state.
 * @return True when the time scale changed.
 */
static bool speed_up_gameplay_time(GameState& gs) {
    const int current = gameplay_hours_per_second(gs);
    if (current >= 8760) return false;

    int next = current;
    if (current == 16) next = 24;
    else if (current == 720) next = 1440;
    else if (current == 1440) next = 2880;
    else if (current == 2880) next = 5760;
    else if (current == 5760) next = 8760;
    else if (current >= 384) next = 720;
    else next = current * 2;

    set_gameplay_hours_per_second(gs, next);
    return true;
}

// ── Per-scene update handlers ─────────────────────────────────────────────────

/**
 * @brief Handle input on the Title screen.
 *
 * Options: 0 = NEW GAME → CHARACTER_CREATE, 1 = ACHIEVEMENTS, 2 = OPTIONS, 3 = QUIT.
 */
static void update_title(GameState& gs, const InputState& in) {
    if (navigate_menu(gs.menu_selection, 4, in))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);

    if (in.enter_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        if (gs.menu_selection == 0)      transition(gs, Scene::CHARACTER_CREATE);
        else if (gs.menu_selection == 1) transition(gs, Scene::ACHIEVEMENTS);
        else if (gs.menu_selection == 2) transition(gs, Scene::OPTIONS);
        else                             gs.quit_requested = true;
    }
}

/**
 * @brief Handle input on the Achievements screen.
 */
static void update_achievements(GameState& gs, const InputState& in) {
    if (in.enter_pressed || in.escape_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        transition(gs, Scene::TITLE);
    }
}

/**
 * @brief Handle input on the Options screen.
 *
 * Options: 0 = MASTER VOLUME, 1 = MUSIC VOLUME, 2 = SFX VOLUME, 3 = BACK.
 * LEFT/RIGHT adjusts the selected volume row.
 */
static void update_options(GameState& gs, const InputState& in) {
    if (navigate_menu(gs.menu_selection, 4, in))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);

    bool changed = false;
    if (gs.menu_selection == 0)      changed = adjust_volume(gs.master_volume_percent, in);
    else if (gs.menu_selection == 1) changed = adjust_volume(gs.music_volume_percent, in);
    else if (gs.menu_selection == 2) changed = adjust_volume(gs.sfx_volume_percent, in);
    if (changed) queue_ui_sfx(gs, UiSfx::NAVIGATE);

    if (in.enter_pressed && gs.menu_selection == 3) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        transition(gs, Scene::TITLE);
    }
    if (in.escape_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        transition(gs, Scene::TITLE);
    }
}

/**
 * @brief Handle input on the Character Creation choice screen.
 *
 * Options: 0 = RANDOM, 1 = CUSTOMIZE, 2 = BACK.
 */
static void update_character_create(GameState& gs, const InputState& in) {
    if (navigate_menu(gs.menu_selection, 3, in))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);

    if (in.enter_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        if (gs.menu_selection == 0) {
            gs.random_char_generated = false;
            transition(gs, Scene::RANDOM_CHARACTER);
        } else if (gs.menu_selection == 1) {
            gs.text_input.clear();
            gs.custom_char_field    = 0;
            int cy = current_year();
            gs.year_selection       = (cy >= YEAR_MIN && cy <= YEAR_MAX)
                                      ? cy - YEAR_MIN : 0;
            gs.month_selection      = 1;
            gs.day_selection        = 1;
            gs.location_selection   = 0;
            transition(gs, Scene::CUSTOM_CHARACTER);
        } else {
            transition(gs, Scene::TITLE);
        }
    }
    if (in.escape_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        transition(gs, Scene::TITLE);
    }
}

/**
 * @brief Handle input on the Random Character screen.
 *
 * Generates a character on first entry.
 * Options: 0 = RE-ROLL, 1 = ACCEPT → GAMEPLAY, 2 = BACK.
 */
static void update_random_character(GameState& gs, const InputState& in) {
    if (!gs.random_char_generated) {
        gs.player                = random_player();
        gs.random_char_generated = true;
        gs.menu_selection        = 0;
        minfo("rolled random character: %s", gs.player.name.c_str());
    }

    if (navigate_menu(gs.menu_selection, 3, in))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);

    if (in.enter_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        if (gs.menu_selection == 0) {
            gs.player                = random_player();
            gs.random_char_generated = true;
            minfo("re-rolled character: %s", gs.player.name.c_str());
        } else if (gs.menu_selection == 1) {
            gs.new_game_started = true;
            reset_gameplay_clock(gs);
            award_starting_achievements(gs);
            transition(gs, Scene::GAMEPLAY);
        } else {
            gs.random_char_generated = false;
            transition(gs, Scene::CHARACTER_CREATE);
        }
    }
    if (in.escape_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        gs.random_char_generated = false;
        transition(gs, Scene::CHARACTER_CREATE);
    }
}

/**
 * @brief Handle input on the Custom Character screen.
 *
 * Three sequential fields:
 *   - 0: Name     — keyboard text input, BACKSPACE to delete
 *   - 1: Year     — LEFT/RIGHT (or UP/DOWN) to cycle through [YEAR_MIN, YEAR_MAX]
 *   - 2: Month    — LEFT/RIGHT changes month
 *   - 3: Day      — LEFT/RIGHT changes day, clamped to the month/year
 *   - 4: Location — LEFT/RIGHT (with repeat) scrolls locations; UP/DOWN navigates fields
 *
 * ENTER advances to the next field; on the last field it confirms the
 * character and transitions to GAMEPLAY.  ESC goes to the previous field
 * or back to CHARACTER_CREATE on field 0.
 */
static void update_custom_character(GameState& gs, const InputState& in) {
    static const int FIELD_COUNT = 7;  // 0=name, 1=year, 2=month, 3=day, 4=location, 5=start, 6=back
    const int prev_field = gs.custom_char_field;

    // ESC: go back one field or exit to previous screen
    if (in.escape_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        if (gs.custom_char_field > 0) gs.custom_char_field--;
        else                          transition(gs, Scene::CHARACTER_CREATE);
        return;
    }

    // ENTER: advance, start game, or go back depending on active field
    if (in.enter_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        if (gs.custom_char_field == 5) {
            // Commit values from transient state into player data
            gs.player.name          = gs.text_input.empty() ? "Anonymous" : gs.text_input;
            gs.player.year_of_birth = YEAR_MIN + gs.year_selection;
            gs.player.birth_month   = gs.month_selection;
            gs.player.birth_day     = gs.day_selection;
            gs.player.location      = ALL_LOCATIONS[gs.location_selection];
            gs.player.wealth        = Wealth::POOR;
            gs.player.health        = Health::SICKLY;
            gs.player.talent        = Talent::SLOW;
            gs.player.env           = Environment::LOWER_CLASS;
            gs.new_game_started     = true;
            reset_gameplay_clock(gs);
            award_starting_achievements(gs);
            minfo("custom character confirmed: %s", gs.player.name.c_str());
            transition(gs, Scene::GAMEPLAY);
        } else if (gs.custom_char_field == 6) {
            transition(gs, Scene::CHARACTER_CREATE);
        } else {
            gs.custom_char_field++;
        }
        return;
    }

    // Per-field input
    switch (gs.custom_char_field) {
    case 0: // Name — type freely; LEFT/RIGHT rolls a random name; UP/DOWN navigates fields
        if (in.char_pressed != 0 && gs.text_input.size() < 24)
            gs.text_input += static_cast<char>(in.char_pressed);
        if (in.backspace_pressed && !gs.text_input.empty())
            gs.text_input.pop_back();
        if (in.left_repeat || in.right_repeat) {
            PlayerData tmp = random_player();
            gs.text_input  = tmp.name;
            queue_ui_sfx(gs, UiSfx::NAVIGATE);
        }
        if (in.down_pressed && gs.custom_char_field < FIELD_COUNT - 1)
            gs.custom_char_field++;
        break;

    case 1: // Year — LEFT/RIGHT (with repeat) changes year; UP/DOWN navigates fields
        if (in.left_repeat)
            gs.year_selection = (gs.year_selection - 1 + YEAR_COUNT) % YEAR_COUNT;
        if (in.right_repeat)
            gs.year_selection = (gs.year_selection + 1) % YEAR_COUNT;
        clamp_custom_birth_day(gs);
        if (in.up_pressed && gs.custom_char_field > 0)
            gs.custom_char_field--;
        if (in.down_pressed && gs.custom_char_field < FIELD_COUNT - 1)
            gs.custom_char_field++;
        if (in.left_repeat || in.right_repeat)
            queue_ui_sfx(gs, UiSfx::NAVIGATE);
        break;

    case 2: // Month — LEFT/RIGHT changes month; UP/DOWN navigates fields
        if (in.left_repeat)
            gs.month_selection = (gs.month_selection + 10) % 12 + 1;
        if (in.right_repeat)
            gs.month_selection = (gs.month_selection % 12) + 1;
        clamp_custom_birth_day(gs);
        if (in.up_pressed && gs.custom_char_field > 0)
            gs.custom_char_field--;
        if (in.down_pressed && gs.custom_char_field < FIELD_COUNT - 1)
            gs.custom_char_field++;
        if (in.left_repeat || in.right_repeat)
            queue_ui_sfx(gs, UiSfx::NAVIGATE);
        break;

    case 3: // Day — LEFT/RIGHT changes day within the selected month/year
        if (in.left_repeat) {
            gs.day_selection--;
            if (gs.day_selection < 1)
                gs.day_selection = days_in_month(YEAR_MIN + gs.year_selection, gs.month_selection);
        }
        if (in.right_repeat) {
            gs.day_selection++;
            if (gs.day_selection > days_in_month(YEAR_MIN + gs.year_selection, gs.month_selection))
                gs.day_selection = 1;
        }
        if (in.up_pressed && gs.custom_char_field > 0)
            gs.custom_char_field--;
        if (in.down_pressed && gs.custom_char_field < FIELD_COUNT - 1)
            gs.custom_char_field++;
        if (in.left_repeat || in.right_repeat)
            queue_ui_sfx(gs, UiSfx::NAVIGATE);
        break;

    case 4: // Location — LEFT/RIGHT (with repeat) changes location; UP/DOWN navigates fields
        if (in.left_repeat)
            gs.location_selection = (gs.location_selection - 1 + unlocked_location_count())
                                    % unlocked_location_count();
        if (in.right_repeat)
            gs.location_selection = (gs.location_selection + 1) % unlocked_location_count();
        if (in.up_pressed && gs.custom_char_field > 0)
            gs.custom_char_field--;
        if (in.down_pressed && gs.custom_char_field < FIELD_COUNT - 1)
            gs.custom_char_field++;
        if (in.left_repeat || in.right_repeat)
            queue_ui_sfx(gs, UiSfx::NAVIGATE);
        break;

    case 5: // Start Game button — UP/DOWN navigates between buttons
        if (in.up_pressed)   gs.custom_char_field--;
        if (in.down_pressed) gs.custom_char_field++;
        break;

    case 6: // Back button — UP navigates to Start Game
        if (in.up_pressed) gs.custom_char_field--;
        break;
    }

    if (gs.custom_char_field != prev_field)
        queue_ui_sfx(gs, UiSfx::NAVIGATE);
}

/**
 * @brief Stub handler for the main gameplay scene.
 * @todo Implement the core gameplay loop.
 */
static void update_gameplay(GameState& gs, const InputState& in) {
    update_achievement_popup(gs, in);
    update_decision_popup(gs);

    if (in.frame_seconds > 0.0f)
        gs.play_session_seconds += in.frame_seconds;

    if (gs.decision_popup_visible) {
        handle_active_decision(gs, in);
        return;
    }

    if (in.space_pressed) {
        gs.gameplay_paused = !gs.gameplay_paused;
        queue_ui_sfx(gs, UiSfx::SELECT);
    }

    if (in.left_bracket_pressed && slow_down_gameplay_time(gs))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);
    if (in.right_bracket_pressed && speed_up_gameplay_time(gs))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);
    if (in.enter_pressed && gs.access.owns_computer) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        queue_computer_use_preference_prompt(gs);
    }

    if (!gs.gameplay_paused && gs.seconds_per_game_hour > 0.0f && in.frame_seconds > 0.0f) {
        gs.gameplay_time_accumulator += in.frame_seconds;
        while (gs.gameplay_time_accumulator >= gs.seconds_per_game_hour) {
            gs.gameplay_time_accumulator -= gs.seconds_per_game_hour;
            gs.elapsed_game_hours++;
        }
    }

    update_daily_gameplay(gs);
    update_gameplay_achievements(gs);
    update_news_events(gs);
    queue_age_16_job_prompt_if_needed(gs);
    update_job_search(gs);
    update_computer_purchase_offer(gs);
    update_computer_use_preference_prompt(gs);
    update_decision_popup(gs);
    if (gs.decision_popup_visible) return;

    // ESC returns to title for now
    if (in.escape_pressed) {
        queue_ui_sfx(gs, UiSfx::SELECT);
        transition(gs, Scene::TITLE);
    }
}

// ── Public dispatch ───────────────────────────────────────────────────────────

void update_scene(GameState& gs, const InputState& in) {
    switch (gs.current_scene) {
    case Scene::TITLE:            update_title(gs, in);            break;
    case Scene::ACHIEVEMENTS:     update_achievements(gs, in);     break;
    case Scene::OPTIONS:          update_options(gs, in);          break;
    case Scene::CHARACTER_CREATE: update_character_create(gs, in); break;
    case Scene::RANDOM_CHARACTER: update_random_character(gs, in); break;
    case Scene::CUSTOM_CHARACTER: update_custom_character(gs, in); break;
    case Scene::GAMEPLAY:         update_gameplay(gs, in);         break;
    }
}
