# Hacker Story

Hacker Story is a cyberpunk life-sim and management game written in C++ with raylib.

The project is inspired by the structure and long-term progression of *Game Dev Story*, but shifted into the world of computers, hacking, software, historical tech culture, and life choices. The current game already supports a complete front-end flow from title screen through character creation into the first playable slice of the life simulation.

## Current direction

The long-term goal is a full life-simulation game where the player grows from birth through childhood, work, technology access, money pressure, historical computing events, and eventually bigger decisions around software, hacking, business, and personal life.

Right now, the playable foundation includes:

- title, achievements, and options screens
- random and custom character creation
- birth-date-aware simulation start
- shared animated cyberpunk background across every scene
- main gameplay HUD with date, timer, pause state, time scale, cash, gold price, work state, wellbeing, and computer state
- historical computing news events with a marquee and side feed
- first job-search and grocery-store work loop
- first personal computer purchase
- persistent home-computer use preference that drives daily skill growth
- desktop and web builds
- unit tests for core game-state and gameplay systems

## Tech stack

- C++
- raylib
- make
- CxxTest
- Doxygen
- Emscripten for the web build

## Build

All build targets live in [`src/Makefile`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/Makefile), so run commands from `src/`.

### Desktop build

```bash
cd src
make game
./game
```

### Web build

```bash
cd src
make web
```

This generates `index.html`, `index.js`, `index.wasm`, and `index.data` in `src/`.

### Tests

```bash
cd src
make tests
./tests
```

### Docs

```bash
cd src
make docs
```

Generated API docs are written under `src/docs/api/`.

## Dependencies

The current Makefile expects:

- `clang++` for the main game build
- `g++` for the generated CxxTest runner
- raylib headers and libraries installed locally
- `cxxtestgen` available on `PATH`
- `doxygen` available on `PATH`
- Emscripten installed for `make web`

The web target is currently wired for a local raylib web archive and a local Emscripten setup, so it may need path adjustments on a new machine.

## Controls

This is a keyboard-driven game. The current input model is intentionally simple:

- arrow keys for menu navigation and gameplay time control
- `Enter` to confirm selections
- `Space` for scene/gameplay actions where applicable
- `Escape` to back out or pause depending on context

On the title screen, the first keypress only reveals the menu. It does not also navigate or select.

## Project structure

- [`src/main.cpp`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/main.cpp): window setup, input polling, frame loop
- [`src/libinput.cpp`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/libinput.cpp): scene updates and gameplay logic
- [`src/libdraw.cpp`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/libdraw.cpp): all raylib rendering
- [`src/gamestate.h`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/gamestate.h): core simulation state and gameplay data structures
- [`src/events.h`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/events.h): historical computing event data
- [`src/test_suites`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/test_suites): gameplay and state tests
- [`src/PLAN.md`](/home/darkmage/src/mikedesu/cpp/hacker-story/src/PLAN.md): living project plan and roadmap

## Design rules

Two implementation rules matter more than the rest:

1. Game logic stays free of raylib so it can be tested without a display.
2. Rendering stays concentrated in the draw layer instead of leaking into gameplay code.

That separation is what makes the project easy to test and iterate on.

## Roadmap

Planned systems include:

- deeper education and skill progression
- more jobs, promotions, and income paths
- software and product development loops
- hardware ownership and upgrades
- cryptocurrency systems
- relationships, housing, and long-term life events
- catastrophes and setbacks that can derail progression

## License

MIT. See [`LICENSE`](/home/darkmage/src/mikedesu/cpp/hacker-story/LICENSE).
