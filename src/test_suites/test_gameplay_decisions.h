/** @file test_suites/test_gameplay_decisions.h
 *  @brief CxxTest suite for gameplay decision popups and age-gated prompts.
 *
 *  Verifies the first gameplay decision slice: once the player reaches age 16,
 *  the game should offer to start looking for a part-time job.
 */

#pragma once
#include <cstdlib>
#include <cxxtest/TestSuite.h>
#include "../gamestate.h"
#include "../input_state.h"
#include "../libinput.h"

/**
 * @brief Return the exact number of hours from birth to the given January 1 birthday.
 * @param birth_year Player birth year.
 * @param target_age Whole-number age milestone to reach.
 * @return Total in-game hours between birth and the target birthday.
 */
static inline long long hours_until_jan_1_birthday(int birth_year, int target_age) {
    long long days = 0;
    for (int year = birth_year; year < birth_year + target_age; year++) {
        days += is_leap_year(year) ? 366 : 365;
    }
    return days * 24LL;
}

/**
 * @brief Advance a gameplay state to age 16 and accept the initial job-search prompt.
 * @param gs Mutable game state to prepare.
 */
static inline void enable_job_search_at_age_16(GameState& gs) {
    gs.current_scene = Scene::GAMEPLAY;
    gs.player.year_of_birth = 2000;
    gs.player.birth_month = 1;
    gs.player.birth_day = 1;
    gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16);

    InputState frame;
    update_scene(gs, frame);

    InputState enter;
    enter.enter_pressed = true;
    update_scene(gs, enter);
}

/**
 * @brief Prepare a gameplay state with an accepted grocery-clerk job on age 16.
 * @param gs Mutable game state to prepare.
 */
static inline void accept_grocery_job_at_age_16(GameState& gs) {
    srand(4);
    enable_job_search_at_age_16(gs);

    InputState frame;
    update_scene(gs, frame);

    InputState enter;
    enter.enter_pressed = true;
    update_scene(gs, enter);
}

/**
 * @brief Prime gameplay day-processing without triggering age-16 job logic.
 * @param gs Mutable game state to prepare.
 */
static inline void prime_gameplay_day_tracking(GameState& gs) {
    gs.current_scene = Scene::GAMEPLAY;
    gs.player.year_of_birth = 2000;
    gs.player.birth_month = 1;
    gs.player.birth_day = 1;
    gs.elapsed_game_hours = 0;
    gs.age_16_job_prompt_shown = true;

    InputState frame;
    update_scene(gs, frame);
}

/**
 * @brief Buy a starter computer and leave the preference chooser open.
 * @param gs Mutable game state to prepare.
 */
static inline void buy_starter_computer(GameState& gs) {
    prime_gameplay_day_tracking(gs);
    gs.player.cash = 300.0f;

    InputState frame;
    update_scene(gs, frame);

    InputState enter;
    enter.enter_pressed = true;
    update_scene(gs, enter);
}

/**
 * @brief Buy a starter computer and commit a specific computer-use preference.
 * @param gs Mutable game state to prepare.
 * @param choice_index Preference choice index in the modal.
 */
static inline void choose_computer_use_preference(GameState& gs, int choice_index) {
    buy_starter_computer(gs);
    for (int i = 0; i < choice_index; i++) {
        InputState down;
        down.down_pressed = true;
        update_scene(gs, down);
    }

    InputState enter;
    enter.enter_pressed = true;
    update_scene(gs, enter);
}

/**
 * @brief Tests for the first gameplay decision modal and consequences.
 */
class TestGameplayDecisions : public CxxTest::TestSuite {
public:

    void test_skills_default_to_zero_progress() {
        GameState gs;
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].level, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].xp, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].level, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].xp, 0);
    }

    void test_age_16_prompt_does_not_show_before_birthday() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2000;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16) - 24;

        InputState in;
        update_scene(gs, in);

        TS_ASSERT_EQUALS(gameplay_age_years(gs), 15);
        TS_ASSERT(!gs.decision_popup_visible);
        TS_ASSERT(!gs.age_16_job_prompt_shown);
    }

    void test_age_16_prompt_shows_once_when_birthday_is_reached() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2000;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16);

        InputState in;
        update_scene(gs, in);

        TS_ASSERT_EQUALS(gameplay_age_years(gs), 16);
        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT(gs.age_16_job_prompt_shown);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Part-Time Work");
        TS_ASSERT_EQUALS(gs.active_decision.choices.size(), 2U);
        TS_ASSERT_EQUALS(gs.active_decision.choices[0].label, "Yes");
        TS_ASSERT_EQUALS(gs.active_decision.choices[1].label, "No");
    }

    void test_confirming_yes_starts_job_search() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2000;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16);

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(gs.employment.searching_for_job);
        TS_ASSERT(!gs.decision_popup_visible);
        TS_ASSERT(!gs.employment.employed);
        TS_ASSERT_EQUALS(gs.employment.current_job, JobId::NONE);
    }

    void test_confirming_no_leaves_job_search_disabled_and_prompt_does_not_repeat() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2000;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16);

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        InputState down;
        down.down_pressed = true;
        update_scene(gs, down);
        TS_ASSERT_EQUALS(gs.decision_selection, 1);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(!gs.employment.searching_for_job);
        TS_ASSERT(!gs.decision_popup_visible);

        InputState later_frame;
        later_frame.frame_seconds = 1.0f;
        update_scene(gs, later_frame);
        TS_ASSERT(!gs.decision_popup_visible);
        TS_ASSERT(gs.age_16_job_prompt_shown);
    }

    void test_active_decision_pauses_gameplay_time_progression() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2000;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16);

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        gs.seconds_per_game_hour = 1.0f;
        const long long before = gs.elapsed_game_hours;

        InputState waiting_frame;
        waiting_frame.frame_seconds = 3.0f;
        update_scene(gs, waiting_frame);

        TS_ASSERT_EQUALS(gs.elapsed_game_hours, before);
    }

    void test_daily_job_search_roll_can_queue_grocery_offer() {
        srand(4);

        GameState gs;
        enable_job_search_at_age_16(gs);
        TS_ASSERT(gs.employment.searching_for_job);

        InputState frame;
        update_scene(gs, frame);

        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Job Offer");
        TS_ASSERT_EQUALS(gs.active_decision.choices.size(), 2U);
        TS_ASSERT_EQUALS(gs.active_decision.choices[0].label, "Accept");
        TS_ASSERT_EQUALS(gs.active_decision.choices[1].label, "Decline");
    }

    void test_daily_job_search_roll_can_miss_without_offer() {
        srand(1);

        GameState gs;
        enable_job_search_at_age_16(gs);
        TS_ASSERT(gs.employment.searching_for_job);

        InputState frame;
        update_scene(gs, frame);

        TS_ASSERT(!gs.decision_popup_visible);
        TS_ASSERT(!gs.employment.employed);
        TS_ASSERT_EQUALS(gs.employment.current_job, JobId::NONE);
    }

    void test_accepting_grocery_offer_sets_employment_state() {
        srand(4);

        GameState gs;
        enable_job_search_at_age_16(gs);

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(gs.employment.employed);
        TS_ASSERT(!gs.employment.searching_for_job);
        TS_ASSERT_EQUALS(gs.employment.current_job, JobId::GROCERY_STORE_CLERK);
        TS_ASSERT_EQUALS(gs.employment.hourly_wage_cents, 850);
    }

    void test_declining_grocery_offer_keeps_job_search_active_for_future_days() {
        srand(4);

        GameState gs;
        enable_job_search_at_age_16(gs);

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        InputState down;
        down.down_pressed = true;
        update_scene(gs, down);
        TS_ASSERT_EQUALS(gs.decision_selection, 1);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(!gs.employment.employed);
        TS_ASSERT(gs.employment.searching_for_job);
        TS_ASSERT_EQUALS(gs.employment.current_job, JobId::NONE);
        TS_ASSERT_EQUALS(gs.employment.hourly_wage_cents, 0);

        InputState same_day;
        update_scene(gs, same_day);
        TS_ASSERT(!gs.decision_popup_visible);

        gs.elapsed_game_hours += 24;
        srand(4);
        InputState next_day;
        update_scene(gs, next_day);
        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Job Offer");
    }

    void test_employment_accumulates_weekday_shifts_on_new_days_only() {
        GameState gs;
        accept_grocery_job_at_age_16(gs);
        TS_ASSERT(gs.employment.employed);

        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16) + 3 * 24LL; // 2016-01-04 Monday
        InputState day_frame;
        update_scene(gs, day_frame);

        TS_ASSERT_EQUALS(gs.employment.shifts_worked_this_week, 1);
        TS_ASSERT_DELTA(gs.player.cash, 0.0f, 0.001f);

        update_scene(gs, day_frame);
        TS_ASSERT_EQUALS(gs.employment.shifts_worked_this_week, 1);
        TS_ASSERT_DELTA(gs.player.cash, 0.0f, 0.001f);
    }

    void test_employment_pays_after_first_full_workweek() {
        GameState gs;
        accept_grocery_job_at_age_16(gs);
        TS_ASSERT(gs.employment.employed);

        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16) + 7 * 24LL; // through Friday 2016-01-08
        InputState week_frame;
        update_scene(gs, week_frame);

        TS_ASSERT_EQUALS(gs.employment.last_paycheck_cents, 17000);
        TS_ASSERT_DELTA(gs.player.cash, 170.0f, 0.001f);
        TS_ASSERT_EQUALS(gs.employment.shifts_worked_this_week, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].level, 5);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].xp, 0);
    }

    void test_zero_mental_health_can_cause_shift_absence() {
        GameState gs;
        accept_grocery_job_at_age_16(gs);
        gs.wellbeing.mental_health = 0;
        gs.wellbeing.physical_health = 100;

        srand(1); // 83, so a 50% attendance chance misses
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16) + 3 * 24LL; // Monday
        InputState day_frame;
        update_scene(gs, day_frame);

        TS_ASSERT_EQUALS(gs.employment.shifts_worked_this_week, 0);
        TS_ASSERT_EQUALS(gs.employment.missed_shifts_this_week, 1);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].xp, 0);
    }

    void test_zero_physical_health_can_cause_shift_absence() {
        GameState gs;
        accept_grocery_job_at_age_16(gs);
        gs.wellbeing.mental_health = 100;
        gs.wellbeing.physical_health = 0;

        srand(1); // 83, so a 50% attendance chance misses
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16) + 3 * 24LL; // Monday
        InputState day_frame;
        update_scene(gs, day_frame);

        TS_ASSERT_EQUALS(gs.employment.shifts_worked_this_week, 0);
        TS_ASSERT_EQUALS(gs.employment.missed_shifts_this_week, 1);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].xp, 0);
    }

    void test_zero_attendance_week_results_in_zero_paycheck() {
        GameState gs;
        accept_grocery_job_at_age_16(gs);
        gs.wellbeing.mental_health = 0;
        gs.wellbeing.physical_health = 0;

        srand(4); // irrelevant at 0% attendance, but explicit for determinism
        gs.elapsed_game_hours = hours_until_jan_1_birthday(2000, 16) + 7 * 24LL; // through Friday
        InputState week_frame;
        update_scene(gs, week_frame);

        TS_ASSERT_EQUALS(gs.employment.last_paycheck_cents, 0);
        TS_ASSERT_DELTA(gs.player.cash, 0.0f, 0.001f);
        TS_ASSERT_EQUALS(gs.employment.shifts_worked_this_week, 0);
        TS_ASSERT_EQUALS(gs.employment.missed_shifts_this_week, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::SOCIALIZING)].xp, 0);
    }

    void test_affording_starter_computer_queues_purchase_offer() {
        GameState gs;
        prime_gameplay_day_tracking(gs);
        gs.player.cash = 300.0f;

        InputState frame;
        update_scene(gs, frame);

        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Used Computer");
        TS_ASSERT_EQUALS(gs.active_decision.choices.size(), 2U);
        TS_ASSERT_EQUALS(gs.active_decision.choices[0].label, "Buy");
        TS_ASSERT_EQUALS(gs.active_decision.choices[1].label, "Not Yet");
    }

    void test_buying_starter_computer_spends_cash_and_grants_access() {
        GameState gs;
        prime_gameplay_day_tracking(gs);
        gs.player.cash = 300.0f;

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(gs.access.owns_computer);
        TS_ASSERT_DELTA(gs.player.cash, 0.0f, 0.001f);
        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Computer Use");
    }

    void test_declining_starter_computer_waits_for_cash_increase_before_reoffering() {
        GameState gs;
        prime_gameplay_day_tracking(gs);
        gs.player.cash = 300.0f;

        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT(gs.decision_popup_visible);

        InputState down;
        down.down_pressed = true;
        update_scene(gs, down);
        TS_ASSERT_EQUALS(gs.decision_selection, 1);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(!gs.access.owns_computer);
        TS_ASSERT(!gs.decision_popup_visible);

        InputState later_frame;
        update_scene(gs, later_frame);
        TS_ASSERT(!gs.decision_popup_visible);

        gs.player.cash = 300.01f;
        update_scene(gs, later_frame);
        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Used Computer");
    }

    void test_selecting_practice_sets_persistent_computer_use_preference() {
        GameState gs;
        choose_computer_use_preference(gs, 0);

        TS_ASSERT(gs.access.owns_computer);
        TS_ASSERT_EQUALS(gs.access.computer_use_preference, ComputerUsePreference::PRACTICE);
        TS_ASSERT(!gs.decision_popup_visible);
    }

    void test_enter_reopens_computer_use_preferences_with_current_selection_highlighted() {
        GameState gs;
        choose_computer_use_preference(gs, 4);
        TS_ASSERT_EQUALS(gs.access.computer_use_preference, ComputerUsePreference::PROGRAMMING);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.active_decision.title, "Computer Use");
        TS_ASSERT_EQUALS(gs.decision_selection, 4);
    }

    void test_computer_skills_do_not_grow_without_computer() {
        GameState gs;
        prime_gameplay_day_tracking(gs);

        gs.elapsed_game_hours = 5 * 24LL;
        InputState frame;
        update_scene(gs, frame);

        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].level, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].xp, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::PROGRAMMING)].level, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::PROGRAMMING)].xp, 0);
    }

    void test_practice_preference_grows_computer_basics() {
        GameState gs;
        choose_computer_use_preference(gs, 0);

        gs.elapsed_game_hours = 5 * 24LL;
        InputState frame;
        update_scene(gs, frame);

        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].level, 1);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].xp, 0);
    }

    void test_programming_preference_grows_programming_skill() {
        GameState gs;
        choose_computer_use_preference(gs, 4);

        gs.elapsed_game_hours = 5 * 24LL;
        InputState frame;
        update_scene(gs, frame);

        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::PROGRAMMING)].level, 1);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::PROGRAMMING)].xp, 0);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)].level, 0);
    }

    void test_preference_persists_until_player_changes_it() {
        GameState gs;
        choose_computer_use_preference(gs, 2); // Gaming

        gs.elapsed_game_hours = 5 * 24LL;
        InputState frame;
        update_scene(gs, frame);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::GAMING)].level, 1);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT(gs.decision_popup_visible);
        TS_ASSERT_EQUALS(gs.decision_selection, 2);

        InputState down;
        down.down_pressed = true; // Social Media
        update_scene(gs, down);
        update_scene(gs, down);    // Programming

        InputState confirm;
        confirm.enter_pressed = true;
        update_scene(gs, confirm);

        TS_ASSERT_EQUALS(gs.access.computer_use_preference, ComputerUsePreference::PROGRAMMING);

        gs.elapsed_game_hours = 10 * 24LL;
        update_scene(gs, frame);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::GAMING)].level, 1);
        TS_ASSERT_EQUALS(gs.player.skills[skill_index(SkillId::PROGRAMMING)].level, 1);
    }
};
