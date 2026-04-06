/** @file libdraw.h
 *  @brief Scene rendering public interface.
 *
 *  `draw_scene()` is the single entry point called each frame by the game
 *  loop.  It calls `BeginDrawing()` / `EndDrawing()` internally and dispatches
 *  to the appropriate per-scene draw handler based on
 *  `GameState::current_scene`.
 *
 *  All raylib calls are confined to libdraw.cpp; callers only need this header.
 */

#pragma once

#include "gamestate.h"

/**
 * @brief Initialize rendering-adjacent resources owned by libdraw.
 *
 * Currently this sets up the audio device and loads UI sound effects.
 * Safe to call once after the window has been created.
 */
void init_draw_system();

/**
 * @brief Render one frame for the currently active scene.
 *
 * Calls BeginDrawing(), clears the background, renders the active scene,
 * then calls EndDrawing().  Must be called after `update_scene()` each frame.
 *
 * @param gs  Const reference to the global game state (draw never mutates state).
 */
void draw_scene(const GameState& gs);

/**
 * @brief Release rendering-adjacent resources owned by libdraw.
 *
 * Safe to call during shutdown before the window is closed.
 */
void shutdown_draw_system();
