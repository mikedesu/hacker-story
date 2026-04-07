/** @file input_state.h
 *  @brief Platform-agnostic snapshot of player input for one frame.
 *
 *  Decouples game logic from raylib so scenes can be updated and tested
 *  without an active display or window.  In normal play the game loop
 *  populates this from raylib; in unit tests a harness fills it directly.
 */

#pragma once

/**
 * @brief One-frame snapshot of relevant input events.
 *
 * Populated by `poll_input()` in main.cpp each frame, then passed
 * by value into `update_scene()` so all game logic remains raylib-free.
 */
struct InputState {
    bool up_pressed{false};          ///< UP arrow — fires once on press
    bool down_pressed{false};        ///< DOWN arrow — fires once on press
    bool left_pressed{false};        ///< LEFT arrow — fires once on press
    bool right_pressed{false};       ///< RIGHT arrow — fires once on press
    bool left_repeat{false};         ///< LEFT arrow — fires on press and while held (key repeat)
    bool right_repeat{false};        ///< RIGHT arrow — fires on press and while held (key repeat)
    bool enter_pressed{false};       ///< ENTER / KP_ENTER
    bool space_pressed{false};       ///< SPACEBAR
    bool escape_pressed{false};      ///< ESCAPE
    bool backspace_pressed{false};   ///< BACKSPACE
    int  char_pressed{0};            ///< Unicode codepoint from GetCharPressed() (0 = none)
    float frame_seconds{0.0f};       ///< Real seconds elapsed since the previous frame
};
