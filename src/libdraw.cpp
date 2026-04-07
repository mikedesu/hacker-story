/** @file libdraw.cpp
 *  @brief Scene rendering implementations.
 *
 *  All raylib calls live here.  The rest of the codebase is raylib-free.
 *  Each scene has a private static draw handler; `draw_scene()` dispatches
 *  to the correct one inside a BeginDrawing/EndDrawing pair.
 */

#include "libdraw.h"
#include "config.h"
#include "events.h"
#include "gamedata.h"
#include <raylib.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using std::string;
using std::to_string;
using std::vector;

// ── UI audio resources ────────────────────────────────────────────────────────

static const char* SFX_NAVIGATE_PATH = "audio/sfx/scroll.wav";
static const char* SFX_SELECT_PATH   = "audio/sfx/confirm_01.wav";
static const char* HUMAN_IDLE_PATH   = "img/char/human_idle.png";
static const int CHAR_FRAME_SIZE     = 32;
static const int CHAR_PREVIEW_SCALE  = 8;

/**
 * @brief Runtime-owned UI audio resources for menu feedback.
 */
struct UiAudioResources {
    bool initialized{false};          ///< True once init was attempted
    bool audio_ready{false};          ///< True when the audio device is available
    bool navigate_loaded{false};      ///< True when navigate SFX loaded successfully
    bool select_loaded{false};        ///< True when select SFX loaded successfully
    Sound navigate_sound{};           ///< Navigation / cursor movement sound
    Sound select_sound{};             ///< Confirmation / selection sound
    unsigned int last_event_id{0};    ///< Last consumed `GameState::ui_sfx_event_id`
};

/**
 * @brief Return the singleton UI audio resource bundle.
 */
static UiAudioResources& ui_audio() {
    static UiAudioResources res;
    return res;
}

/**
 * @brief One preloaded character spritesheet.
 */
struct CharacterTextureSheet {
    string path;                      ///< Source asset path
    Texture2D texture{};             ///< Loaded texture
};

/**
 * @brief Runtime-owned cache of character spritesheet textures.
 *
 * All character-sheet PNGs in `img/char/` are loaded up front so future sprite/spritegroup
 * instances can safely point at preloaded texture storage owned by libdraw.
 */
struct CharacterTextureResources {
    bool loaded{false};                        ///< True once the directory scan has run
    vector<CharacterTextureSheet> sheets;      ///< All loaded spritesheet textures
};

/**
 * @brief Return the singleton character-texture bundle.
 */
static CharacterTextureResources& character_textures() {
    static CharacterTextureResources res;
    return res;
}

/**
 * @brief Runtime-owned music playback resources.
 */
struct MusicResources {
    vector<string> tracks;            ///< Available music file paths
    bool stream_loaded{false};        ///< True when a music stream is currently loaded
    int current_track{-1};            ///< Index of the current track in `tracks`
    Music stream{};                   ///< Active music stream
};

/**
 * @brief Return the singleton music-resource bundle.
 */
static MusicResources& music_resources() {
    static MusicResources res;
    return res;
}

/**
 * @brief Runtime-owned render target for fixed-resolution scene drawing.
 */
struct RenderResources {
    bool initialized{false};          ///< True once the render target was created
    RenderTexture2D target{};         ///< Internal fixed-resolution render texture
};

/**
 * @brief Return the singleton render-resource bundle.
 */
static RenderResources& render_resources() {
    static RenderResources res;
    return res;
}

// ── Colour palette (terminal-green hacker aesthetic) ─────────────────────────

static const Color COL_BG       = {10,  10,  10,  255};  ///< Near-black background
static const Color COL_GREEN    = {0,   210, 75,  255};  ///< Primary terminal green
static const Color COL_BRIGHT   = {200, 255, 210, 255};  ///< Bright highlighted text
static const Color COL_DIM      = {55,  95,  55,  255};  ///< Dim / inactive text
static const Color COL_CURSOR   = {255, 255, 100, 255};  ///< Active field / cursor yellow
static const Color COL_SHADOW   = {0,   60,  20,  150};  ///< Subtle shadow tint

// ── Typography constants ──────────────────────────────────────────────────────

static const int FONT_TITLE = 70;  ///< Main title size
static const int FONT_HEAD  = 30;  ///< Section / menu heading size
static const int FONT_BODY  = 30;  ///< Body / field label size
static const int FONT_SMALL = 20;  ///< Hint / caption size

/// @brief Current user-facing game version shown on the title screen.
static const char* GAME_VERSION_STRING = "v0.1.0";

// ── Layout helpers ────────────────────────────────────────────────────────────

static const int CX     = SCREEN_WIDTH / 2;  ///< Horizontal center
static const int MARGIN = 80;                 ///< Left margin for left-aligned content

/**
 * @brief Draw text horizontally centred on the screen.
 * @param text      Null-terminated string to draw.
 * @param y         Vertical position of the text top edge.
 * @param font_size Font size in pixels.
 * @param color     Text colour.
 */
static void draw_centered(const char* text, int y, int font_size, Color color) {
    int w = MeasureText(text, font_size);
    DrawText(text, CX - w / 2, y, font_size, color);
}

/**
 * @brief Draw newline-delimited text as separate lines.
 * @param text Full text block, optionally containing `\n`.
 * @param x Left edge for all rendered lines.
 * @param y Top edge of the first line.
 * @param font_size Font size in pixels.
 * @param color Text colour.
 * @param line_gap Vertical spacing between line tops.
 */
static void draw_multiline_text(const string& text, int x, int y, int font_size,
                                Color color, int line_gap) {
    size_t start = 0;
    int line_y = y;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        const string line = (end == string::npos)
            ? text.substr(start)
            : text.substr(start, end - start);
        DrawText(line.c_str(), x, line_y, font_size, color);
        if (end == string::npos) break;
        start = end + 1;
        line_y += line_gap;
    }
}

/**
 * @brief Draw a single menu item, optionally with a `>` cursor indicator.
 * @param text      Option label.
 * @param y         Vertical position.
 * @param selected  When true, draw cursor and use highlight colour.
 * @param font_size Font size (defaults to FONT_BODY).
 */
static void draw_menu_item(const char* text, int y, bool selected,
                           int font_size = FONT_BODY) {
    if (selected) {
        DrawText(">", MARGIN + 16, y, font_size, COL_CURSOR);
    }
    Color col = selected ? COL_BRIGHT : COL_GREEN;
    DrawText(text, MARGIN + 56, y, font_size, col);
}

/**
 * @brief Load all character spritesheet textures from `img/char/`.
 */
static void load_character_textures() {
    CharacterTextureResources& chars = character_textures();
    if (chars.loaded) return;

    FilePathList files = LoadDirectoryFilesEx("img/char", ".png", false);
    for (unsigned int i = 0; i < files.count; i++) {
        Texture2D tex = LoadTexture(files.paths[i]);
        if (tex.id == 0) continue;
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        chars.sheets.push_back(CharacterTextureSheet{files.paths[i], tex});
    }
    UnloadDirectoryFiles(files);
    chars.loaded = true;
}

/**
 * @brief Look up a preloaded character spritesheet by asset path.
 * @param path Asset path to search for.
 * @return Pointer to the loaded texture, or `nullptr` if missing.
 */
static Texture2D* find_character_texture(const char* path) {
    CharacterTextureResources& chars = character_textures();
    if (!chars.loaded) load_character_textures();

    for (auto& sheet : chars.sheets) {
        if (sheet.path == path) return &sheet.texture;
    }
    return nullptr;
}

/**
 * @brief Draw a scaled static preview from the human idle spritesheet.
 *
 * We intentionally use only the first 32x32 frame for now.  Layered character
 * customization is coming soon, so a clean static preview is more useful than
 * wiring in full animation prematurely.
 *
 * @param panel_x Left edge of the preview panel.
 * @param panel_y Top edge of the preview panel.
 */
static void draw_character_preview_panel(int panel_x, int panel_y) {
    const int sprite_px = CHAR_FRAME_SIZE * CHAR_PREVIEW_SCALE;
    const int panel_w   = sprite_px + 96;
    const int panel_h   = sprite_px + 120;
    const int sprite_x  = panel_x + (panel_w - sprite_px) / 2;
    const int sprite_y  = panel_y + (panel_h - sprite_px) / 2;

    DrawRectangle(panel_x, panel_y, panel_w, panel_h, Color{8, 18, 12, 160});
    DrawRectangleLinesEx(Rectangle{
            static_cast<float>(panel_x), static_cast<float>(panel_y),
            static_cast<float>(panel_w), static_cast<float>(panel_h)},
        3.0f, COL_DIM);

    Texture2D* tex = find_character_texture(HUMAN_IDLE_PATH);
    if (!tex) {
        DrawText("missing sprite", panel_x + 20, panel_y + 64, FONT_BODY, COL_CURSOR);
        return;
    }

    Rectangle src = {0.0f, 0.0f, static_cast<float>(CHAR_FRAME_SIZE), static_cast<float>(CHAR_FRAME_SIZE)};
    Rectangle dst = {
        static_cast<float>(sprite_x), static_cast<float>(sprite_y),
        static_cast<float>(sprite_px), static_cast<float>(sprite_px)};
    DrawTexturePro(*tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

/**
 * @brief Format the gameplay time scale as "1s = X".
 * @param gs Current game state.
 * @return Human-readable time-scale description.
 */
static string gameplay_scale_label(const GameState& gs) {
    const int hours_per_second = static_cast<int>((1.0f / gs.seconds_per_game_hour) + 0.5f);
    if (hours_per_second >= 8760) return "1s = 1 year";
    if (hours_per_second >= 720) {
        const int months = hours_per_second / 720;
        if (months * 720 == hours_per_second)
            return TextFormat("1s = %d month%s", months, months == 1 ? "" : "s");
    }
    if (hours_per_second >= 24) {
        const int days = hours_per_second / 24;
        const int hours = hours_per_second % 24;
        if (hours == 0) return TextFormat("1s = %d day%s", days, days == 1 ? "" : "s");
        return TextFormat("1s = %dd %dh", days, hours);
    }
    return TextFormat("1s = %d hour%s", hours_per_second, hours_per_second == 1 ? "" : "s");
}

/**
 * @brief Format the current real-time gameplay session duration.
 * @param gs Current game state.
 * @return `HH:MM:SS` string for the active gameplay session.
 */
static string gameplay_session_label(const GameState& gs) {
    const int total_seconds = static_cast<int>(gs.play_session_seconds);
    const int hours = total_seconds / 3600;
    const int minutes = (total_seconds / 60) % 60;
    const int seconds = total_seconds % 60;
    return TextFormat("Session: %02d:%02d:%02d", hours, minutes, seconds);
}

/**
 * @brief Draw the active gameplay achievement popup modal.
 * @param gs Current game state.
 */
static void draw_achievement_popup(const GameState& gs) {
    if (!gs.achievement_popup_visible) return;

    const int box_w = 760;
    const int box_h = 220;
    const int box_x = SCREEN_WIDTH - box_w - 80;
    const int box_y = 70;
    const AchievementId id = gs.active_achievement_popup;

    DrawRectangle(box_x, box_y, box_w, box_h, Color{5, 14, 8, 230});
    DrawRectangle(box_x + 14, box_y + 14, box_w - 28, box_h - 28, Color{0, 0, 0, 120});
    DrawRectangleLinesEx(Rectangle{
            static_cast<float>(box_x), static_cast<float>(box_y),
            static_cast<float>(box_w), static_cast<float>(box_h)},
        4.0f, COL_GREEN);
    DrawRectangleLinesEx(Rectangle{
            static_cast<float>(box_x + 10), static_cast<float>(box_y + 10),
            static_cast<float>(box_w - 20), static_cast<float>(box_h - 20)},
        2.0f, COL_DIM);

    for (int y = box_y + 30; y < box_y + box_h - 20; y += 10) {
        DrawLine(box_x + 24, y, box_x + box_w - 24, y, Color{0, 120, 40, 18});
    }

    DrawText("ACHIEVEMENT UNLOCKED", box_x + 28, box_y + 24, FONT_SMALL, COL_CURSOR);
    DrawText(achievement_title(id), box_x + 28, box_y + 64, FONT_HEAD, COL_BRIGHT);
    DrawText(achievement_description(id), box_x + 28, box_y + 118, FONT_BODY, COL_GREEN);

    const string ttl = TextFormat("popup %.1fs", gs.achievement_popup_seconds_remaining);
    DrawText(ttl.c_str(), box_x + 28, box_y + 176, FONT_SMALL, COL_DIM);
}

/**
 * @brief Draw the active gameplay decision popup modal.
 * @param gs Current game state.
 */
static void draw_decision_popup(const GameState& gs) {
    if (!gs.decision_popup_visible) return;

    const int box_w = 980;
    const int box_h = 420;
    const int box_x = (SCREEN_WIDTH - box_w) / 2;
    const int box_y = (SCREEN_HEIGHT - box_h) / 2;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{0, 0, 0, 135});
    DrawRectangle(box_x, box_y, box_w, box_h, Color{4, 14, 9, 235});
    DrawRectangle(box_x + 14, box_y + 14, box_w - 28, box_h - 28, Color{0, 0, 0, 120});
    DrawRectangleLinesEx(Rectangle{
            static_cast<float>(box_x), static_cast<float>(box_y),
            static_cast<float>(box_w), static_cast<float>(box_h)},
        4.0f, COL_GREEN);
    DrawRectangleLinesEx(Rectangle{
            static_cast<float>(box_x + 10), static_cast<float>(box_y + 10),
            static_cast<float>(box_w - 20), static_cast<float>(box_h - 20)},
        2.0f, COL_DIM);

    for (int y = box_y + 34; y < box_y + box_h - 24; y += 12) {
        DrawLine(box_x + 24, y, box_x + box_w - 24, y, Color{0, 120, 40, 18});
    }

    DrawText(gs.active_decision.title.c_str(), box_x + 36, box_y + 28, FONT_HEAD, COL_BRIGHT);
    draw_multiline_text(gs.active_decision.prompt, box_x + 36, box_y + 96,
                        FONT_BODY, COL_GREEN, 42);

    int choice_y = box_y + 244;
    for (int i = 0; i < static_cast<int>(gs.active_decision.choices.size()); i++) {
        const bool selected = (i == gs.decision_selection);
        if (selected) DrawText(">", box_x + 38, choice_y, FONT_HEAD, COL_CURSOR);
        DrawText(gs.active_decision.choices[static_cast<size_t>(i)].label.c_str(),
                 box_x + 78, choice_y, FONT_HEAD, selected ? COL_BRIGHT : COL_GREEN);
        choice_y += 54;
    }

    DrawText("UP/DOWN: choose    ENTER: confirm",
             box_x + 36, box_y + box_h - 44, FONT_SMALL, COL_DIM);
}

/**
 * @brief Draw a volume row in the options menu.
 * @param gs Game state containing the current volume and selection.
 * @param label Row label.
 * @param value Volume value to display.
 * @param selected True when the row is highlighted.
 * @param y  Vertical position.
 */
static void draw_volume_row(const GameState& gs, const char* label, int value,
                            bool selected, int y) {
    if (selected) DrawText(">", MARGIN + 16, y, FONT_HEAD, COL_CURSOR);

    DrawText(label, MARGIN + 56, y, FONT_HEAD, selected ? COL_BRIGHT : COL_GREEN);

    string volume = "< " + to_string(value) + "% >";
    DrawText(volume.c_str(), MARGIN + 520, y, FONT_HEAD, selected ? COL_BRIGHT : COL_DIM);
}

/**
 * @brief Convert two percentage volumes into a normalized output gain.
 * @param channel_percent Channel-specific volume [0, 100].
 * @param master_percent  Master volume [0, 100].
 * @return Final normalized gain [0.0, 1.0].
 */
static float combined_volume(int channel_percent, int master_percent) {
    return (static_cast<float>(channel_percent) / 100.0f) *
           (static_cast<float>(master_percent) / 100.0f);
}

/**
 * @brief Play any newly queued UI sound effect.
 * @param gs Current game state.
 */
static void play_ui_sfx(const GameState& gs) {
    UiAudioResources& audio = ui_audio();
    if (!audio.initialized || !audio.audio_ready) return;
    if (gs.ui_sfx_event_id == 0 || gs.ui_sfx_event_id == audio.last_event_id) return;

    audio.last_event_id = gs.ui_sfx_event_id;
    const float volume = combined_volume(gs.sfx_volume_percent, gs.master_volume_percent);

    switch (gs.ui_sfx) {
    case UiSfx::NAVIGATE:
        if (audio.navigate_loaded) {
            SetSoundVolume(audio.navigate_sound, volume);
            PlaySound(audio.navigate_sound);
        }
        break;
    case UiSfx::SELECT:
        if (audio.select_loaded) {
            SetSoundVolume(audio.select_sound, volume);
            PlaySound(audio.select_sound);
        }
        break;
    case UiSfx::NONE:
        break;
    }
}

/**
 * @brief Gather playable music files from `audio/music/`.
 */
static void load_music_catalog() {
    MusicResources& music = music_resources();
    if (!music.tracks.empty()) return;

    FilePathList files = LoadDirectoryFilesEx("audio/music", ".mp3", false);
    for (unsigned int i = 0; i < files.count; i++) {
        music.tracks.push_back(files.paths[i]);
    }
    UnloadDirectoryFiles(files);
}

/**
 * @brief Return a random track index, optionally avoiding the current track.
 * @param exclude Index to avoid when possible.
 * @return Chosen track index, or -1 when no tracks exist.
 */
static int pick_random_track_index(int exclude) {
    const MusicResources& music = music_resources();
    if (music.tracks.empty()) return -1;
    if (music.tracks.size() == 1) return 0;

    int idx = exclude;
    while (idx == exclude) {
        idx = rand() % static_cast<int>(music.tracks.size());
    }
    return idx;
}

/**
 * @brief Load and start a specific music track.
 * @param track_index Index into the discovered music catalog.
 */
static void play_music_track(int track_index) {
    UiAudioResources& audio = ui_audio();
    MusicResources& music = music_resources();
    if (!audio.audio_ready) return;
    if (track_index < 0 || track_index >= static_cast<int>(music.tracks.size())) return;

    if (music.stream_loaded) {
        StopMusicStream(music.stream);
        UnloadMusicStream(music.stream);
        music.stream_loaded = false;
    }

    music.stream = LoadMusicStream(music.tracks[track_index].c_str());
    music.current_track = track_index;
    music.stream_loaded = true;
    PlayMusicStream(music.stream);
}

/**
 * @brief Ensure music playback is running and advance to a new random track when needed.
 * @param gs Current game state.
 */
static void update_music_playback(const GameState& gs) {
    UiAudioResources& audio = ui_audio();
    MusicResources& music = music_resources();
    if (!audio.initialized || !audio.audio_ready) return;

    if (music.tracks.empty()) load_music_catalog();
    if (music.tracks.empty()) return;

    if (!music.stream_loaded) {
        play_music_track(pick_random_track_index(-1));
    }
    if (!music.stream_loaded) return;

    SetMusicVolume(music.stream, combined_volume(gs.music_volume_percent, gs.master_volume_percent));
    UpdateMusicStream(music.stream);

    const float length = GetMusicTimeLength(music.stream);
    const float played = GetMusicTimePlayed(music.stream);
    if ((length > 0.0f && played >= length - 0.05f) || !IsMusicStreamPlaying(music.stream)) {
        play_music_track(pick_random_track_index(music.current_track));
    }
}

/**
 * @brief Create the fixed-resolution render target if it has not been created.
 */
static void init_render_target() {
    RenderResources& render = render_resources();
    if (render.initialized) return;

    render.target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(render.target.texture, TEXTURE_FILTER_BILINEAR);
    render.initialized = true;
}

void init_draw_system() {
    UiAudioResources& audio = ui_audio();
    if (audio.initialized) return;

    init_render_target();
    load_character_textures();
    audio.initialized = true;
    InitAudioDevice();
    audio.audio_ready = IsAudioDeviceReady();
    if (!audio.audio_ready) return;

    if (FileExists(SFX_NAVIGATE_PATH)) {
        audio.navigate_sound  = LoadSound(SFX_NAVIGATE_PATH);
        audio.navigate_loaded = true;
    }
    if (FileExists(SFX_SELECT_PATH)) {
        audio.select_sound  = LoadSound(SFX_SELECT_PATH);
        audio.select_loaded = true;
    }
}

void shutdown_draw_system() {
    UiAudioResources& audio = ui_audio();
    CharacterTextureResources& chars = character_textures();
    MusicResources& music = music_resources();
    RenderResources& render = render_resources();
    if (!audio.initialized) return;

    for (auto& sheet : chars.sheets) {
        UnloadTexture(sheet.texture);
    }
    chars = CharacterTextureResources{};

    if (audio.navigate_loaded) UnloadSound(audio.navigate_sound);
    if (audio.select_loaded)   UnloadSound(audio.select_sound);
    if (music.stream_loaded) {
        StopMusicStream(music.stream);
        UnloadMusicStream(music.stream);
    }
    music = MusicResources{};
    if (audio.audio_ready)     CloseAudioDevice();
    audio = UiAudioResources{};

    if (render.initialized) {
        UnloadRenderTexture(render.target);
        render = RenderResources{};
    }
}

// ── Title screen ──────────────────────────────────────────────────────────────

/**
 * @brief Render the Title screen.
 *
 * Four options: NEW GAME, ACHIEVEMENTS, OPTIONS, and QUIT.
 * Navigated with UP/DOWN; confirmed with ENTER.
 */
static void draw_title(const GameState& gs) {
    draw_centered("HACKER STORY",   435, FONT_TITLE, COL_BRIGHT);
    draw_centered("a life in code", 520, FONT_SMALL, COL_DIM);

    if (gs.title_menu_revealed) {
        const int LIST_Y = 590;
        const int STEP   = 52;
        const char* labels[4] = {"NEW GAME", "ACHIEVEMENTS", "OPTIONS", "QUIT"};
        for (int i = 0; i < 4; i++) {
            const int w = MeasureText(labels[i], FONT_HEAD);
            const int x = CX - w / 2;
            const int y = LIST_Y + STEP * i;
            const bool selected = (gs.menu_selection == i);
            if (selected) {
                DrawText(">", x - 40, y, FONT_HEAD, COL_CURSOR);
            }
            DrawText(labels[i], x, y, FONT_HEAD, selected ? COL_BRIGHT : COL_GREEN);
        }
    }

    const int version_w = MeasureText(GAME_VERSION_STRING, FONT_SMALL);
    DrawText(GAME_VERSION_STRING, SCREEN_WIDTH - MARGIN - version_w,
             SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);

    DrawText("UP/DOWN: navigate    ENTER: select",
             MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Achievements screen ──────────────────────────────────────────────────────

/**
 * @brief Render the Achievements browser screen.
 *
 * Shows all achievements, their unlock state, and a session-local progress count.
 */
static void draw_achievements(const GameState& gs) {
    draw_centered("ACHIEVEMENTS", 80, FONT_HEAD, COL_BRIGHT);

    const string progress = TextFormat("%d / %d unlocked",
                                       achievement_unlock_count(gs), ACHIEVEMENT_COUNT);
    draw_centered(progress.c_str(), 128, FONT_SMALL, COL_DIM);

    const int CARD_X = 180;
    const int CARD_W = SCREEN_WIDTH - CARD_X * 2;
    const int CARD_H = 150;
    const int STEP_Y = 190;

    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        const AchievementId id = static_cast<AchievementId>(i);
        const bool unlocked = achievement_unlocked(gs, id);
        const int y = 190 + i * STEP_Y;

        DrawRectangle(CARD_X, y, CARD_W, CARD_H, Color{8, 18, 12, 255});
        DrawRectangleLinesEx(Rectangle{
                static_cast<float>(CARD_X), static_cast<float>(y),
                static_cast<float>(CARD_W), static_cast<float>(CARD_H)},
            3.0f, unlocked ? COL_GREEN : COL_DIM);

        DrawText(unlocked ? "UNLOCKED" : "LOCKED",
                 CARD_X + 26, y + 18, FONT_SMALL, unlocked ? COL_CURSOR : COL_DIM);
        DrawText(achievement_title(id), CARD_X + 26, y + 52, FONT_HEAD,
                 unlocked ? COL_BRIGHT : COL_GREEN);
        DrawText(achievement_description(id), CARD_X + 26, y + 102, FONT_BODY,
                 unlocked ? COL_GREEN : COL_DIM);
    }

    DrawText("ENTER or ESC: back to title",
             MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Options screen ────────────────────────────────────────────────────────────

/**
 * @brief Render the Options screen.
 *
 * Four options: MASTER VOLUME, MUSIC VOLUME, SFX VOLUME, and BACK.
 */
static void draw_options(const GameState& gs) {
    draw_centered("OPTIONS", 100, FONT_HEAD, COL_BRIGHT);
    draw_centered("Configure presentation and accessibility settings.", 152, FONT_SMALL, COL_DIM);

    const int LIST_Y = 260;
    const int STEP   = 72;
    draw_volume_row(gs, "MASTER VOLUME", gs.master_volume_percent, gs.menu_selection == 0, LIST_Y);
    draw_volume_row(gs, "MUSIC VOLUME",  gs.music_volume_percent,  gs.menu_selection == 1, LIST_Y + STEP);
    draw_volume_row(gs, "SFX VOLUME",    gs.sfx_volume_percent,    gs.menu_selection == 2, LIST_Y + STEP * 2);
    draw_menu_item("BACK", LIST_Y + STEP * 3, gs.menu_selection == 3, FONT_HEAD);

    const char* hint = (gs.menu_selection <= 2)
        ? "LEFT/RIGHT: adjust volume    UP/DOWN: navigate    ESC: back"
        : "ENTER: back to title    UP: volume rows    ESC: back";
    DrawText(hint, MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Character create choice screen ───────────────────────────────────────────

/**
 * @brief Render the Character Creation choice screen.
 *
 * Three options: RANDOM CHARACTER, CUSTOMIZE, BACK.
 */
static void draw_character_create(const GameState& gs) {
    draw_centered("CHARACTER CREATION",                        80, FONT_HEAD,  COL_BRIGHT);
    draw_centered("How would you like to create your character?", 130, FONT_SMALL, COL_DIM);

    const int LIST_Y = 280;
    const int STEP   = 52;
    draw_menu_item("RANDOM CHARACTER", LIST_Y,            gs.menu_selection == 0, FONT_HEAD);
    draw_menu_item("CUSTOMIZE",        LIST_Y + STEP,     gs.menu_selection == 1, FONT_HEAD);
    draw_menu_item("BACK",             LIST_Y + STEP * 2, gs.menu_selection == 2, FONT_HEAD);

    DrawText("UP/DOWN: navigate    ENTER: select    ESC: back to title",
             MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Random character screen ───────────────────────────────────────────────────

/**
 * @brief Render the Random Character screen.
 *
 * Shows the generated character's details and three options:
 * ACCEPT, RE-ROLL, BACK.
 */
static void draw_random_character(const GameState& gs) {
    draw_centered("RANDOM CHARACTER", 60, FONT_HEAD, COL_BRIGHT);

    if (!gs.random_char_generated) {
        draw_centered("Generating...", 220, FONT_BODY, COL_GREEN);
        return;
    }

    const PlayerData& p  = gs.player;
    const int LABEL_X    = MARGIN + 40;
    const int VALUE_X    = MARGIN + 270;
    const int STEP       = 36;
    int y                = 140;

    // Character details
    DrawText("Name:",          LABEL_X, y, FONT_BODY, COL_DIM);
    DrawText(p.name.c_str(),   VALUE_X, y, FONT_BODY, COL_GREEN);
    y += STEP;

    const string birth_date = TextFormat("%02d/%02d/%04d",
                                         p.birth_month, p.birth_day, p.year_of_birth);
    DrawText("Birth Date:", LABEL_X, y, FONT_BODY, COL_DIM);
    DrawText(birth_date.c_str(), VALUE_X, y, FONT_BODY, COL_GREEN);
    y += STEP;

    DrawText("Location:",      LABEL_X, y, FONT_BODY, COL_DIM);
    DrawText(p.location.c_str(), VALUE_X, y, FONT_BODY, COL_GREEN);
    y += STEP;

    DrawText("Genetic:",       LABEL_X, y, FONT_BODY, COL_DIM);
    string genetic;
    genetic += (p.wealth == Wealth::RICH)     ? "Rich"     : "Poor";
    genetic += " / ";
    genetic += (p.health == Health::HEALTHY)  ? "Healthy"  : "Sickly";
    genetic += " / ";
    genetic += (p.talent == Talent::TALENTED) ? "Talented" : "Slow";
    DrawText(genetic.c_str(), VALUE_X, y, FONT_BODY, COL_GREEN);
    y += STEP;

    DrawText("Environmental:", LABEL_X, y, FONT_BODY, COL_DIM);
    DrawText(env_to_str(p.env).c_str(), VALUE_X, y, FONT_BODY, COL_GREEN);

    draw_character_preview_panel(1480, 140);

    // Menu options (shifted down to accommodate the extra conditions row)
    const int LIST_Y  = 420;
    const int MSTEP   = 46;
    draw_menu_item("RE-ROLL", LIST_Y,             gs.menu_selection == 0, FONT_HEAD);
    draw_menu_item("ACCEPT",  LIST_Y + MSTEP,     gs.menu_selection == 1, FONT_HEAD);
    draw_menu_item("BACK",    LIST_Y + MSTEP * 2, gs.menu_selection == 2, FONT_HEAD);

    DrawText("UP/DOWN: navigate    ENTER: select    ESC: back",
             MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Custom character screen ───────────────────────────────────────────────────

/**
 * @brief Render the Custom Character screen.
 *
 * Sequential input fields: Name, Birth Year, Birth Month, Birth Day, Location.
 * The active field is highlighted in yellow; inactive fields are green.
 * Conditions are always Poor / Sickly / Slow for custom characters.
 */
static void draw_custom_character(const GameState& gs) {
    draw_centered("CUSTOM CHARACTER", 60, FONT_HEAD, COL_BRIGHT);

    const int LABEL_X = MARGIN + 40;
    const int VALUE_X = MARGIN + 420;  // wide enough for "Environmental:" at FONT_BODY
    const int FSTEP   = 72;
    int y             = 130;

    // ── Field 0: Name ─────────────────────────────────────────────────────────
    bool name_active = (gs.custom_char_field == 0);
    DrawText("Name:", LABEL_X, y, FONT_BODY, name_active ? COL_CURSOR : COL_DIM);
    string name_disp = gs.text_input + (name_active ? "_" : "");
    if (name_disp.empty()) name_disp = name_active ? "_" : "(enter name)";
    DrawText(name_disp.c_str(), VALUE_X, y, FONT_BODY,
             name_active ? COL_BRIGHT : COL_GREEN);
    y += FSTEP;

    // ── Field 1: Year of Birth ────────────────────────────────────────────────
    bool year_active = (gs.custom_char_field == 1);
    DrawText("Birth Year:", LABEL_X, y, FONT_BODY, year_active ? COL_CURSOR : COL_DIM);
    string year_str = to_string(YEAR_MIN + gs.year_selection);
    if (year_active) year_str = "< " + year_str + " >";
    DrawText(year_str.c_str(), VALUE_X, y, FONT_BODY,
             year_active ? COL_BRIGHT : COL_GREEN);
    y += FSTEP;

    // ── Field 2: Birth Month ──────────────────────────────────────────────────
    bool month_active = (gs.custom_char_field == 2);
    DrawText("Birth Month:", LABEL_X, y, FONT_BODY, month_active ? COL_CURSOR : COL_DIM);
    string month_str = to_string(gs.month_selection);
    if (month_active) month_str = "< " + month_str + " >";
    DrawText(month_str.c_str(), VALUE_X, y, FONT_BODY,
             month_active ? COL_BRIGHT : COL_GREEN);
    y += FSTEP;

    // ── Field 3: Birth Day ────────────────────────────────────────────────────
    bool day_active = (gs.custom_char_field == 3);
    DrawText("Birth Day:", LABEL_X, y, FONT_BODY, day_active ? COL_CURSOR : COL_DIM);
    string day_str = to_string(gs.day_selection);
    if (day_active) day_str = "< " + day_str + " >";
    DrawText(day_str.c_str(), VALUE_X, y, FONT_BODY,
             day_active ? COL_BRIGHT : COL_GREEN);
    y += FSTEP;

    // ── Field 4: Location ─────────────────────────────────────────────────────
    bool loc_active = (gs.custom_char_field == 4);
    DrawText("Location:", LABEL_X, y, FONT_BODY, loc_active ? COL_CURSOR : COL_DIM);
    string loc_str = ALL_LOCATIONS[gs.location_selection];
    DrawText(loc_str.c_str(), VALUE_X, y, FONT_BODY,
             loc_active ? COL_BRIGHT : COL_GREEN);
    if (loc_active) {
        int loc_w = MeasureText(loc_str.c_str(), FONT_BODY);
        DrawText("<", VALUE_X - 30,          y, FONT_BODY, COL_DIM);
        DrawText(">", VALUE_X + loc_w + 14,  y, FONT_BODY, COL_DIM);
    }
    y += FSTEP;

    // ── Fixed conditions ──────────────────────────────────────────────────────
    DrawText("Genetic:",       LABEL_X, y, FONT_BODY, COL_DIM);
    DrawText("Poor / Sickly / Slow", VALUE_X, y, FONT_BODY, COL_DIM);
    DrawText("(unlock better conditions through play)",
             VALUE_X, y + 36, FONT_SMALL, COL_SHADOW);
    y += FSTEP;

    DrawText("Environmental:", LABEL_X, y, FONT_BODY, COL_DIM);
    DrawText("Lower Class",    VALUE_X, y, FONT_BODY, COL_DIM);
    DrawText("(unlock better conditions through play)",
             VALUE_X, y + 36, FONT_SMALL, COL_SHADOW);
    y += FSTEP * 2;

    draw_character_preview_panel(1480, 200);

    // ── Action buttons ────────────────────────────────────────────────────────
    draw_menu_item("START GAME", y,              gs.custom_char_field == 5, FONT_HEAD);
    draw_menu_item("BACK",       y + FONT_HEAD + 16, gs.custom_char_field == 6, FONT_HEAD);

    // ── Context hint ──────────────────────────────────────────────────────────
    const char* hint = "";
    switch (gs.custom_char_field) {
    case 0: hint = "Type name    LEFT/RIGHT: random name    BACKSPACE: delete    ENTER/DOWN: next field"; break;
    case 1: hint = "LEFT/RIGHT: change year    UP/DOWN: prev/next field    ENTER: next";    break;
    case 2: hint = "LEFT/RIGHT: change month    UP/DOWN: prev/next field    ENTER: next";   break;
    case 3: hint = "LEFT/RIGHT: change day    UP/DOWN: prev/next field    ENTER: next";     break;
    case 4: hint = "LEFT/RIGHT: change location    UP/DOWN: prev/next field    ENTER: next"; break;
    case 5: hint = "ENTER: start game    DOWN: back option    ESC: prev field";             break;
    case 6: hint = "ENTER: go back    UP: start game option    ESC: prev field";            break;
    }
    DrawText(hint, MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Gameplay news feed ────────────────────────────────────────────────────────

/// @brief Cached marquee ticker string (rebuilt when feed size changes).
static std::string s_marquee_text;
/// @brief Feed size at the time `s_marquee_text` was last built.
static size_t s_marquee_feed_size = static_cast<size_t>(-1);
/// @brief Current left-edge X position of the scrolling marquee (pixels).
static float s_marquee_x = static_cast<float>(SCREEN_WIDTH);

/**
 * @brief Draw the vertical news-event feed panel on the right side of the gameplay screen.
 *
 * Shows the most-recent events that occurred on or after the player's birth date,
 * newest at top, with uniform brightness.  Events predating the player are omitted.
 *
 * @param gs Current game state.
 */
static void draw_news_feed_panel(const GameState& gs) {
    const int PANEL_W = 540;
    const int PANEL_X = SCREEN_WIDTH - PANEL_W - MARGIN;
    const int PANEL_Y = 100;
    const int PANEL_H = 800;

    DrawRectangle(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, Color{4, 12, 6, 140});
    DrawRectangleLinesEx(Rectangle{
            static_cast<float>(PANEL_X), static_cast<float>(PANEL_Y),
            static_cast<float>(PANEL_W), static_cast<float>(PANEL_H)},
        2.0f, COL_DIM);

    DrawText("NEWS FEED", PANEL_X + 16, PANEL_Y + 12, FONT_SMALL, COL_BRIGHT);
    DrawLine(PANEL_X + 8, PANEL_Y + 36, PANEL_X + PANEL_W - 8, PANEL_Y + 36, COL_DIM);

    const int ENTRY_H = 54;
    const int TEXT_X  = PANEL_X + 14;
    int       text_y  = PANEL_Y + 46;

    // Walk newest-first; skip events that predate the player's birth
    const int feed_size = static_cast<int>(gs.news_feed.size());
    bool any_shown = false;
    for (int i = feed_size - 1; i >= 0; i--) {
        const NewsEvent& ev = ALL_EVENTS[gs.news_feed[static_cast<size_t>(i)]];

        const bool before_birth =
            ev.year < gs.player.year_of_birth ||
            (ev.year == gs.player.year_of_birth &&
             ev.month < gs.player.birth_month);
        if (before_birth) continue;

        DrawText(TextFormat("%02d/%02d/%04d", ev.month, ev.day, ev.year),
                 TEXT_X, text_y, FONT_SMALL, COL_DIM);
        DrawText(ev.headline, TEXT_X, text_y + 16, FONT_SMALL, COL_GREEN);
        text_y += ENTRY_H;
        any_shown = true;
        if (text_y + ENTRY_H > PANEL_Y + PANEL_H - 8) break;
    }

    if (!any_shown)
        DrawText("No events yet...", TEXT_X, PANEL_Y + 52, FONT_SMALL, COL_DIM);
}

/**
 * @brief Draw the scrolling news-ticker marquee strip at the top of the gameplay screen.
 *
 * All triggered events are concatenated into a single scrolling line.  The
 * strip scrolls when the game is unpaused and freezes when paused.  The
 * marquee text cache is rebuilt automatically whenever new events are triggered.
 *
 * @param gs Current game state.
 */
static void draw_news_marquee(const GameState& gs) {
    const int STRIP_H = 38;

    DrawRectangle(0, 0, SCREEN_WIDTH, STRIP_H, Color{0, 8, 3, 230});
    DrawLine(0, STRIP_H, SCREEN_WIDTH, STRIP_H, COL_DIM);

    // Rebuild cache when feed grows
    if (gs.news_feed.size() != s_marquee_feed_size) {
        s_marquee_text.clear();
        for (int idx : gs.news_feed) {
            const NewsEvent& ev = ALL_EVENTS[idx];
            const bool before_birth =
                ev.year < gs.player.year_of_birth ||
                (ev.year == gs.player.year_of_birth &&
                 ev.month < gs.player.birth_month);
            if (before_birth) continue;
            s_marquee_text += TextFormat("  %02d/%02d/%04d  |  ", ev.month, ev.day, ev.year);
            s_marquee_text += ev.headline;
            s_marquee_text += "   ///";
        }
        s_marquee_feed_size = gs.news_feed.size();
    }

    // Show placeholder when no post-birth events have triggered yet
    if (s_marquee_text.empty()) {
        DrawText("HACKER STORY NEWS NETWORK  --  awaiting first transmission...",
                 static_cast<int>(s_marquee_x), 10, FONT_SMALL, WHITE);
        if (!gs.gameplay_paused) s_marquee_x -= GetFrameTime() * 120.0f;
        if (s_marquee_x < -SCREEN_WIDTH * 2) s_marquee_x = static_cast<float>(SCREEN_WIDTH);
        return;
    }

    const int text_w = MeasureText(s_marquee_text.c_str(), FONT_SMALL);

    if (!gs.gameplay_paused) {
        s_marquee_x -= GetFrameTime() * 140.0f;
        if (s_marquee_x <= -static_cast<float>(text_w))
            s_marquee_x = static_cast<float>(SCREEN_WIDTH);
    }

    // Draw twice for seamless wrap
    DrawText(s_marquee_text.c_str(), static_cast<int>(s_marquee_x), 10,
             FONT_SMALL, WHITE);
    DrawText(s_marquee_text.c_str(), static_cast<int>(s_marquee_x) + text_w, 10,
             FONT_SMALL, WHITE);
}

// ── Animated background (used by every scene) ────────────────────────────────

/** @brief One star in the background star field. */
struct Star {
    float x;            ///< Screen X position
    float y;            ///< Screen Y position
    unsigned char base; ///< Base brightness [80, 240]
    float phase;        ///< Twinkle phase offset (radians)
};

/// @brief Pre-generated star positions (populated once, deterministic).
static std::vector<Star> s_stars;

/// @brief Accumulated time used for star twinkle animation (seconds).
static float s_star_time = 0.0f;

/**
 * @brief Populate `s_stars` with deterministic pseudo-random positions.
 *
 * Uses an inline LCG so the global `rand()` state (used by music shuffle)
 * is left untouched.
 */
static void init_starfield() {
    if (!s_stars.empty()) return;
    unsigned int seed = 0xDEADBEEF;
    auto lcg = [&]() -> unsigned int {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    };
    s_stars.reserve(220);
    for (int i = 0; i < 220; i++) {
        Star s;
        s.x     = static_cast<float>(lcg() % SCREEN_WIDTH);
        s.y     = static_cast<float>(lcg() % 470);          // stays above horizon (vp_y=482)
        s.base  = static_cast<unsigned char>(80 + lcg() % 160);
        s.phase = static_cast<float>(lcg() % 628) / 100.0f; // 0..2π
        s_stars.push_back(s);
    }
}

/**
 * @brief Draw the background star field above the perspective grid horizon.
 *
 * Stars twinkle via a per-star sine phase.  Animation freezes only on the
 * gameplay scene while the simulation is paused; on every other scene the
 * stars always animate so the background reads as alive.
 *
 * @param gs Current game state (scene + pause flag read here; nothing mutated).
 */
static void draw_starfield(const GameState& gs) {
    init_starfield();
    const bool freeze = (gs.current_scene == Scene::GAMEPLAY) && gs.gameplay_paused;
    if (!freeze) {
        s_star_time += GetFrameTime();
    }
    for (const Star& s : s_stars) {
        const float t      = 0.5f + 0.5f * std::sin(s_star_time * 1.4f + s.phase);
        const auto  bright = static_cast<unsigned char>(s.base * t);
        DrawPixel(static_cast<int>(s.x), static_cast<int>(s.y),
                  Color{bright, bright, static_cast<unsigned char>(bright * 0.85f), 255});
    }
}

// ── Animated perspective grid (used by every scene) ──────────────────────────

/// @brief Accumulated right-to-left scroll offset for the perspective grid (world units).
static float s_grid_scroll_x = 0.0f;

/**
 * @brief Draw the cyberpunk perspective grid as an animated scene background.
 *
 * A classic perspective grid on the "floor" below the horizon, with vertical
 * columns that scroll left while time is running — simulating flight into the
 * future.  Horizontal depth bands are drawn first; diverging column lines on top.
 *
 * Scrolling freezes only on the gameplay scene while the simulation is paused;
 * on every other scene the grid always scrolls.
 *
 * @param gs Current game state (scene + pause flag read here; nothing mutated).
 */
static void draw_perspective_grid(const GameState& gs) {
    const bool freeze = (gs.current_scene == Scene::GAMEPLAY) && gs.gameplay_paused;
    if (!freeze) {
        s_grid_scroll_x += GetFrameTime() * 0.5f;
    }

    const float vp_x    = SCREEN_WIDTH * 0.5f;                    ///< Vanishing point X
    const float vp_y    = 482.0f;                                  ///< Horizon / vanishing point Y
    const float g_btm   = static_cast<float>(SCREEN_HEIGHT) - 50.0f;
    const float persp   = g_btm - vp_y;                           ///< Vertical perspective scale
    const float col_spc = 0.15f;                                   ///< World-space column interval

    // ── Horizontal depth bands ────────────────────────────────────────────────
    // Step in screen-pixel space so lines are evenly distributed top-to-bottom.
    // Depth is derived from screen y purely for the brightness fade.
    const float h_step = 26.0f;
    for (float sy = vp_y + h_step; sy < g_btm; sy += h_step) {
        const float z     = persp / (sy - vp_y);   // depth at this screen row
        const auto bright = static_cast<unsigned char>(std::min(180.0f / z, 220.0f));
        const auto alpha  = static_cast<unsigned char>(std::min(130.0f / z, 200.0f));
        DrawLine(0, static_cast<int>(sy), SCREEN_WIDTH, static_cast<int>(sy),
                 Color{0, bright, static_cast<unsigned char>(bright / 4u), alpha});
    }

    // ── Vertical columns (scroll left while unpaused) ─────────────────────────
    const float frac   = std::fmod(s_grid_scroll_x, col_spc);
    const int   half_n = static_cast<int>(2.0f / col_spc) + 3;
    for (int i = -half_n; i <= half_n; i++) {
        const float wx    = i * col_spc - frac;
        const float sx_bt = vp_x + wx * persp;
        if (sx_bt < -60.0f || sx_bt > SCREEN_WIDTH + 60.0f) continue;
        const float edge_fade = 1.0f - std::fabs(wx) / (half_n * col_spc);
        const auto alpha = static_cast<unsigned char>(std::max(0.0f, 110.0f * edge_fade));
        DrawLine(static_cast<int>(vp_x), static_cast<int>(vp_y),
                 static_cast<int>(sx_bt), static_cast<int>(g_btm),
                 Color{0, 200, 70, alpha});
    }
}

// ── Gameplay stub screen ──────────────────────────────────────────────────────

/**
 * @brief Render the Gameplay stub screen.
 *
 * Placeholder until the core gameplay loop is implemented.
 */
static void draw_gameplay(const GameState& gs) {
    draw_news_marquee(gs);

    const GameDateTime dt = gameplay_datetime(gs);
    const string age_line     = "Age: "   + to_string(gameplay_age_years(gs));
    const string date_line    = TextFormat("Date: %02d/%02d/%04d %02d:00",
                                           dt.month, dt.day, dt.year, dt.hour);
    const string session_line = gameplay_session_label(gs);
    const string pause_line   = string("Time: ") + (gs.gameplay_paused ? "PAUSED" : "RUNNING");
    const string scale_line   = "Scale: " + gameplay_scale_label(gs);
    const string job_line = gs.employment.employed
        ? string("Work: ") + job_title(gs.employment.current_job)
        : (gs.employment.searching_for_job ? "Work: Searching for part-time job"
                                           : "Work: Not job hunting");
    const string computer_line = gs.access.owns_computer
        ? "Computer: Owned"
        : "Computer: None";
    const string preference_line = string("Use: ")
        + computer_use_preference_name(gs.access.computer_use_preference);
    const SkillProgress& basics = gs.player.skills[skill_index(SkillId::COMPUTER_BASICS)];
    const SkillProgress& hacking = gs.player.skills[skill_index(SkillId::HACKING)];
    const SkillProgress& gaming = gs.player.skills[skill_index(SkillId::GAMING)];
    const SkillProgress& social_media = gs.player.skills[skill_index(SkillId::SOCIAL_MEDIA)];
    const SkillProgress& programming = gs.player.skills[skill_index(SkillId::PROGRAMMING)];
    const string computer_skill_line = TextFormat("B:%d H:%d G:%d S:%d P:%d",
        basics.level, hacking.level, gaming.level, social_media.level, programming.level);
    const string wellbeing_line = TextFormat("Mind/Body: %d / %d",
                                             gs.wellbeing.mental_health,
                                             gs.wellbeing.physical_health);
    int attendance_chance = (gs.wellbeing.mental_health + gs.wellbeing.physical_health) / 2;
    if (attendance_chance < 0) attendance_chance = 0;
    if (attendance_chance > 100) attendance_chance = 100;
    const string shifts_line = TextFormat("Week: %d worked / %d missed",
                                          gs.employment.shifts_worked_this_week,
                                          gs.employment.missed_shifts_this_week);
    const string attendance_line = TextFormat("Attendance: %d%%", attendance_chance);
    const string paycheck_line = TextFormat("Last Paycheck: $%.2f",
        static_cast<float>(gs.employment.last_paycheck_cents) / 100.0f);

    // ── HUD — name + time/date block ─────────────────────────────────────────
    DrawText(gs.player.name.c_str(), MARGIN, 46,  FONT_BODY,  COL_BRIGHT);
    DrawText(age_line.c_str(),       MARGIN, 82,  FONT_BODY,  COL_GREEN);
    DrawText(date_line.c_str(),      MARGIN, 118, FONT_BODY,  COL_GREEN);
    DrawText(session_line.c_str(),   MARGIN, 192, FONT_SMALL, COL_DIM);
    DrawText(pause_line.c_str(),     MARGIN, 216, FONT_SMALL,
             gs.gameplay_paused ? COL_CURSOR : COL_DIM);
    DrawText(scale_line.c_str(),     MARGIN, 240, FONT_SMALL, COL_DIM);

    const string cash_line = TextFormat("Cash: $%.2f", gs.player.cash);
    DrawText(cash_line.c_str(),      MARGIN, 268, FONT_SMALL, COL_CURSOR);
    DrawText(job_line.c_str(),       MARGIN, 292, FONT_SMALL, COL_GREEN);
    DrawText(computer_line.c_str(),  MARGIN, 316, FONT_SMALL, COL_GREEN);
    DrawText(preference_line.c_str(),MARGIN, 340, FONT_SMALL, COL_DIM);
    DrawText(computer_skill_line.c_str(), MARGIN, 364, FONT_SMALL, COL_DIM);
    DrawText(wellbeing_line.c_str(), MARGIN, 388, FONT_SMALL, COL_DIM);
    DrawText(shifts_line.c_str(),    MARGIN, 412, FONT_SMALL, COL_GREEN);
    DrawText(attendance_line.c_str(),MARGIN, 436, FONT_SMALL, COL_DIM);
    DrawText(paycheck_line.c_str(),  MARGIN, 460, FONT_SMALL, COL_DIM);

    const string gold_line = TextFormat("Gold: $%.2f/oz", gold_price_for_year(dt.year));
    DrawText(gold_line.c_str(),      MARGIN, 484, FONT_SMALL, Color{255, 200, 50, 255});

    draw_news_feed_panel(gs);
    draw_character_preview_panel(784, 352);

    // ── Welcome message — fades out over real-time seconds ───────────────────
    // Full alpha for 3 s, then fades to transparent over the next 2 s.
    const float t_fade = std::max(0.0f,
                         std::min(1.0f, (gs.play_session_seconds - 3.0f) / 2.0f));
    const auto  w_alpha = static_cast<unsigned char>(255.0f * (1.0f - t_fade));
    if (w_alpha > 0) {
        const string welcome = "Welcome, " + gs.player.name + "!";
        draw_centered(welcome.c_str(), 310, FONT_HEAD,
                      Color{COL_GREEN.r, COL_GREEN.g, COL_GREEN.b, w_alpha});

        const string detail = TextFormat("%02d/%02d/%04d  |  %s",
                                         gs.player.birth_month, gs.player.birth_day,
                                         gs.player.year_of_birth,
                                         gs.player.location.c_str());
        draw_centered(detail.c_str(), 360, FONT_BODY,
                      Color{COL_GREEN.r, COL_GREEN.g, COL_GREEN.b, w_alpha});
    }
    draw_achievement_popup(gs);
    draw_decision_popup(gs);

    DrawText("SPACE: pause    ENTER: computer use    LEFT: slower    RIGHT: faster    ESC: back to title",
             MARGIN, SCREEN_HEIGHT - 40, FONT_SMALL, COL_DIM);
}

// ── Public dispatch ───────────────────────────────────────────────────────────

/**
 * @brief Draw the active scene into the fixed-resolution render target.
 *
 * The animated starfield + perspective grid are rendered first so every scene
 * shares the same cyberpunk background; per-scene content draws on top.
 *
 * @param gs Current game state.
 */
static void draw_scene_to_target(const GameState& gs) {
    draw_starfield(gs);
    draw_perspective_grid(gs);
    switch (gs.current_scene) {
    case Scene::TITLE:            draw_title(gs);            break;
    case Scene::ACHIEVEMENTS:     draw_achievements(gs);     break;
    case Scene::OPTIONS:          draw_options(gs);          break;
    case Scene::CHARACTER_CREATE: draw_character_create(gs); break;
    case Scene::RANDOM_CHARACTER: draw_random_character(gs); break;
    case Scene::CUSTOM_CHARACTER: draw_custom_character(gs); break;
    case Scene::GAMEPLAY:         draw_gameplay(gs);         break;
    }
}

void draw_scene(const GameState& gs) {
    if (!ui_audio().initialized) init_draw_system();
    update_music_playback(gs);
    play_ui_sfx(gs);

    RenderResources& render = render_resources();
    if (!render.initialized) init_render_target();

    BeginTextureMode(render.target);
    ClearBackground(COL_BG);
    draw_scene_to_target(gs);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(COL_BG);
    Rectangle src = {0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), -static_cast<float>(SCREEN_HEIGHT)};
    Rectangle dst = {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    DrawTexturePro(render.target.texture, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}
