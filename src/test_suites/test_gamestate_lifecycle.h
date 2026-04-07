/** @file test_suites/test_gamestate_lifecycle.h
 *  @brief CxxTest suite for GameState initialisation and scene transitions.
 *
 *  Exercises the `update_scene()` function in libinput using the
 *  platform-agnostic `InputState` so no raylib window is needed.
 */

#pragma once
#include <cxxtest/TestSuite.h>
#include "../gamestate.h"
#include "../input_state.h"
#include "../libinput.h"

/**
 * @brief Tests for GameState default values and scene navigation.
 */
class TestGamestateLifecycle : public CxxTest::TestSuite {
public:

    // ── Initial state ─────────────────────────────────────────────────────────

    void test_default_scene_is_title() {
        GameState gs;
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    void test_quit_not_requested_initially() {
        GameState gs;
        TS_ASSERT(!gs.quit_requested);
    }

    void test_new_game_not_started_initially() {
        GameState gs;
        TS_ASSERT(!gs.new_game_started);
    }

    void test_menu_selection_zero_initially() {
        GameState gs;
        TS_ASSERT_EQUALS(gs.menu_selection, 0);
    }

    void test_default_sfx_volume_is_70_percent() {
        GameState gs;
        TS_ASSERT_EQUALS(gs.sfx_volume_percent, 70);
    }

    void test_default_music_volume_is_50_percent() {
        GameState gs;
        TS_ASSERT_EQUALS(gs.music_volume_percent, 50);
    }

    void test_default_master_volume_is_100_percent() {
        GameState gs;
        TS_ASSERT_EQUALS(gs.master_volume_percent, 100);
    }

    void test_default_seconds_per_game_hour_is_one_day_scale() {
        GameState gs;
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f / 24.0f, 0.001f);
    }

    void test_gameplay_time_scale_caps_are_defined() {
        TS_ASSERT_DELTA(GAMEPLAY_SECONDS_PER_HOUR_MIN, 1.0f / 8760.0f, 0.0001f);
        TS_ASSERT_DELTA(GAMEPLAY_SECONDS_PER_HOUR_MAX, 1.0f, 0.001f);
    }

    // ── Title → Character Create ──────────────────────────────────────────────

    void test_enter_on_new_game_transitions_to_character_create() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState in;
        in.enter_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::CHARACTER_CREATE);
    }

    void test_transition_resets_menu_selection() {
        GameState gs;
        gs.menu_selection = 5;
        InputState in;
        in.enter_pressed = true;
        // selection=5 on title wraps: 5 % 4 == 1 = ACHIEVEMENTS
        // so quit_requested will be true, not a scene transition — test reset differently
        GameState gs2;
        gs2.current_scene  = Scene::CHARACTER_CREATE;
        gs2.menu_selection = 2;  // BACK option
        update_scene(gs2, in);
        TS_ASSERT_EQUALS(gs2.current_scene, Scene::TITLE);
        TS_ASSERT_EQUALS(gs2.menu_selection, 0);
    }

    // ── Title: QUIT ───────────────────────────────────────────────────────────

    void test_down_then_enter_sets_quit() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState nav;
        nav.down_pressed = true;
        update_scene(gs, nav);
        TS_ASSERT_EQUALS(gs.menu_selection, 1);

        update_scene(gs, nav);
        TS_ASSERT_EQUALS(gs.menu_selection, 2);

        update_scene(gs, nav);
        TS_ASSERT_EQUALS(gs.menu_selection, 3);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT(gs.quit_requested);
    }

    // ── Menu navigation wrapping ──────────────────────────────────────────────

    void test_up_wraps_from_zero_to_last() {
        GameState gs;
        gs.title_menu_revealed = true;
        TS_ASSERT_EQUALS(gs.menu_selection, 0);
        InputState up;
        up.up_pressed = true;
        update_scene(gs, up);
        TS_ASSERT_EQUALS(gs.menu_selection, 3);  // title has 4 options: wraps to 3
    }

    void test_down_advances_selection() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);
        TS_ASSERT_EQUALS(gs.menu_selection, 1);
    }

    void test_down_then_enter_on_title_opens_options() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);

        update_scene(gs, dn);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::OPTIONS);
    }

    void test_down_then_enter_on_title_opens_achievements() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::ACHIEVEMENTS);
    }

    // ── Character Create: back navigation ────────────────────────────────────

    void test_escape_from_character_create_returns_to_title() {
        GameState gs;
        gs.current_scene = Scene::CHARACTER_CREATE;
        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    void test_back_option_in_character_create_returns_to_title() {
        GameState gs;
        gs.current_scene  = Scene::CHARACTER_CREATE;
        gs.menu_selection = 2;  // BACK
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    // ── Character Create → Random ─────────────────────────────────────────────

    void test_random_option_transitions_to_random_character() {
        GameState gs;
        gs.current_scene  = Scene::CHARACTER_CREATE;
        gs.menu_selection = 0;
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::RANDOM_CHARACTER);
    }

    // ── Character Create → Custom ─────────────────────────────────────────────

    void test_customize_option_transitions_to_custom_character() {
        GameState gs;
        gs.current_scene  = Scene::CHARACTER_CREATE;
        gs.menu_selection = 1;
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::CUSTOM_CHARACTER);
    }

    void test_custom_char_field_reset_on_transition() {
        GameState gs;
        gs.current_scene    = Scene::CHARACTER_CREATE;
        gs.menu_selection   = 1;
        gs.custom_char_field = 2;
        gs.text_input       = "leftover";
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.custom_char_field, 0);
        TS_ASSERT_EQUALS(gs.text_input, "");
    }

    // ── Gameplay stub: ESC returns to title ───────────────────────────────────

    void test_escape_from_gameplay_returns_to_title() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    void test_space_toggles_gameplay_pause() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        TS_ASSERT(!gs.gameplay_paused);

        InputState space;
        space.space_pressed = true;
        update_scene(gs, space);
        TS_ASSERT(gs.gameplay_paused);

        update_scene(gs, space);
        TS_ASSERT(!gs.gameplay_paused);
    }

    void test_gameplay_advances_one_hour_after_two_seconds() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 1900;
        gs.player.birth_month = 5;
        gs.player.birth_day = 17;
        gs.seconds_per_game_hour = 1.0f;

        InputState frame;
        frame.frame_seconds = 1.0f;
        update_scene(gs, frame);
        TS_ASSERT_EQUALS(gs.elapsed_game_hours, 1LL);

        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 1900);
        TS_ASSERT_EQUALS(dt.month, 5);
        TS_ASSERT_EQUALS(dt.day, 17);
        TS_ASSERT_EQUALS(dt.hour, 1);
        TS_ASSERT_EQUALS(gameplay_age_years(gs), 0);
    }

    void test_gameplay_does_not_advance_while_paused() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.gameplay_paused = true;
        gs.elapsed_game_hours = 5;

        InputState frame;
        frame.frame_seconds = 10.0f;
        update_scene(gs, frame);
        TS_ASSERT_EQUALS(gs.elapsed_game_hours, 5LL);
        TS_ASSERT_DELTA(gs.play_session_seconds, 10.0f, 0.001f);
    }

    void test_gameplay_accumulates_partial_time_steps() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f;

        InputState frame;
        frame.frame_seconds = 0.5f;
        update_scene(gs, frame);
        TS_ASSERT_EQUALS(gs.elapsed_game_hours, 0LL);

        update_scene(gs, frame);
        TS_ASSERT_EQUALS(gs.elapsed_game_hours, 1LL);
    }

    void test_gameplay_calendar_rolls_into_next_year() {
        GameState gs;
        gs.player.year_of_birth = 1900;
        gs.player.birth_month = 6;
        gs.player.birth_day = 15;
        gs.elapsed_game_hours = 365LL * 24LL;

        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 1901);
        TS_ASSERT_EQUALS(dt.month, 6);
        TS_ASSERT_EQUALS(dt.day, 15);
        TS_ASSERT_EQUALS(dt.hour, 0);
        TS_ASSERT_EQUALS(gameplay_age_years(gs), 1);
    }

    void test_gameplay_age_does_not_advance_until_birthday() {
        GameState gs;
        gs.player.year_of_birth = 2000;
        gs.player.birth_month = 12;
        gs.player.birth_day = 31;
        gs.elapsed_game_hours = 364LL * 24LL; // 2000-12-30 00:00 in leap year? wait next patch?

        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 2001);
        TS_ASSERT_EQUALS(dt.month, 12);
        TS_ASSERT_EQUALS(dt.day, 30);
        TS_ASSERT_EQUALS(gameplay_age_years(gs), 0);
    }

    void test_living_into_1970_unlocks_survived_unix_epoch() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 1969;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.seconds_per_game_hour = 1.0f;
        gs.elapsed_game_hours = 365LL * 24LL - 1LL;  // 1969-12-31 23:00

        InputState frame;
        frame.frame_seconds = 1.0f;
        update_scene(gs, frame);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::SURVIVED_UNIX_EPOCH));
        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 1970);
        TS_ASSERT_EQUALS(dt.month, 1);
        TS_ASSERT_EQUALS(dt.day, 1);
        TS_ASSERT_EQUALS(dt.hour, 0);
    }

    void test_birth_in_1970_does_not_unlock_survived_unix_epoch_immediately() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 1970;

        InputState frame;
        frame.frame_seconds = 0.0f;
        update_scene(gs, frame);

        TS_ASSERT(!achievement_unlocked(gs, AchievementId::SURVIVED_UNIX_EPOCH));
    }

    void test_living_into_2000_unlocks_survived_y2k() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 1999;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.seconds_per_game_hour = 1.0f;
        gs.elapsed_game_hours = 365LL * 24LL - 1LL;  // 1999-12-31 23:00

        InputState frame;
        frame.frame_seconds = 1.0f;
        update_scene(gs, frame);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::SURVIVED_Y2K));
        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 2000);
        TS_ASSERT_EQUALS(dt.month, 1);
        TS_ASSERT_EQUALS(dt.day, 1);
        TS_ASSERT_EQUALS(dt.hour, 0);
    }

    void test_birth_in_2000_does_not_unlock_survived_y2k_immediately() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2000;

        InputState frame;
        frame.frame_seconds = 0.0f;
        update_scene(gs, frame);

        TS_ASSERT(!achievement_unlocked(gs, AchievementId::SURVIVED_Y2K));
    }

    void test_living_into_2038_unlocks_survived_2038() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2037;
        gs.player.birth_month = 1;
        gs.player.birth_day = 1;
        gs.seconds_per_game_hour = 1.0f;
        gs.elapsed_game_hours = 365LL * 24LL - 1LL;  // 2037-12-31 23:00

        InputState frame;
        frame.frame_seconds = 1.0f;
        update_scene(gs, frame);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::SURVIVED_2038));
        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 2038);
        TS_ASSERT_EQUALS(dt.month, 1);
        TS_ASSERT_EQUALS(dt.day, 1);
        TS_ASSERT_EQUALS(dt.hour, 0);
    }

    void test_birth_in_2038_does_not_unlock_survived_2038_immediately() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.player.year_of_birth = 2038;

        InputState frame;
        frame.frame_seconds = 0.0f;
        update_scene(gs, frame);

        TS_ASSERT(!achievement_unlocked(gs, AchievementId::SURVIVED_2038));
    }

    void test_left_arrow_slows_gameplay_time_by_doubling_seconds_per_hour() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 0.5f;

        InputState in;
        in.left_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f, 0.001f);
    }

    void test_right_arrow_speeds_up_gameplay_time_by_halving_seconds_per_hour() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f;

        InputState in;
        in.right_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 0.5f, 0.001f);
    }

    void test_right_arrow_steps_from_sixteen_hours_per_second_to_one_day_per_second() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f / 16.0f;

        InputState in;
        in.right_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f / 24.0f, 0.001f);
    }

    void test_left_arrow_steps_from_one_day_per_second_to_sixteen_hours_per_second() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f / 24.0f;

        InputState in;
        in.left_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f / 16.0f, 0.001f);
    }

    void test_left_arrow_respects_slowest_time_cap() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = GAMEPLAY_SECONDS_PER_HOUR_MAX;

        InputState in;
        in.left_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, GAMEPLAY_SECONDS_PER_HOUR_MAX, 0.001f);
    }

    void test_right_arrow_respects_fastest_time_cap() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = GAMEPLAY_SECONDS_PER_HOUR_MIN;

        InputState in;
        in.right_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, GAMEPLAY_SECONDS_PER_HOUR_MIN, 0.001f);
    }

    void test_left_arrow_steps_down_from_month_to_sixteen_days_per_second() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f / 720.0f;

        InputState in;
        in.left_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f / 384.0f, 0.001f);
    }

    void test_right_arrow_steps_from_one_month_per_second_to_two_months_per_second() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f / 720.0f;

        InputState in;
        in.right_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f / 1440.0f, 0.001f);
    }

    void test_right_arrow_reaches_one_year_per_second_cap() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = 1.0f / 5760.0f;

        InputState in;
        in.right_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, GAMEPLAY_SECONDS_PER_HOUR_MIN, 0.001f);
    }

    void test_left_arrow_steps_down_from_year_cap_to_eight_months_per_second() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;
        gs.seconds_per_game_hour = GAMEPLAY_SECONDS_PER_HOUR_MIN;

        InputState in;
        in.left_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_DELTA(gs.seconds_per_game_hour, 1.0f / 5760.0f, 0.001f);
    }

    void test_play_session_time_accumulates_during_gameplay() {
        GameState gs;
        gs.current_scene = Scene::GAMEPLAY;

        InputState frame;
        frame.frame_seconds = 3.25f;
        update_scene(gs, frame);
        TS_ASSERT_DELTA(gs.play_session_seconds, 3.25f, 0.001f);
    }

    void test_starting_new_game_resets_gameplay_clock_from_random_character() {
        GameState gs;
        gs.current_scene = Scene::RANDOM_CHARACTER;
        gs.random_char_generated = true;
        gs.menu_selection = 1;  // ACCEPT
        gs.player.year_of_birth = 1988;
        gs.player.birth_month = 9;
        gs.player.birth_day = 12;
        gs.elapsed_game_hours = 99;
        gs.gameplay_paused = true;
        gs.gameplay_time_accumulator = 1.5f;

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT_EQUALS(gs.current_scene, Scene::GAMEPLAY);
        TS_ASSERT_EQUALS(gs.elapsed_game_hours, 0LL);
        TS_ASSERT(!gs.gameplay_paused);
        TS_ASSERT_DELTA(gs.gameplay_time_accumulator, 0.0f, 0.001f);
        GameDateTime dt = gameplay_datetime(gs);
        TS_ASSERT_EQUALS(dt.year, 1988);
        TS_ASSERT_EQUALS(dt.month, 9);
        TS_ASSERT_EQUALS(dt.day, 12);
    }

    // ── Options: navigation and volume ───────────────────────────────────────

    void test_escape_from_options_returns_to_title() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    void test_escape_from_achievements_returns_to_title() {
        GameState gs;
        gs.current_scene = Scene::ACHIEVEMENTS;
        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    void test_options_back_returns_to_title() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 3;  // BACK
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
    }

    void test_options_right_increases_master_volume() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 0;  // MASTER VOLUME
        gs.master_volume_percent = 90;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.master_volume_percent, 100);
    }

    void test_options_left_clamps_master_volume_at_zero() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 0;
        gs.master_volume_percent = 0;

        InputState left;
        left.left_repeat = true;
        update_scene(gs, left);
        TS_ASSERT_EQUALS(gs.master_volume_percent, 0);
    }

    void test_options_right_increases_music_volume() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 1;  // MUSIC VOLUME
        gs.music_volume_percent = 50;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.music_volume_percent, 60);
    }

    void test_options_left_clamps_music_volume_at_zero() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 1;
        gs.music_volume_percent = 0;

        InputState left;
        left.left_repeat = true;
        update_scene(gs, left);
        TS_ASSERT_EQUALS(gs.music_volume_percent, 0);
    }

    void test_options_right_increases_sfx_volume() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 2;  // SFX VOLUME
        gs.sfx_volume_percent = 70;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.sfx_volume_percent, 80);
    }

    void test_options_left_clamps_sfx_volume_at_zero() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 2;
        gs.sfx_volume_percent = 0;

        InputState left;
        left.left_repeat = true;
        update_scene(gs, left);
        TS_ASSERT_EQUALS(gs.sfx_volume_percent, 0);
    }

    void test_options_right_clamps_sfx_volume_at_one_hundred() {
        GameState gs;
        gs.current_scene = Scene::OPTIONS;
        gs.menu_selection = 2;
        gs.sfx_volume_percent = 100;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.sfx_volume_percent, 100);
    }

    // ── UI SFX events ─────────────────────────────────────────────────────────

    void test_menu_navigation_queues_navigate_sfx() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);
        TS_ASSERT_EQUALS(gs.ui_sfx, UiSfx::NAVIGATE);
        TS_ASSERT_EQUALS(gs.ui_sfx_event_id, 1U);
    }

    void test_enter_on_title_queues_select_sfx() {
        GameState gs;
        gs.title_menu_revealed = true;
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.ui_sfx, UiSfx::SELECT);
        TS_ASSERT_EQUALS(gs.ui_sfx_event_id, 1U);
    }

    // ── Title menu reveal gate ────────────────────────────────────────────────

    void test_title_menu_starts_hidden() {
        GameState gs;
        TS_ASSERT(!gs.title_menu_revealed);
    }

    void test_first_keypress_on_title_reveals_menu() {
        GameState gs;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);
        TS_ASSERT(gs.title_menu_revealed);
    }

    void test_first_keypress_on_title_does_not_navigate_or_select() {
        GameState gs;
        // First press: down should reveal but not move selection from 0
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);
        TS_ASSERT_EQUALS(gs.menu_selection, 0);

        // First press scenario with enter: should reveal but not transition
        GameState gs2;
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs2, enter);
        TS_ASSERT(gs2.title_menu_revealed);
        TS_ASSERT_EQUALS(gs2.current_scene, Scene::TITLE);
        TS_ASSERT(!gs2.quit_requested);
    }

    void test_reveal_does_not_queue_sfx() {
        GameState gs;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);
        TS_ASSERT_EQUALS(gs.ui_sfx, UiSfx::NONE);
    }

    void test_subsequent_keypress_after_reveal_navigates_normally() {
        GameState gs;
        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);  // reveal
        update_scene(gs, dn);  // navigate
        TS_ASSERT_EQUALS(gs.menu_selection, 1);
    }

    void test_no_input_does_not_reveal_title_menu() {
        GameState gs;
        InputState idle;
        update_scene(gs, idle);
        TS_ASSERT(!gs.title_menu_revealed);
    }

    void test_reveal_persists_across_scene_transitions() {
        GameState gs;
        gs.title_menu_revealed = true;
        gs.current_scene = Scene::OPTIONS;
        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::TITLE);
        TS_ASSERT(gs.title_menu_revealed);
    }
};
