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

Current project status: the game framework and character creation flow are complete and stable. The gameplay screen is now a well-composed scene with real atmosphere: a scrolling perspective grid with star field background, a top marquee news ticker, a right-side news feed panel, a centered semi-transparent player display pane, a live HUD (name, age, date, session timer, pause state, time scale, cash, gold price), and a fading welcome overlay. The news event catalogue covers 61 historical computing events from 1936 to 2038 with full year/month/day precision, birth-date filtered so players only see events from their own lifetime. Gold price is tracked historically via interpolated annual data from 1900–2050. All 132 unit tests pass. The long-term vision is a full life-simulation management game where the player grows from birth through childhood, career, and beyond — shaped by historical computing milestones, personal decisions, financial state, and random life events.

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
- [x] 112 CxxTest unit tests, all passing, `-Wall -Werror` clean
- [x] Starter asset folders populated: `img/`, `audio/`, `shaders/` are in workspace and preloaded by current `Makefile`

### Gameplay — foundations started
- [x] Main gameplay screen / HUD (player name, age, date, session timer, pause, time-scale, cash, gold price)
- [x] Time/calendar system (game starts on the player birthday; 1-hour simulation step; pause/unpause; adjustable scale)
- [x] News event system — 61 historical computing events (1936–2038), each with year/month/day; scrolling marquee + right-side feed panel; birth-date filtered for both marquee and panel
- [x] Player finances first pass — `cash` field on `PlayerData`; shown on HUD; all characters start at $0
- [x] Historical gold price tracker — interpolated from annual data 1900–2050; displayed on HUD in gold color as a real-world economic compass
- [x] Perspective grid + star field background on gameplay screen; grid scrolls when unpaused, stars twinkle
- [x] Player display pane — centered on screen, semi-transparent background, no label clutter
- [x] Welcome message overlay — fades out after 5 real seconds; renders above the player pane; both lines in matching green
- [ ] Technology era definitions tied to birth year + calendar year
- [ ] Decision popup modal system — the primary interaction driver (see Future Mechanics below)
- [ ] First-introduction events — first computer, first video game, first internet access; triggers vary by birth year and wealth
- [ ] Hacking / project system (core game loop — the Game Dev Story equivalent)
- [ ] Skill system (grows from conditions + choices)
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
- [ ] `DecisionEvent` struct: prompt text, array of choices, consequences per choice
- [ ] Modal renderer in `libdraw.cpp`: frosted-panel overlay with choice list, keyboard navigation
- [ ] `GameState` queue for pending decision events (similar to achievement popup queue)
- [ ] Consequence resolver in `libinput.cpp`: modifies `GameState` fields based on selected choice
- [ ] Decisions can modify: cash, skills, relationships, health, reputation, unlocked paths

#### Financial system
- [x] `cash` — liquid cash on hand (USD float)
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
- [ ] Skills: Programming, Networking, Social Engineering, Hardware, Cryptography, Business, Communication
- [ ] Skills grow through choices, jobs, projects, and time investment
- [ ] Skill gates control which opportunities become available

#### Career and jobs
- [ ] Job system: apply, interview (skill check), get hired, earn wages each pay period
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
- [ ] Personal computer inventory: buy, upgrade, sell hardware; era-appropriate specs matter
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

### File map
| File | Role |
|---|---|
| `config.h` | Screen size (1920×1080), FPS, window title |
| `scene.h` | `Scene` enum |
| `gamestate.h` | `GameState`, `PlayerData`, achievements, gameplay calendar helpers, condition enums + `env_to_str()` |
| `input_state.h` | `InputState` — one-frame input snapshot |
| `gamedata.h` | `ALL_LOCATIONS`, `YEAR_MIN/MAX/COUNT`, name pools, `random_player()`, `current_year()`, `unlocked_location_count()`, `GOLD_PRICE_TABLE[]`, `gold_price_for_year()` |
| `libinput.h/.cpp` | `update_scene()` — all game logic dispatch |
| `libdraw.h/.cpp` | `draw_scene()` — all raylib rendering dispatch, UI SFX/music playback, achievement popups, fixed-resolution render-target presentation |
| `main.cpp` | Window init, game loop, `poll_input()` |
| `img/` | Starter texture/sprite source library; currently fantasy-heavy and subject to adaptation/replacement |
| `audio/` | Starter music + SFX source library; currently provisional and thematically mixed |
| `shaders/` | Shader workspace already preloaded by `Makefile`; preferred home for visual treatment work |
| `events.h` | `NewsEvent` struct, `ALL_EVENTS[]` (60 events, 1936–2038), `should_trigger_news_event()` |
| `unit_test.h` | CxxTest suite: ComponentTable + entityid |
| `test_suites/test_gamestate_lifecycle.h` | Scene transition tests |
| `test_suites/test_character_creation.h` | Random + custom character creation tests |
| `test_suites/test_news_events.h` | News event trigger logic and catalogue integrity tests |
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
The gameplay screen now looks and feels like a real game. The next phase is turning the clock into an actual interactive loop:
- **Decision popup modal system** — the most important next step; build the `DecisionEvent` struct, the modal renderer, the GameState queue, and the consequence resolver; wire the first real decision to a news event (e.g. "A neighbour has a computer — do you ask to use it?")
- **First-introduction events** — first computer, first video game, first internet access; these are the seeds of the skill tree and career path; date and availability should depend on birth year, wealth, and environment
- **Technology era gating** — tie available actions and events to the current calendar year so a 1975 player can't stumble into the web before 1991
- **Skill system skeleton** — even a flat set of named skills with numeric values is enough to start gating decisions and showing growth
- **Broader audio** — gameplay/event SFX beyond the current UI feedback layer
- **Asset triage** — classify `img/` and `audio/` as keep / adapt / replace for cyberpunk tone

### Handoff expectations for future agents
- Assume the current codebase is stable and working. Extend it carefully; do not churn architecture without a concrete payoff.
- Treat `PLAN.md` as the source of truth for project direction and preserve it as the game grows.
- When adding presentation features, keep future replacement in mind. Asset filenames and source art may change.
- UI SFX volume control, menu feedback, and music playback/rotation are already working. Build forward from them instead of replacing them casually.
- Gameplay time progression is live. Respect the current discrete speed ladder and the birthday-based calendar start unless there is a concrete reason to redesign them.
- Achievement plumbing is live across title/gameplay. New achievements should reuse the existing unlock + popup path rather than inventing a parallel system.
- News event plumbing is live. `ALL_EVENTS[]` in `events.h` is the single source of truth; add events there. The feed and marquee auto-filter by birth date — do not bypass that logic.
- `gold_price_for_year(int year)` in `gamedata.h` provides interpolated historical gold prices; use or extend it rather than hardcoding values elsewhere.
- Character birth date includes year/month/day; do not reintroduce January-1 assumptions.
- Prefer placeholder integration now over waiting for perfect assets, but do not let placeholder fantasy tone leak into final cyberpunk design decisions.
- Leave clean seams for the next agent: obvious ownership, documented resource paths, and tests for non-rendering behaviour.

### Direct handoff note
- The gameplay screen is now visually complete as a foundation: star field, perspective grid, news marquee, news feed panel, centered player pane, HUD with cash and gold price, fading welcome overlay, achievement popups.
- The codebase is stable. 132 unit tests pass. `-Wall -Werror` clean.
- The next agent's highest-value move is implementing the **decision popup modal system** — it is the engine that turns the time-passing simulation into an actual game.
- Be careful with the `Makefile`: it does not track header dependencies well. After header-heavy changes, prefer a clean rebuild (`make clean && make game`) before trusting the game binary.

### Inherited ECS headers (from previous project, not yet wired into gameplay)
- `ComponentTable.h` — type-erased per-entity component store
- `ComponentTraits.h` — tag→type mappings (extend this as new component types are needed)
- `entityid.h` — `entityid` typedef + invalid sentinels
- `entitytype.h` — `entitytype_t` enum
- `sprite.h`, `spritegroup.h` — animation system (relevant when visuals are added)
