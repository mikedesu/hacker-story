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
    gs.achievement_popup_visible = false;
    gs.achievement_popup_seconds_remaining = 0.0f;
    gs.next_news_event_idx = 0;
    gs.news_feed.clear();
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

    if (in.frame_seconds > 0.0f)
        gs.play_session_seconds += in.frame_seconds;

    if (in.space_pressed) {
        gs.gameplay_paused = !gs.gameplay_paused;
        queue_ui_sfx(gs, UiSfx::SELECT);
    }

    if (in.left_bracket_pressed && slow_down_gameplay_time(gs))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);
    if (in.right_bracket_pressed && speed_up_gameplay_time(gs))
        queue_ui_sfx(gs, UiSfx::NAVIGATE);

    if (!gs.gameplay_paused && gs.seconds_per_game_hour > 0.0f && in.frame_seconds > 0.0f) {
        gs.gameplay_time_accumulator += in.frame_seconds;
        while (gs.gameplay_time_accumulator >= gs.seconds_per_game_hour) {
            gs.gameplay_time_accumulator -= gs.seconds_per_game_hour;
            gs.elapsed_game_hours++;
        }
    }

    update_gameplay_achievements(gs);
    update_news_events(gs);

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
