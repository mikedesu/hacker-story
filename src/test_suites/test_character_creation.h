/** @file test_suites/test_character_creation.h
 *  @brief CxxTest suite for character creation flows (random and custom).
 */

#pragma once
#include <cxxtest/TestSuite.h>
#include "../gamestate.h"
#include "../gamedata.h"
#include "../input_state.h"
#include "../libinput.h"

/**
 * @brief Tests for random and custom character creation.
 */
class TestCharacterCreation : public CxxTest::TestSuite {
public:

    // ── random_player() ───────────────────────────────────────────────────────

    void test_random_player_name_not_empty() {
        srand(1);
        PlayerData p = random_player();
        TS_ASSERT(!p.name.empty());
    }

    void test_random_player_birth_year_lower_bound() {
        srand(1);
        for (int i = 0; i < 50; i++) {
            PlayerData p = random_player();
            TS_ASSERT_LESS_THAN_EQUALS(YEAR_MIN, p.year_of_birth);
        }
    }

    void test_random_player_birth_year_upper_bound() {
        srand(1);
        for (int i = 0; i < 50; i++) {
            PlayerData p = random_player();
            TS_ASSERT_LESS_THAN_EQUALS(p.year_of_birth, YEAR_MAX);
        }
    }

    void test_random_player_birth_month_in_range() {
        srand(1);
        for (int i = 0; i < 50; i++) {
            PlayerData p = random_player();
            TS_ASSERT_LESS_THAN_EQUALS(1, p.birth_month);
            TS_ASSERT_LESS_THAN_EQUALS(p.birth_month, 12);
        }
    }

    void test_random_player_birth_day_valid_for_month() {
        srand(1);
        for (int i = 0; i < 50; i++) {
            PlayerData p = random_player();
            TS_ASSERT_LESS_THAN_EQUALS(1, p.birth_day);
            TS_ASSERT_LESS_THAN_EQUALS(p.birth_day, days_in_month(p.year_of_birth, p.birth_month));
        }
    }

    void test_random_player_location_not_empty() {
        srand(1);
        PlayerData p = random_player();
        TS_ASSERT(!p.location.empty());
    }

    void test_random_player_location_is_in_list() {
        srand(42);
        PlayerData p = random_player();
        bool found = false;
        for (const auto& loc : ALL_LOCATIONS) {
            if (loc == p.location) { found = true; break; }
        }
        TS_ASSERT(found);
    }

    // ── Random character screen ───────────────────────────────────────────────

    void test_random_character_generated_on_first_update() {
        GameState gs;
        gs.current_scene         = Scene::RANDOM_CHARACTER;
        gs.random_char_generated = false;
        InputState empty;
        update_scene(gs, empty);
        TS_ASSERT(gs.random_char_generated);
        TS_ASSERT(!gs.player.name.empty());
    }

    void test_reroll_option_generates_new_character() {
        srand(1);
        GameState gs;
        gs.current_scene         = Scene::RANDOM_CHARACTER;
        gs.random_char_generated = false;

        // First update — generate
        InputState empty;
        update_scene(gs, empty);
        string first_name = gs.player.name;

        // Move selection to RE-ROLL (index 1) and press enter
        gs.menu_selection = 0;  // RE-ROLL is now index 0
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        // Player should still be on RANDOM_CHARACTER and have a (possibly different) name
        TS_ASSERT_EQUALS(gs.current_scene, Scene::RANDOM_CHARACTER);
        TS_ASSERT(!gs.player.name.empty());
    }

    void test_accept_transitions_to_gameplay() {
        srand(1);
        GameState gs;
        gs.current_scene         = Scene::RANDOM_CHARACTER;
        gs.random_char_generated = false;

        InputState empty;
        update_scene(gs, empty);  // generate

        gs.menu_selection = 1;    // ACCEPT is now index 1
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT_EQUALS(gs.current_scene, Scene::GAMEPLAY);
        TS_ASSERT(gs.new_game_started);
    }

    void test_accept_random_character_unlocks_being_born() {
        srand(1);
        GameState gs;
        gs.current_scene         = Scene::RANDOM_CHARACTER;
        gs.random_char_generated = false;

        InputState empty;
        update_scene(gs, empty);  // generate

        gs.menu_selection = 1;    // ACCEPT
        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN));
    }

    void test_back_from_random_returns_to_character_create() {
        GameState gs;
        gs.current_scene         = Scene::RANDOM_CHARACTER;
        gs.random_char_generated = true;
        gs.menu_selection        = 2;  // BACK is still index 2

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::CHARACTER_CREATE);
    }

    // ── Custom character: name field ──────────────────────────────────────────

    void test_typing_appends_to_text_input() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 0;
        gs.text_input.clear();

        InputState in;
        in.char_pressed = 'H';
        update_scene(gs, in);
        TS_ASSERT_EQUALS(gs.text_input, "H");

        in.char_pressed = 'i';
        update_scene(gs, in);
        TS_ASSERT_EQUALS(gs.text_input, "Hi");
    }

    void test_backspace_removes_last_char() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 0;
        gs.text_input       = "ABC";

        InputState in;
        in.backspace_pressed = true;
        update_scene(gs, in);
        TS_ASSERT_EQUALS(gs.text_input, "AB");
    }

    void test_backspace_on_empty_input_is_safe() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 0;
        gs.text_input.clear();

        InputState in;
        in.backspace_pressed = true;
        update_scene(gs, in);  // must not crash
        TS_ASSERT_EQUALS(gs.text_input, "");
    }

    void test_name_max_length_24() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 0;
        gs.text_input       = string(24, 'X');

        InputState in;
        in.char_pressed = 'Y';
        update_scene(gs, in);
        TS_ASSERT_EQUALS((int)gs.text_input.size(), 24);  // not 25
    }

    // ── Custom character: year field ──────────────────────────────────────────

    void test_enter_advances_from_name_to_year() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 0;

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);
        TS_ASSERT_EQUALS(gs.custom_char_field, 1);
    }

    void test_right_advances_year_selection() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 1;
        gs.year_selection   = 0;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.year_selection, 1);
    }

    void test_year_selection_wraps_at_start() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 1;
        gs.year_selection   = 0;

        InputState left;
        left.left_repeat = true;
        update_scene(gs, left);
        TS_ASSERT_EQUALS(gs.year_selection, YEAR_COUNT - 1);
    }

    void test_year_field_up_navigates_to_name_field() {
        GameState gs;
        gs.current_scene     = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 1;

        InputState up;
        up.up_pressed = true;
        update_scene(gs, up);
        TS_ASSERT_EQUALS(gs.custom_char_field, 0);
    }

    void test_year_field_down_navigates_to_month_field() {
        GameState gs;
        gs.current_scene     = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 1;

        InputState dn;
        dn.down_pressed = true;
        update_scene(gs, dn);
        TS_ASSERT_EQUALS(gs.custom_char_field, 2);
    }

    // ── Custom character: month/day/location fields ─────────────────────────

    void test_right_advances_birth_month() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 2;
        gs.month_selection    = 1;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.month_selection, 2);
    }

    void test_month_change_clamps_invalid_birth_day() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 2;
        gs.year_selection     = 101; // 2001, not leap year
        gs.month_selection    = 3;   // March
        gs.day_selection      = 31;

        InputState left;
        left.left_repeat = true;      // February
        update_scene(gs, left);
        TS_ASSERT_EQUALS(gs.month_selection, 2);
        TS_ASSERT_EQUALS(gs.day_selection, 28);
    }

    void test_right_advances_birth_day() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 3;
        gs.year_selection     = 0;
        gs.month_selection    = 1;
        gs.day_selection      = 1;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.day_selection, 2);
    }

    void test_birth_day_wraps_at_end_of_month() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 3;
        gs.year_selection     = 0;
        gs.month_selection    = 2;
        gs.day_selection      = 28;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.day_selection, 1);
    }

    void test_right_advances_location() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 4;
        gs.location_selection = 0;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.location_selection, 1);
    }

    void test_location_selection_wraps_at_end() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 4;
        gs.location_selection = unlocked_location_count() - 1;

        InputState right;
        right.right_repeat = true;
        update_scene(gs, right);
        TS_ASSERT_EQUALS(gs.location_selection, 0);  // wraps
    }

    void test_location_field_up_navigates_to_year_field() {
        GameState gs;
        gs.current_scene     = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 4;

        InputState up;
        up.up_pressed = true;
        update_scene(gs, up);
        TS_ASSERT_EQUALS(gs.custom_char_field, 3);
    }

    // ── Custom character: confirm ─────────────────────────────────────────────

    void test_confirm_sets_player_name() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 5;  // START GAME button
        gs.text_input       = "Zara Null";
        gs.year_selection   = 3;
        gs.month_selection  = 12;
        gs.day_selection    = 25;
        gs.location_selection = 1;

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT_EQUALS(gs.player.name, "Zara Null");
        TS_ASSERT_EQUALS(gs.player.birth_month, 12);
        TS_ASSERT_EQUALS(gs.player.birth_day, 25);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::GAMEPLAY);
        TS_ASSERT(gs.new_game_started);
    }

    void test_confirm_sets_fixed_poor_conditions() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 5;  // START GAME button
        gs.text_input       = "Test";

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT_EQUALS(gs.player.wealth, Wealth::POOR);
        TS_ASSERT_EQUALS(gs.player.health, Health::SICKLY);
        TS_ASSERT_EQUALS(gs.player.talent, Talent::SLOW);
        TS_ASSERT_EQUALS(gs.player.env,    Environment::LOWER_CLASS);
    }

    void test_custom_character_unlocks_being_born() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;  // START GAME button
        gs.text_input         = "Nix";
        gs.year_selection     = 70; // 1970

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN));
    }

    void test_custom_character_before_1970_unlocks_pre_unix_epoch() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;  // START GAME button
        gs.text_input         = "Ada";
        gs.year_selection     = 0;  // 1900

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN));
        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN_BEFORE_UNIX_EPOCH));
    }

    void test_custom_character_before_2000_unlocks_pre_y2k() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;   // START GAME button
        gs.text_input         = "Neo";
        gs.year_selection     = 99;  // 1999

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN_BEFORE_Y2K));
    }

    void test_custom_character_before_2038_unlocks_pre_2038() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;    // START GAME button
        gs.text_input         = "Future";
        gs.year_selection     = 137;  // 2037

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN_BEFORE_2038));
    }

    void test_custom_character_in_1970_does_not_unlock_pre_unix_epoch() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;   // START GAME button
        gs.text_input         = "Epoch";
        gs.year_selection     = 70;  // 1970

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(achievement_unlocked(gs, AchievementId::BORN));
        TS_ASSERT(!achievement_unlocked(gs, AchievementId::BORN_BEFORE_UNIX_EPOCH));
    }

    void test_custom_character_in_2000_does_not_unlock_pre_y2k() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;    // START GAME button
        gs.text_input         = "Millie";
        gs.year_selection     = 100;  // 2000

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(!achievement_unlocked(gs, AchievementId::BORN_BEFORE_Y2K));
    }

    void test_custom_character_in_2038_does_not_unlock_pre_2038() {
        GameState gs;
        gs.current_scene      = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field  = 5;    // START GAME button
        gs.text_input         = "Epochal";
        gs.year_selection     = 138;  // 2038

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT(!achievement_unlocked(gs, AchievementId::BORN_BEFORE_2038));
    }

    void test_random_player_has_environment() {
        srand(1);
        // Roll many times and verify env is always a valid value
        for (int i = 0; i < 50; i++) {
            PlayerData p = random_player();
            bool valid = (p.env == Environment::LOWER_CLASS  ||
                          p.env == Environment::MIDDLE_CLASS ||
                          p.env == Environment::UPPER_CLASS);
            TS_ASSERT(valid);
        }
    }

    void test_env_to_str_all_values() {
        TS_ASSERT_EQUALS(env_to_str(Environment::LOWER_CLASS),  "Lower Class");
        TS_ASSERT_EQUALS(env_to_str(Environment::MIDDLE_CLASS), "Middle Class");
        TS_ASSERT_EQUALS(env_to_str(Environment::UPPER_CLASS),  "Upper Class");
    }

    void test_empty_name_defaults_to_anonymous() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 5;  // START GAME button
        gs.text_input.clear();

        InputState enter;
        enter.enter_pressed = true;
        update_scene(gs, enter);

        TS_ASSERT_EQUALS(gs.player.name, "Anonymous");
    }

    // ── Custom character: ESC navigation ─────────────────────────────────────

    void test_esc_on_field_1_goes_back_to_field_0() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 1;

        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.custom_char_field, 0);
    }

    void test_esc_on_field_0_exits_to_character_create() {
        GameState gs;
        gs.current_scene    = Scene::CUSTOM_CHARACTER;
        gs.custom_char_field = 0;

        InputState esc;
        esc.escape_pressed = true;
        update_scene(gs, esc);
        TS_ASSERT_EQUALS(gs.current_scene, Scene::CHARACTER_CREATE);
    }
};
