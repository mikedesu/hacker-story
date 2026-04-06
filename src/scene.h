/** @file scene.h
 *  @brief Scene/screen identifiers for the scene management system.
 *
 *  The game loop dispatches input, update, and draw calls based on the value
 *  of `GameState::current_scene`.  Adding a new screen means: add an enum
 *  value here, add update logic in libinput.cpp, add draw logic in libdraw.cpp.
 */

#pragma once

/**
 * @brief Identifies which screen is currently active.
 */
enum class Scene {
    TITLE,             ///< Title screen — New Game / Achievements / Options / Quit
    ACHIEVEMENTS,      ///< Achievement browser / summary screen
    OPTIONS,           ///< Options/configuration menu
    CHARACTER_CREATE,  ///< Choose between random or custom character creation
    RANDOM_CHARACTER,  ///< View / accept / re-roll a randomly generated character
    CUSTOM_CHARACTER,  ///< Manually enter name, birth year, and starting location
    GAMEPLAY,          ///< Main gameplay loop (stub — future work)
};
