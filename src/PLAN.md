# PLAN.md

**DO NOT DELETE BELOW THIS LINE**

It is time for a new game project.

Our initial goal is simple:

An homage to a game called "Game Dev Story" by Cairosoft.

Our's will be "Hacker Story", or working title. 

The framework that we will be using to build the game will be:

- raylib
- C++
- make

I have previous experience building games with you using this framework and it has worked out great so far!

What we are doing here is FAR less ambitious than my other game, Project.RPG (a 2D roguelike with pixel-art graphics).

The initial core-loop is centered around text-based decision making, which will impact what path your character and world develop.

Current project status: the game framework and character creation flow are complete and stable, and the project is now itch.io-ready on both desktop and web. The animated cyberpunk background — scrolling perspective grid + twinkling star field — is now shared across **every** scene (title, achievements, options, character creation flows, gameplay), giving the whole game a consistent identity. The gameplay screen layers on top: a top marquee news ticker, a right-side news feed panel, a centered semi-transparent player display pane, a live HUD (name, age, date, session timer, pause state, time scale, cash, gold price, work state, wellbeing, computer state), and a fading welcome overlay. The title screen has been re-laid out as a centered, dramatic reveal: HACKER STORY (font 70) sits visually centered with the subtitle below, and the four menu options (NEW GAME / ACHIEVEMENTS / OPTIONS / QUIT) are *invisible* until the player makes their first keypress of the session — that initial press only reveals the menu, it does not also navigate or select. Once revealed, the menu stays revealed for the rest of the session. Gameplay time-scale is now driven by the LEFT/RIGHT arrow keys (the old `[`/`]` bindings have been removed entirely, including the dead `InputState` fields). The web build (`make web`) uses `emscripten_set_main_loop` directly in `main.cpp` rather than Asyncify, which keeps the wasm binary at ~575K and gives the browser native vsync via requestAnimationFrame. The news event catalogue covers 61 historical computing events from 1936 to 2038 with full year/month/day precision, birth-date filtered so players only see events from their own lifetime. Gold price is tracked historically via interpolated annual data from 1900–2050. The first real life-sim loop is live: at age 16 the player can start job-hunting, daily rolls can produce a grocery-store clerk offer, weekday shifts accumulate, Friday paydays add cash, attendance is modulated by mental/physical health, the player can buy a first used home computer for $300, and home-computer use is now a persistent preference (Practice / Hacking / Gaming / Social Media / Programming) that drives daily skill growth until the player changes it. All 163 unit tests pass (`-Wall -Werror` clean, including 7 new title-menu reveal-gate tests). The long-term vision is a full life-simulation management game where the player grows from birth through childhood, career, and beyond — shaped by historical computing milestones, personal decisions, financial state, and random life events.

Before we can really get into the gameplay, first we must build things such as "Scene Management", "Gamestate Management", be able to select between different scenes that all have access to a global gamestate, separate the logic of the game from the rendering of it (so we can optionally play the game in text-only mode, which could be VERY useful for automating debugging), and other "Game Framework" essential tools to make building the more fun aspects...more fun.

The general scene work-flow should start like this:

- Use the default Raylib font for ALL text rendering
- Frame renders should be done based on current scene / screen
- Game loop:
  - Receive input
  - Update gamestate
  - Draw scene
- Once we can control scene/screen management, adding new screens is easy.
- Information is passed between screens/scenes through the gamestate

## Presentation direction

- The target game identity is **cyberpunk / techno / management sim**, not medieval fantasy.
- Starter assets now exist in `img/` and `audio/` and should be treated as a **working source library**:
  - many of the current files come from a roguelike fantasy project
  - some can be reused as placeholders or adapted into cyberpunk equivalents
  - some, especially in `audio/`, are likely temporary and should not be treated as final canon
- We should start integrating textures, sound effects, and music soon, but do so in a way that supports future swaps without code churn.
- Shaders are explicitly encouraged. We are not avoiding them; we should use them intentionally for UI atmosphere, screen treatment, transitions, post-processing, and any presentation layer that helps sell the setting.
- Shader/resource management must stay disciplined: load, own, update, and unload rendering resources in a clear, centralized way instead of scattering them through gameplay logic.

## Checklist

### Pre-gameplay scaffolding
- [x] Scene management system
- [x] GameState management (no raylib dependency — fully testable)
- [x] Input/update/draw separation (`InputState` abstraction enables text-only/test mode)
- [x] Title Screen — NEW GAME / ACHIEVEMENTS / OPTIONS / QUIT
- [x] Achievements Screen — title-menu browser for unlocked/locked achievements
- [x] Options Screen — MASTER / MUSIC / SFX volume + BACK
- [x] Character Creation Screen — Random / Customize choice
- [x] Random Character Creation Screen — RE-ROLL / ACCEPT / BACK
- [x] Custom Character Creation Screen — name, birth year, birth month, birth day, location, START GAME / BACK
- [x] Genetic conditions — Poor/Rich, Sickly/Healthy, Slow/Talented (random or locked worst for custom)
- [x] Environmental conditions — Lower/Middle/Upper Class (random or locked Lower Class for custom)
- [x] Birth year range 1900–2050 (era-spanning; impacts future gameplay)
- [x] 15 starting locations, identical pool for both random and custom paths
- [x] Desktop window is resizable; fixed 1920×1080 render target stretches to current window size
- [x] Build system — desktop (`make game`), web (`make web`), tests (`make tests`), docs (`make docs`)
- [x] Web build is itch.io-ready: `main.cpp` uses `emscripten_set_main_loop` directly under `PLATFORM_WEB` (no Asyncify; wasm ~575K; browser-driven vsync via requestAnimationFrame)
- [x] 163 CxxTest unit tests, all passing, `-Wall -Werror` clean
- [x] Starter asset folders populated: `img/`, `audio/`, `shaders/` are in workspace and preloaded by current `Makefile`
- [x] Animated cyberpunk background (perspective grid + star field) is shared across **every** scene via a single call site in `draw_scene_to_target()` — title, achievements, options, character creation flows, and gameplay all share the same atmosphere
- [x] Title screen reveal gate: menu items are invisible until the player's first keypress of the session; that initial press only reveals (does not navigate or select); reveal is session-sticky across scene transitions
- [x] Title screen layout: HACKER STORY centered (font 70), subtitle below, four menu options centered horizontally below the subtitle

### Gameplay — foundations started
- [x] Main gameplay screen / HUD (player name, age, date, session timer, pause, time-scale, cash, gold price)
- [x] Time/calendar system (game starts on the player birthday; 1-hour simulation step; pause/unpause; adjustable scale)
- [x] News event system — 61 historical computing events (1936–2038), each with year/month/day; scrolling marquee + right-side feed panel; birth-date filtered for both marquee and panel
- [x] Player finances first pass — `cash` field on `PlayerData`; shown on HUD; all characters start at $0
- [x] Historical gold price tracker — interpolated from annual data 1900–2050; displayed on HUD in gold color as a real-world economic compass
- [x] Perspective grid + star field background — now drawn behind **every** scene from `draw_scene_to_target()`; on the gameplay scene the animation freezes when paused, on every other scene it always animates
- [x] Player display pane — centered on screen, semi-transparent background, no label clutter
- [x] Welcome message overlay — fades out after 5 real seconds; renders above the player pane; both lines in matching green
- [ ] Technology era definitions tied to birth year + calendar year
- [x] Decision popup modal system — the primary interaction driver (first pass live; age-16 work prompt, grocery-store offer, used-computer purchase, computer-use preference chooser)
- [ ] First-introduction events — first computer, first video game, first internet access; triggers vary by birth year and wealth
- [ ] Hacking / project system (core game loop — the Game Dev Story equivalent)
- [x] Skill system first pass — tracked skills, per-skill XP/levels, daily growth from work/computer use, persistent computer-use preference
- [ ] NPC / contacts system
- [ ] ECS component traits for gameplay entities (employees, projects, contacts)
- [x] Achievement system first pass (title browser + gameplay popup modal + birth/epoch/Y2K/2038 achievements)
- [ ] Asset catalog pass: classify current `img/` + `audio/` files as keep / adapt / replace for cyberpunk tone
- [x] Texture integration first pass for character previews from `img/char/`
- [ ] Texture integration for gameplay UI, portraits, icons, backgrounds, and broader scene dressing
- [x] UI SFX integration for navigation/select feedback with working volume control
- [x] Music integration for title/options/gameplay flow using the current `audio/music/` tree as provisional source material
- [ ] Broader audio integration for gameplay/event SFX beyond the current UI feedback layer
- [ ] Shader pipeline for presentation polish (menu treatment, HUD glow, transitions, screen-space effects)
- [ ] Resource management pass for textures/audio/shaders so future asset swaps do not touch gameplay logic

### Future Mechanics — decision system and life simulation

The game is a life-simulation management game centred on the computing and hacking world. All major gameplay unfolds through **popup decision modals** triggered by news events, life milestones, and random occurrences. No WASD; everything is arrow-key and ENTER driven.

#### Decision popup modal system
- [x] `DecisionEvent` struct: prompt text, array of choices, consequences per choice
- [x] Modal renderer in `libdraw.cpp`: frosted-panel overlay with choice list, keyboard navigation
- [x] `GameState` queue for pending decision events (similar to achievement popup queue)
- [x] Consequence resolver in `libinput.cpp`: modifies `GameState` fields based on selected choice
- [x] Decisions currently modify: cash, job-search state, employment state, computer ownership, persistent computer-use preference
- [ ] Expand decision consequences into relationships, reputation, long-term flags, and richer branching outcomes

#### Financial system
- [x] `cash` — liquid cash on hand (USD float)
- [x] First earned-income loop — grocery-store clerk shifts and Friday paydays
- [x] First hardware purchase loop — used home computer purchase at $300
- [ ] Bank accounts — savings and chequing; interest; overdraft
- [ ] Credit cards — credit limit, balance, minimum payment, interest rate
- [ ] Debit cards — linked to bank account
- [ ] Line of credit — flexible borrowing against a limit
- [ ] Income streams — wages, freelance, passive (dividends, royalties, rent)
- [ ] Expenses — rent, food, subscriptions, hardware, software licences
- [ ] Net-worth tracker visible on HUD or dedicated finance screen
- [ ] Financial decisions integrated with news events (e.g. buy an iPad on launch day if funds permit)

#### First-introduction events (milestone triggers)
- [ ] First encounter with a computer — date determined by birth year + wealth + environment
- [ ] First video game experience — arcade, home console, or PC; shapes early skill tree
- [ ] First internet access — era-appropriate: dial-up BBS, AOL, broadband, mobile
- [ ] Each introduction unlocks a branch of the skill tree and opens new decision paths

#### Skill system
- [ ] Skill tree rooted in genetic conditions (Talent, Health, Wealth) and environment
- [x] First tracked skills live: Computer Basics, Hacking, Gaming, Social Media, Programming, Socializing
- [x] Skills grow through choices, jobs, and daily time investment
- [ ] Skill gates control which opportunities become available
- [ ] Add explicit dependency/gating rules so advanced computer-use paths depend on foundational skill levels

#### Career and jobs
- [x] Job system first pass: opt into job search at age 16, daily offer rolls, accept/decline grocery-store clerk, weekday shifts, Friday paydays
- [ ] Job system expansion: interviews, firings, better jobs, and more than one role
- [ ] Job types by era: paper route → tech support → junior dev → senior engineer → CTO / CEO / founder
- [ ] Freelance / contractor gigs: one-off income events, time-limited, skill-gated
- [ ] Unemployment and layoff as random catastrophe
- [ ] Reputation affects job opportunities and NPC interactions

#### Hacking system
- [ ] White-hat path: bug bounties, pen-testing contracts, security consulting, CVE credits
- [ ] Black-hat path: data theft, ransomware, dark-web services — high reward, high legal risk
- [ ] Grey-hat path: vigilante actions, exposing corporate wrongdoing (à la Snowden/Anonymous)
- [ ] Legal risk mechanic: probability of FBI/law-enforcement event scales with black-hat activity
- [ ] Reputation system: community respect vs. notoriety

#### Software and product development
- [ ] Projects: tool, app, game, OS module, exploit kit, crypto miner, SaaS product
- [ ] Development loop: choose project → allocate time → skill checks → ship → monetise
- [ ] Platforms: BBS software → shareware → open source → App Store → SaaS → AI product
- [ ] Employees: hire (cost), skill-match, productivity, morale, payroll
- [ ] Company formation: LLC → startup → acquisition or IPO path

#### Computing and infrastructure
- [x] First personal computer purchase loop — one used starter computer with persistent daily-use preference
- [ ] Personal computer inventory expansion: buy, upgrade, sell hardware; era-appropriate specs matter
- [ ] Home network: router, LAN, NAS, local server
- [ ] Virtual private servers (VPS): rent remote compute, run services, host dark-web nodes
- [ ] Data management: backups, storage, encryption; data-loss events if neglected

#### Relationships and life events
- [ ] NPC contacts: mentors, rivals, collaborators, employers, romantic partners
- [ ] Friendship / reputation tracks per NPC
- [ ] Romantic relationship → marriage option; affects finances and time budget
- [ ] Family: children, ageing parents; care responsibilities affect available time
- [ ] Housing: rent → buy; mortgage affects finances; home-office unlocks hardware bonuses

#### Cryptocurrency
- [ ] Buy / sell / mine crypto at historically accurate prices
- [ ] Wallet management: hot wallet, cold storage, exchange accounts
- [ ] DeFi and NFT events when era-appropriate
- [ ] Loss events: exchange hack, forgotten seed phrase, tax audit

#### Random catastrophe system
These are real life mechanics that can derail progression at any time:
- [ ] Death in the family — emotional toll, time cost, possible inheritance
- [ ] Car accident — medical bills, lost work time, possible disability modifier
- [ ] Mental health decline — productivity penalty, triggers optional treatment decision
- [ ] Physical illness — time and cash drain; health condition worsens if ignored
- [ ] Computer virus / ransomware — data loss, hardware damage, financial hit
- [ ] Natural disaster — location-dependent; property damage, displacement
- [ ] Relationship breakdown — financial and emotional impact
- [ ] Legal trouble — fines, legal fees, possible imprisonment (game-over or major setback)
- [ ] Economic recession — job loss risk, investment value drops, reduced income
- [ ] Catastrophe severity is modulated by the player's preparedness (savings, health, insurance)

**DO NOT ABOVE BELOW THIS LINE**

------

## Architecture reference (for incoming agents)

### Key design rules — do not break these
1. **Raylib stays in `libdraw.cpp` only.** `libinput.cpp` and all game logic headers are raylib-free. This keeps unit tests runnable without a display.
2. **Input is always passed as `InputState`.** Populate it in `main.cpp` via `poll_input()`. Never call `IsKeyPressed` etc. outside of `main.cpp`.
3. **Scene transitions go through `transition(gs, Scene::X)`.** This saves `previous_scene` and resets `menu_selection`.
4. **Doxygen `/** @file */` + `@brief`/`@param`/`@return` on everything.** Owner wants documentation kept up to date.
5. **Every new behaviour gets a CxxTest.** Test headers live in `test_suites/`, generated runner via `cxxtestgen`.
6. **No WASD bindings.** This is a text-input game. Arrow keys only for navigation.
7. **Presentation assets are allowed to evolve rapidly, but the game identity is cyberpunk.** Fantasy-origin placeholder assets are acceptable only if treated as temporary or heavily adapted.
8. **Shaders are first-class rendering tools, not hacks.** Keep shader ownership/update/unload paths explicit and inside the rendering layer.
9. **Desktop rendering currently uses a fixed 1920×1080 render target stretched to the live window size.** Preserve that separation unless there is a strong reason to rework it.
10. **Birth date now means year + month + day.** Gameplay starts on the player birthday, not January 1 by default.
11. **Web build does NOT use Asyncify.** `main.cpp` switches between `emscripten_set_main_loop(update_draw_frame, 0, 1)` (under `PLATFORM_WEB`) and a `while (!WindowShouldClose())` loop on desktop. The `g_gs` file-scope pointer plus `update_draw_frame()` helper exist for this — do not collapse them or call `SetTargetFPS` on the web path (raylib's `WaitTime` would route through `emscripten_sleep` and crash without Asyncify).
12. **Animated background is shared across all scenes.** `draw_starfield(gs)` and `draw_perspective_grid(gs)` are called once at the top of `draw_scene_to_target()` so every scene gets the cyberpunk grid + star field for free. New scenes do NOT need to call them. The pause-freeze cue is gated specifically on `gs.current_scene == Scene::GAMEPLAY && gs.gameplay_paused` so non-gameplay scenes always animate.
13. **Time controls are LEFT/RIGHT arrow keys, not brackets.** The old `[`/`]` bindings and the corresponding `InputState::left_bracket_pressed` / `right_bracket_pressed` fields have been removed. The gameplay scene's modal early-return ensures the new arrow→time bindings cannot fire while a decision popup is open.
14. **Title screen menu has a session-sticky reveal gate.** `gs.title_menu_revealed` starts false; the first keypress on the title scene flips it true and is otherwise *consumed* (no navigate, no select). Once true it persists for the session — do not reset it on scene transitions. `draw_title()` gates only the menu loop on this flag; the title text, subtitle, version, and bottom hint always render.
15. **Font constants in `libdraw.cpp` are global and shared.** `FONT_TITLE` / `FONT_HEAD` / `FONT_BODY` / `FONT_SMALL` are used across many screens. Before changing any of them, grep all call sites and confirm with the user — a "make this screen smaller" request is rarely intended to globally restyle every other screen that uses the same constant. Current sizes: TITLE 70, HEAD 30, BODY 30, SMALL 20.

### File map
| File | Role |
|---|---|
| `config.h` | Screen size (1920×1080), FPS, window title |
| `scene.h` | `Scene` enum |
| `gamestate.h` | `GameState`, `PlayerData`, skills, access/employment/wellbeing state, decision events, achievements, gameplay calendar helpers, condition enums + `env_to_str()` |
| `input_state.h` | `InputState` — one-frame input snapshot |
| `gamedata.h` | `ALL_LOCATIONS`, `YEAR_MIN/MAX/COUNT`, name pools, `random_player()`, `current_year()`, `unlocked_location_count()`, `GOLD_PRICE_TABLE[]`, `gold_price_for_year()` |
| `libinput.h/.cpp` | `update_scene()` — all game logic dispatch |
| `libdraw.h/.cpp` | `draw_scene()` — all raylib rendering dispatch, UI SFX/music playback, achievement popups, fixed-resolution render-target presentation |
| `main.cpp` | Window init, `poll_input()`, `update_draw_frame()` per-frame helper, and dual-mode entry point: `emscripten_set_main_loop` on web / `while` loop on desktop |
| `img/` | Starter texture/sprite source library; currently fantasy-heavy and subject to adaptation/replacement |
| `audio/` | Starter music + SFX source library; currently provisional and thematically mixed |
| `shaders/` | Shader workspace already preloaded by `Makefile`; preferred home for visual treatment work |
| `events.h` | `NewsEvent` struct, `ALL_EVENTS[]` (60 events, 1936–2038), `should_trigger_news_event()` |
| `unit_test.h` | CxxTest suite: ComponentTable + entityid |
| `test_suites/test_gamestate_lifecycle.h` | Scene transition tests |
| `test_suites/test_character_creation.h` | Random + custom character creation tests |
| `test_suites/test_news_events.h` | News event trigger logic and catalogue integrity tests |
| `test_suites/test_gameplay_decisions.h` | Decision-event, job, attendance, computer purchase, and preference-flow tests |
| `minshell.html` | Emscripten web shell |
| `Doxyfile` | Doxygen → `docs/api/html/` |
| `Makefile` | `make game / web / tests / docs / clean` |
| `Makefile.old` | Reference from previous project — keep, do not use directly |

### PlayerData fields (current)
```
name            string
year_of_birth   int         [YEAR_MIN=1900, YEAR_MAX=2050]
birth_month     int         [1, 12]
birth_day       int         [1, 31], clamped by month/year
location        string      from ALL_LOCATIONS (15 entries)
wealth          Wealth      POOR | RICH              (genetic)
health          Health      SICKLY | HEALTHY         (genetic)
talent          Talent      SLOW | TALENTED          (genetic)
env             Environment LOWER_CLASS | MIDDLE_CLASS | UPPER_CLASS
cash            float       liquid cash on hand (USD); set at creation, grows through gameplay
skills          array       tracked skill progress entries (level + xp) for Computer Basics / Hacking / Gaming / Social Media / Programming / Socializing
```

### Custom character screen field indices
```
0  Name        type freely; LEFT/RIGHT rolls random name
1  Year        LEFT/RIGHT (+hold) scrolls year; UP/DOWN navigates fields
2  Month       LEFT/RIGHT (+hold) scrolls month; UP/DOWN navigates fields
3  Day         LEFT/RIGHT (+hold) scrolls valid day for selected month/year
4  Location    LEFT/RIGHT (+hold) scrolls location; UP/DOWN navigates fields
5  START GAME  ENTER confirms + transitions to GAMEPLAY
6  BACK        ENTER returns to CHARACTER_CREATE
```

### What the next session should tackle
The gameplay loop now exists. The next phase is turning these foundation systems into differentiated paths and era-aware content:
- **First-introduction events** — first computer, first video game, first internet access; these should now hook into the existing decision system and skill model instead of creating new parallel systems
- **Computer-use preference depth** — replace purely passive daily gains with occasional modal events/opportunities tied to the selected preference (Programming, Hacking, Gaming, Social Media, Practice)
- **Technology era gating** — tie available actions, computer options, and internet/hacking opportunities to the current calendar year so a 1975 player cannot access a 2005-style internet path
- **Job-system expansion** — add additional entry jobs, interviews/skill checks, and better career progression than the single grocery-store clerk loop
- **Skill gating** — require foundations (for example Computer Basics) before advanced tracks like Hacking or Programming can fully accelerate
- **Broader audio** — gameplay/event SFX beyond the current UI feedback layer
- **Asset triage** — classify `img/` and `audio/` as keep / adapt / replace for cyberpunk tone

### Handoff expectations for future agents
- Assume the current codebase is stable and working. Extend it carefully; do not churn architecture without a concrete payoff.
- Treat `PLAN.md` as the source of truth for project direction and preserve it as the game grows.
- **Before changing any global constant or shared helper, grep its call sites and confirm with the user.** The font constants (`FONT_TITLE` / `FONT_HEAD` / `FONT_BODY` / `FONT_SMALL`) and `draw_menu_item()` are particularly cross-cutting — a request that sounds local will silently restyle multiple screens if you touch them without warning.
- When adding presentation features, keep future replacement in mind. Asset filenames and source art may change.
- UI SFX volume control, menu feedback, and music playback/rotation are already working. Build forward from them instead of replacing them casually.
- Gameplay time progression is live. Respect the current discrete speed ladder, the birthday-based calendar start, and the LEFT/RIGHT arrow time-scale bindings unless there is a concrete reason to redesign them.
- The first gameplay loop is now live. Preserve the current separation between: job search, employment/pay, wellbeing/attendance, computer ownership, and computer-use preference. Do not collapse them into a single generic stat blob.
- Achievement plumbing is live across title/gameplay. New achievements should reuse the existing unlock + popup path rather than inventing a parallel system.
- News event plumbing is live. `ALL_EVENTS[]` in `events.h` is the single source of truth; add events there. The feed and marquee auto-filter by birth date — do not bypass that logic.
- `gold_price_for_year(int year)` in `gamedata.h` provides interpolated historical gold prices; use or extend it rather than hardcoding values elsewhere.
- Character birth date includes year/month/day; do not reintroduce January-1 assumptions.
- Persistent choices now matter. The player's selected computer-use mode is intended to behave like a preference/default, not a one-shot action. If the next agent adds more preference-driven systems, keep them editable but sticky until changed.
- Prefer placeholder integration now over waiting for perfect assets, but do not let placeholder fantasy tone leak into final cyberpunk design decisions.
- Leave clean seams for the next agent: obvious ownership, documented resource paths, and tests for non-rendering behaviour.
- The animated background and the title-screen reveal gate are part of the game's identity now. New scenes inherit the background for free; do not draw it manually inside per-scene functions.

### Direct handoff note
- The whole game now shares a unified animated cyberpunk background — perspective grid + star field — drawn from a single call site in `draw_scene_to_target()`. Title, achievements, options, character creation, and gameplay all share it.
- The title screen is now a centered, dramatic reveal: HACKER STORY (font 70) at y=435, subtitle at y=520, four menu options centered horizontally at LIST_Y=590, all gated behind `gs.title_menu_revealed` (first keypress of the session reveals; reveal is session-sticky and survives scene transitions).
- The gameplay screen is visually complete as a foundation: shared background, news marquee, news feed panel, centered player pane, HUD with cash and gold price, fading welcome overlay, achievement popups. Time-scale is now LEFT/RIGHT arrow keys; the bottom HUD hint reflects this.
- The web build is itch.io-ready. `make web` produces `index.html` + `index.js` + `index.wasm` (~575K) + `index.data`. No Asyncify; uses `emscripten_set_main_loop` directly. The `Makefile` `EMCC_OPTIONS` line should NOT have `-sASYNCIFY` re-added.
- The codebase is stable. 163 unit tests pass. `-Wall -Werror` clean.
- The decision system is already live and used for: age-16 job search, grocery-store job offers, used-computer purchase, and persistent computer-use preference selection/change.
- Current live gameplay loop:
  - turn 16 -> opt into job search
  - daily unemployed offer rolls
  - accept grocery-store clerk
  - weekday work / Friday pay
  - attendance reduced by low mental/physical health
  - save $300 -> buy used computer
  - choose persistent default computer-use mode
  - daily skill gain follows that preference until changed
- Exact next high-value move for the next agent:
  - use the existing decision system to create the first preference-specific computer events
  - start with **Programming** and **Hacking**
  - do not just add more passive XP; add actual modal decisions/opportunities that depend on the current preference and calendar year
- Be careful with the `Makefile`: it does not track header dependencies well. After header-heavy changes, prefer a clean rebuild (`make clean && make game`) before trusting the game binary.

### Inherited ECS headers (from previous project, not yet wired into gameplay)
- `ComponentTable.h` — type-erased per-entity component store
- `ComponentTraits.h` — tag→type mappings (extend this as new component types are needed)
- `entityid.h` — `entityid` typedef + invalid sentinels
- `entitytype.h` — `entitytype_t` enum
- `sprite.h`, `spritegroup.h` — animation system (relevant when visuals are added)
