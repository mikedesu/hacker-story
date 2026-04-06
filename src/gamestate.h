/** @file gamestate.h
 *  @brief Core game-state struct and player-data sub-structs.
 *
 *  `GameState` is the single source of truth passed (by reference) to every
 *  update and draw function.  No raylib types are used here so this header
 *  can be included in unit tests compiled without a display.
 */

#pragma once

#include "scene.h"
#include <array>
#include <string>
#include <vector>

using std::array;
using std::string;
using std::vector;

// ── Genetic conditions ────────────────────────────────────────────────────────

/// @brief Innate wealth disposition — genetic luck of the draw.
enum class Wealth  { POOR, RICH };
/// @brief Innate constitution — genetic health baseline.
enum class Health  { SICKLY, HEALTHY };
/// @brief Innate cognitive aptitude — genetic talent baseline.
enum class Talent  { SLOW, TALENTED };

// ── Environmental conditions ──────────────────────────────────────────────────

/// @brief Socioeconomic class of the household the character was raised in.
enum class Environment { LOWER_CLASS, MIDDLE_CLASS, UPPER_CLASS };

/// @brief UI sound-effect request emitted by scene logic.
enum class UiSfx { NONE, NAVIGATE, SELECT };

/// @brief Enumerates all achievements currently supported by the game.
enum class AchievementId {
    BORN = 0,                      ///< Awarded when a new life begins
    BORN_BEFORE_UNIX_EPOCH,        ///< Awarded for births before January 1, 1970
    SURVIVED_UNIX_EPOCH,           ///< Awarded for living long enough to reach the Unix epoch
    BORN_BEFORE_Y2K,               ///< Awarded for births before January 1, 2000
    SURVIVED_Y2K,                  ///< Awarded for living long enough to reach Y2K
    BORN_BEFORE_2038,              ///< Awarded for births before January 1, 2038
    SURVIVED_2038,                 ///< Awarded for living long enough to reach the year 2038
};

/// Total number of achievement definitions.
static constexpr int ACHIEVEMENT_COUNT = 7;
/// Seconds an achievement popup remains visible before advancing.
static constexpr float ACHIEVEMENT_POPUP_DURATION = 4.5f;

/**
 * @brief Simple calendar/date snapshot used by the gameplay clock.
 */
struct GameDateTime {
    int year{1900};                  ///< Four-digit year
    int month{1};                    ///< Month number [1, 12]
    int day{1};                      ///< Day of month [1, 31]
    int hour{0};                     ///< Hour of day [0, 23]
};

/**
 * @brief Convert an `Environment` value to a display string.
 * @param e Environment value.
 * @return Human-readable label.
 */
static inline std::string env_to_str(Environment e) {
    switch (e) {
    case Environment::LOWER_CLASS:  return "Lower Class";
    case Environment::MIDDLE_CLASS: return "Middle Class";
    case Environment::UPPER_CLASS:  return "Upper Class";
    }
    return "Unknown";
}

/**
 * @brief Convert an achievement id to a stable array index.
 * @param id Achievement identifier.
 * @return Zero-based achievement index.
 */
static inline int achievement_index(AchievementId id) {
    return static_cast<int>(id);
}

/**
 * @brief Return the display title for an achievement.
 * @param id Achievement identifier.
 * @return Short title string.
 */
static inline const char* achievement_title(AchievementId id) {
    switch (id) {
    case AchievementId::BORN:                   return "Being Born";
    case AchievementId::BORN_BEFORE_UNIX_EPOCH: return "Born Before The Unix Epoch";
    case AchievementId::SURVIVED_UNIX_EPOCH:    return "I Survived The Unix Epoch";
    case AchievementId::BORN_BEFORE_Y2K:        return "Born Before Y2K";
    case AchievementId::SURVIVED_Y2K:           return "I Survived Y2K";
    case AchievementId::BORN_BEFORE_2038:       return "Born Before 2038";
    case AchievementId::SURVIVED_2038:          return "I Survived 2038";
    }
    return "Unknown Achievement";
}

/**
 * @brief Return the flavor description for an achievement.
 * @param id Achievement identifier.
 * @return Descriptive achievement text.
 */
static inline const char* achievement_description(AchievementId id) {
    switch (id) {
    case AchievementId::BORN:
        return "Congratulations, you were born.";
    case AchievementId::BORN_BEFORE_UNIX_EPOCH:
        return "You entered the world before January 1, 1970.";
    case AchievementId::SURVIVED_UNIX_EPOCH:
        return "You lived long enough to reach January 1, 1970.";
    case AchievementId::BORN_BEFORE_Y2K:
        return "You were already here before January 1, 2000.";
    case AchievementId::SURVIVED_Y2K:
        return "You lived long enough to reach January 1, 2000.";
    case AchievementId::BORN_BEFORE_2038:
        return "You were already here before January 1, 2038.";
    case AchievementId::SURVIVED_2038:
        return "You lived long enough to reach January 1, 2038.";
    }
    return "No description.";
}

/**
 * @brief Return whether the supplied year is a leap year.
 * @param year Four-digit year.
 * @return `true` for leap years, otherwise `false`.
 */
static inline bool is_leap_year(int year) {
    return (year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0));
}

/**
 * @brief Return the number of days in a specific month of a specific year.
 * @param year  Four-digit year.
 * @param month Month number [1, 12].
 * @return Number of days in the requested month.
 */
static inline int days_in_month(int year, int month) {
    static const int MONTH_LENGTHS[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    if (month == 2 && is_leap_year(year)) return 29;
    if (month < 1 || month > 12) return 30;
    return MONTH_LENGTHS[month - 1];
}

/// Slowest allowed gameplay time scale: 1 real second per in-game hour.
static constexpr float GAMEPLAY_SECONDS_PER_HOUR_MAX = 1.0f;
/// Fastest allowed gameplay time scale: 1 real second per 365 in-game days.
static constexpr float GAMEPLAY_SECONDS_PER_HOUR_MIN = 1.0f / 8760.0f;

/**
 * @brief Convert elapsed gameplay hours into an in-game calendar date.
 * @param start_year Starting year of the campaign.
 * @param elapsed_hours Hours elapsed since the start of the campaign.
 * @return Calendar snapshot representing the in-game date and hour.
 */
static inline GameDateTime game_datetime_from_hours(int start_year, int start_month,
                                                    int start_day, long long elapsed_hours) {
    GameDateTime dt;
    dt.year = start_year;
    dt.month = start_month;
    dt.day = start_day;

    long long remaining_days = elapsed_hours / 24;
    dt.hour = static_cast<int>(elapsed_hours % 24);

    while (remaining_days > 0) {
        const int month_days = days_in_month(dt.year, dt.month);
        const int days_left_in_month = month_days - dt.day;
        if (remaining_days <= days_left_in_month) {
            dt.day += static_cast<int>(remaining_days);
            remaining_days = 0;
        } else {
            remaining_days -= (days_left_in_month + 1);
            dt.day = 1;
            dt.month++;
            if (dt.month > 12) {
                dt.month = 1;
                dt.year++;
            }
        }
    }
    return dt;
}

// ── Player character data ─────────────────────────────────────────────────────

/**
 * @brief All persistent data describing the player character.
 */
struct PlayerData {
    string      name;                              ///< Character full name
    int         year_of_birth{1975};               ///< Birth year (e.g. 1975)
    int         birth_month{1};                    ///< Birth month [1, 12]
    int         birth_day{1};                      ///< Birth day within the birth month
    string      location;                          ///< Starting location on Earth

    // Genetic conditions
    Wealth      wealth{Wealth::POOR};              ///< Innate wealth disposition
    Health      health{Health::SICKLY};            ///< Innate health constitution
    Talent      talent{Talent::SLOW};              ///< Innate cognitive aptitude

    // Environmental conditions
    Environment env{Environment::LOWER_CLASS};     ///< Socioeconomic upbringing

    // Finances
    float cash{0.0f};                              ///< Liquid cash on hand (USD)
};

// ── Global game state ─────────────────────────────────────────────────────────

/**
 * @brief Global game state shared across all scenes.
 *
 * Scene transitions are performed by writing to `current_scene`.  The previous
 * scene is saved to `previous_scene` to support ESC / back navigation.
 * All menu navigation indices are reset to 0 on transition.
 */
struct GameState {
    // Scene management
    Scene current_scene{Scene::TITLE};   ///< Currently active scene
    Scene previous_scene{Scene::TITLE};  ///< Scene before the last transition

    // Player data (populated during character creation)
    PlayerData player;                   ///< Player character

    // Menu navigation
    int  menu_selection{0};              ///< Index of the highlighted menu option
    bool quit_requested{false};          ///< Set true to exit the application
    bool new_game_started{false};        ///< True once character creation is confirmed

    // User options
    int  master_volume_percent{100};     ///< Global output volume [0, 100]
    int  music_volume_percent{50};       ///< Music playback volume [0, 100]
    int  sfx_volume_percent{70};         ///< UI / SFX playback volume [0, 100]
    float seconds_per_game_hour{1.0f / 24.0f};   ///< Real seconds required for one in-game hour

    // Custom character creation transient state
    int    custom_char_field{0};         ///< Active field index: 0=name, 1=year, 2=month, 3=day, 4=location
    string text_input;                   ///< Live text buffer for name entry
    int    year_selection{0};            ///< Index into CUSTOM_YEARS list
    int    month_selection{1};           ///< Birth month [1, 12]
    int    day_selection{1};             ///< Birth day within the selected month/year
    int    location_selection{0};        ///< Index into available locations list

    // Random character creation transient state
    bool random_char_generated{false};   ///< True once a random character has been rolled

    // Gameplay time progression
    bool gameplay_paused{false};         ///< True when gameplay time is paused
    float gameplay_time_accumulator{0.0f}; ///< Real-time accumulator toward the next game hour
    long long elapsed_game_hours{0};     ///< Hours elapsed since the campaign began
    float play_session_seconds{0.0f};    ///< Real-time duration spent in this gameplay run

    // Achievement progression
    array<bool, ACHIEVEMENT_COUNT> achievements_unlocked{}; ///< Session-local achievement unlock state
    vector<AchievementId> achievement_popup_queue; ///< Pending achievement popups to show during gameplay
    bool achievement_popup_visible{false}; ///< True while an achievement modal is active
    AchievementId active_achievement_popup{AchievementId::BORN}; ///< Currently displayed achievement popup
    float achievement_popup_seconds_remaining{0.0f}; ///< Time left on the active popup modal

    // News event feed
    int          next_news_event_idx{0}; ///< Index of the next unchecked event in ALL_EVENTS
    vector<int>  news_feed;              ///< Triggered event indices, oldest first (capped at 30)

    // One-shot UI audio event emitted by update logic and consumed by renderer
    UiSfx        ui_sfx{UiSfx::NONE};    ///< Most recent queued UI sound effect
    unsigned int ui_sfx_event_id{0};     ///< Monotonic event id for edge-triggered playback
};

/**
 * @brief Return the current in-game date/time for the supplied game state.
 * @param gs Current game state.
 * @return Calendar snapshot derived from the player's birth year and elapsed time.
 */
static inline GameDateTime gameplay_datetime(const GameState& gs) {
    return game_datetime_from_hours(gs.player.year_of_birth,
                                    gs.player.birth_month,
                                    gs.player.birth_day,
                                    gs.elapsed_game_hours);
}

/**
 * @brief Return the player's current age in years.
 * @param gs Current game state.
 * @return Whole years elapsed since the player's birth year.
 */
static inline int gameplay_age_years(const GameState& gs) {
    const GameDateTime dt = gameplay_datetime(gs);
    int age = dt.year - gs.player.year_of_birth;
    if (dt.month < gs.player.birth_month
        || (dt.month == gs.player.birth_month && dt.day < gs.player.birth_day)) {
        age--;
    }
    return age;
}

/**
 * @brief Return whether an achievement has already been unlocked.
 * @param gs Current game state.
 * @param id Achievement identifier.
 * @return `true` when the achievement is unlocked.
 */
static inline bool achievement_unlocked(const GameState& gs, AchievementId id) {
    return gs.achievements_unlocked[achievement_index(id)];
}

/**
 * @brief Return how many achievements have been unlocked in this session.
 * @param gs Current game state.
 * @return Number of unlocked achievements.
 */
static inline int achievement_unlock_count(const GameState& gs) {
    int count = 0;
    for (bool unlocked : gs.achievements_unlocked) {
        if (unlocked) count++;
    }
    return count;
}
