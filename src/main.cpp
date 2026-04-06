/** @file main.cpp
 *  @brief Application entry point and main game loop.
 *
 *  Responsibilities:
 *  - Open the raylib window and seed the RNG.
 *  - Run the game loop: poll input → update state → draw frame.
 *  - Gracefully close when the window is closed or quit is requested.
 *
 *  The game loop is intentionally thin: all logic lives in libinput.cpp and
 *  all rendering lives in libdraw.cpp.
 */

#include "config.h"
#include "gamestate.h"
#include "input_state.h"
#include "libdraw.h"
#include "libinput.h"
#include <cstdlib>
#include <ctime>
#include <raylib.h>

/**
 * @brief Poll raylib for this frame's input events.
 *
 * Translates raylib key queries into a platform-agnostic `InputState`
 * snapshot.  This is the *only* place raylib input functions are called.
 *
 * @return Populated `InputState` for the current frame.
 */
static InputState poll_input() {
    InputState in;
    in.up_pressed        = IsKeyPressed(KEY_UP);
    in.down_pressed      = IsKeyPressed(KEY_DOWN);
    in.left_pressed      = IsKeyPressed(KEY_LEFT);
    in.right_pressed     = IsKeyPressed(KEY_RIGHT);
    in.left_repeat       = IsKeyPressed(KEY_LEFT)  || IsKeyPressedRepeat(KEY_LEFT);
    in.right_repeat      = IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT);
    in.enter_pressed     = IsKeyPressed(KEY_ENTER)  || IsKeyPressed(KEY_KP_ENTER);
    in.space_pressed     = IsKeyPressed(KEY_SPACE);
    in.left_bracket_pressed  = IsKeyPressed(KEY_LEFT_BRACKET);
    in.right_bracket_pressed = IsKeyPressed(KEY_RIGHT_BRACKET);
    in.escape_pressed    = IsKeyPressed(KEY_ESCAPE);
    in.backspace_pressed = IsKeyPressed(KEY_BACKSPACE);
    in.char_pressed      = GetCharPressed();
    in.frame_seconds     = GetFrameTime();
    return in;
}

/**
 * @brief Application entry point.
 * @return 0 on clean exit.
 */
int main() {
    // Seed random number generator for character generation
    srand(static_cast<unsigned int>(time(nullptr)));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);
    init_draw_system();

    GameState gs;

    while (!WindowShouldClose() && !gs.quit_requested) {
        InputState in = poll_input();
        update_scene(gs, in);
        draw_scene(gs);
    }

    shutdown_draw_system();
    CloseWindow();
    return 0;
}
