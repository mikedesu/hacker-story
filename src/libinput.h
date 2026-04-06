/** @file libinput.h
 *  @brief Scene update / input-processing public interface.
 *
 *  `update_scene()` is the single entry point called each frame by the game
 *  loop.  It dispatches to the appropriate per-scene handler based on
 *  `GameState::current_scene`.
 *
 *  All functions here are raylib-free; they receive input via `InputState`
 *  and mutate only `GameState`.  This makes every scene handler unit-testable
 *  without a display.
 */

#pragma once

#include "gamestate.h"
#include "input_state.h"

/**
 * @brief Process one frame of input and advance the game state.
 *
 * Dispatches to the active scene's update handler.  Scene transitions are
 * performed by writing to `GameState::current_scene` inside the handlers.
 *
 * @param gs  Mutable reference to the global game state.
 * @param in  Immutable input snapshot for this frame.
 */
void update_scene(GameState& gs, const InputState& in);
