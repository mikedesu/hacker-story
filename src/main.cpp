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

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

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
    in.escape_pressed    = IsKeyPressed(KEY_ESCAPE);
    in.backspace_pressed = IsKeyPressed(KEY_BACKSPACE);
    in.char_pressed      = GetCharPressed();
    in.frame_seconds     = GetFrameTime();
    return in;
}

/// File-scope pointer to the live GameState so the per-frame callback used by
/// `emscripten_set_main_loop` can reach it without an arg-passing variant.
static GameState* g_gs = nullptr;

/**
 * @brief Run a single iteration of the game loop.
 *
 * Extracted into its own function so the same body drives both the desktop
 * `while` loop and the emscripten per-frame callback under `PLATFORM_WEB`.
 */
static void update_draw_frame() {
    InputState in = poll_input();
    update_scene(*g_gs, in);
    draw_scene(*g_gs);
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
    init_draw_system();

    GameState gs;
    g_gs = &gs;

#if defined(PLATFORM_WEB)
    // Browser drives the frame cadence via requestAnimationFrame.
    // Intentionally do NOT call SetTargetFPS on web — raylib's WaitTime path
    // routes through emscripten_sleep, which would require Asyncify.
    emscripten_set_main_loop(update_draw_frame, 0, 1);
#else
    SetTargetFPS(TARGET_FPS);
    while (!WindowShouldClose() && !gs.quit_requested) {
        update_draw_frame();
    }
    shutdown_draw_system();
    CloseWindow();
#endif
    return 0;
}
