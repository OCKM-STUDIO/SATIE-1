/* SPDX-License-Identifier: GPL-3.0-only */
/* SATIE-1: simple home controls and instrument/effect pages. */
#include "satie_presets.h"
#include "satie_art.h"
#include "../dsp/satie_data.h"

#define SATIE_MAGIC 0x31544153u /* SAT1 */
#define LEGACY_SCORE_MAGIC 0x31484342u /* previous favourite layout */
#define SATIE_MOODS 5
#define SATIE_PAGES 8
enum { SATIE_PENDING_TRACK = 1, SATIE_PENDING_MOOD };
static const char *const SATIE_PAGE[SATIE_PAGES] = {"HOME", "LEAD", "BASS", "DRUMS", "LAYERS", "COLOUR", "DELAY", "SPACE"};
static const char *const SATIE_LABEL[SATIE_PAGES][4] = {
    {"TEMPO","ENERGY","EVOLVE","MIX"}, {"TONE","CHAR","SPACE","LEVEL"},
    {"TONE","ATTACK","LENGTH","LEVEL"}, {"TONE","SWING","PATTERN","LEVEL"},
    {"VOICE","ATMOS","SPACE","LEVEL"}, {"STYLE","AMOUNT","WOBBLE","DUST"},
    {"TIME","REPEATS","TONE","WET"}, {"REVERB","SIZE","TONE","WIDTH"}
};
static const uint8_t SATIE_EDIT_DEFAULT[7][4] = {
    {50,50,50,50}, {50,50,50,50}, {50,50,2,50}, {50,50,50,50},
    {0,35,15,0}, {1,55,50,50}, {50,50,50,50}
};
static const char *const SATIE_COLOUR[4] = {"CLEAN","VINYL","TAPE","GRIT"};
static const char *const SATIE_PATTERN[5] = {"SPARSE","STEADY","MOOD","BROKEN","BUSY"};
static const char *const SATIE_TIME[3] = {"1/8","1/8.","1/4"};
#define SATIE_HELP_PAGES 8
/* Short lines for the 240-pixel display; the preview uses this same copy. */
static const struct { const char *title; const char *line[6]; } SATIE_HELP[SATIE_HELP_PAGES] = {
    {"KEPT SIMPLE", {"I've kept it simple.", "Choose a tune and a mood,", "then let the music evolve", "while you work.", "Start on Home. Four knobs", "are enough to make it yours."}},
    {"CHOOSE YOUR MUSIC", {"PRESET chooses the tune.", "SELECT chooses the mood.", "SEL opens the tune library.", "Turn SELECT, then SEL", "to choose a collection/tune.", "HOME goes back."}},
    {"FOUR HOME KNOBS", {"1 TEMPO: how fast it plays.", "2 ENERGY: pulse and drive.", "3 EVOLVE: how far it wanders.", "4 MIX: Solo, Duet or Trio.", "At zero Evolve, the lead", "follows the source notes."}},
    {"SOLO / DUET / TRIO", {"Press HOME, then use knob 4.", "MIX 0: lead and drums.", "MIX 45: add bass and air.", "MIX 85: add a third voice.", "It blends between them.", "Delay WET only adds echoes."}},
    {"SHAPE THE SOUND", {"ALGO selects an edit page:", "Lead, Bass, Drums, Layers.", "The four knobs follow it.", "Level 50 keeps the balance.", "Level 0 mutes that part.", "EDIT resets just that page."}},
    {"COLOUR AND SPACE", {"ALGO also finds the FX.", "Colour: Vinyl, Tape or Grit.", "Clean bypasses the colour.", "Wobble bends; Dust crackles.", "Delay: time, repeats, wet.", "Space: room, tone and width."}},
    {"LET IT EVOLVE", {"HOME + EDIT: a new track.", "Press HOME, then EDIT.", "REC holds this 8-bar phrase.", "REC again lets it continue.", "PLAY pauses or resumes.", "MASTER sets your volume."}},
    {"KEEP AND RECALL", {"SAVE keeps one favourite:", "tune, mood and every edit.", "GLO opens this guide.", "SEL here recalls your save.", "It restarts on the next bar", "while playing. HOME closes."}}
};
typedef struct {
    uint32_t magic;
    uint16_t version, seed;
    uint8_t mood, source, tempo, energy, evolve, space, voices, reserved; /* space stores Mix; keep saved layout */
    uint8_t edit[7][4]; /* version 4; first 16 bytes retain the legacy layout */
} satie_settings_t;
static struct {
    satie_settings_t current, requested, saving;
    uint32_t buttons, last_bar;
    uint32_t mix_ms;
    int mix_position;
    int16_t base[P_NPARAMS];
    uint8_t live[7][4];
    uint8_t page, pending, store_wait, help;
    uint8_t browser, browser_collection, browser_cursor;
} satie_ui;

static int satie_clamp(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

/* Keep the legacy favourite prefix and storage slot. Import the six old
 * Satie choices; a retired tune becomes Gymnopedie 1, never another work. */
static int satie_load(satie_settings_t *settings)
{
    int p, k;
    const int size = plat_store_load(OBJ_SATIE, settings, sizeof *settings);
    if ((size != 16 && size != sizeof *settings) || !settings->seed) return 0;
    if (settings->magic == LEGACY_SCORE_MAGIC) {
        if ((settings->version != 1 && settings->version != 2) ||
            (settings->version == 1 && (settings->source >= 3 || settings->mood >= 2)) ||
            (settings->version == 2 && settings->source >= 39)) return 0;
        if (settings->version == 2 && settings->source >= 33) settings->source -= 33;
        else {
            settings->source = 0;
            settings->tempo = SATIE_SOURCE[0].tempo;
        }
        settings->magic = SATIE_MAGIC;
        settings->version = 3;
        if (settings->voices > 2) return 0;
        settings->reserved = 0;
    }
    if (settings->magic == SATIE_MAGIC && settings->version == 3 && size == 16) {
        memcpy(settings->edit, SATIE_EDIT_DEFAULT, sizeof settings->edit);
        settings->version = 4;
    } else if (settings->version != 4 || size != sizeof *settings) return 0;
    for (p = 0; p < 7; p++) for (k = 0; k < 4; k++) {
        const int max = p == 2 && k == 2 ? 4 : p == 4 && k == 0 ? 3 : p == 5 && k == 0 ? 2 : 100;
        if (settings->edit[p][k] > max) return 0;
    }
    return settings->magic == SATIE_MAGIC && settings->version == 4 &&
        settings->mood < SATIE_MOODS && settings->source < SATIE_SOURCES &&
        settings->tempo >= 50 && settings->tempo <= 150 && settings->energy <= 100 &&
        settings->evolve <= 100 && settings->space <= 100 && settings->voices < 3;
}

static const char *const SATIE_MOOD_NAME[SATIE_MOODS] = {"AFTER HOURS", "IBIZA CHILL", "ORBIT", "BLUE NOTE", "GHOST"};
static const char *const SATIE_VOICE_NAME[3] = {"SOLO", "DUET", "TRIO"};

static int satie_tempo(const satie_settings_t *settings)
{
    const satie_source_t *source = &SATIE_SOURCE[settings->source];
    return source->tempo;
}

static void satie_param(int parameter, int value)
{
    const en_param_t *info = en_param_info(parameter);
    proj.par[parameter] = (int16_t)satie_clamp(value, info->lo, info->hi);
    en_set(parameter, proj.par[parameter]);
}

/* One continuous arrangement and mix control; never restart the score. */
static int satie_mix_value(int punch, int balanced, int floating)
{
    const int mix = satie_ui.mix_position;
    return mix <= 50 ? punch + (balanced - punch) * mix / 50 :
        balanced + (floating - balanced) * (mix - 50) / 50;
}

static void satie_mix(void)
{
    static const uint8_t room[5][3] = {{6,42,82},{8,42,78},{12,42,82},{3,28,48},{8,44,78}};
    static const uint8_t echo[5][3] = {{4,33,60},{6,33,58},{12,43,68},{0,5,12},{8,34,60}};
    static const uint8_t size[5][3] = {{35,66,92},{40,66,90},{50,76,94},{25,44,62},{40,74,92}};
    const int mood = satie_ui.current.mood;
    const int mix = satie_ui.mix_position;
    const int accompaniment = satie_clamp((mix - EN_SATIE_MIX_BASS_START) * 100 /
        (EN_SATIE_MIX_BASS_FULL - EN_SATIE_MIX_BASS_START), 0, 100);
    const int answer = satie_clamp((mix - EN_SATIE_MIX_INNER_START) * 100 /
        (EN_SATIE_MIX_INNER_FULL - EN_SATIE_MIX_INNER_START), 0, 100);
    const int drum = proj.par[P_PULSE], bass = proj.par[P_BASS];
    const int lead = proj.par[P_CM_LVL], pad = proj.par[P_PAD], inner = proj.par[P_ACID];
    satie_param(P_PULSE, satie_mix_value(drum + (mood == 3 ? 5 : 10), drum, drum - (mood == 3 ? 5 : 12)));
    satie_param(P_BASS, satie_mix_value(bass + 6, bass, bass - 8) * accompaniment / 100);
    satie_param(P_CM_LVL, satie_mix_value(lead - 3, lead, lead + 3));
    satie_param(P_PAD, pad ? satie_mix_value(pad - 14, pad, pad + 12) * accompaniment / 100 : 0);
    satie_param(P_ACID, inner ? satie_mix_value(inner - 4, inner, inner + 4) * answer / 100 : 0);
    satie_param(P_REVERB, satie_mix_value(room[mood][0], room[mood][1], room[mood][2]));
    satie_param(P_ECHO, satie_mix_value(echo[mood][0], echo[mood][1], echo[mood][2]));
    satie_param(P_SIZE, satie_mix_value(size[mood][0], size[mood][1], size[mood][2]));
    en_satie_arrangement(mix);
}

static void satie_base_macros(void)
{
    const satie_settings_t *settings = &satie_ui.current;
    const int energy = settings->energy;
    const int orbit = settings->mood == 2;
    satie_param(P_TEMPO, settings->tempo);
    if (settings->mood == 4) {
        satie_param(P_PULSE, 48 + energy / 3);
        satie_param(P_CM_LVL, 52 + energy / 8);
        satie_param(P_ACID, 10 + energy / 8);
        satie_param(P_BASS, 68);
        satie_param(P_PAD, 32);
        satie_param(P_DENS3, 35 + energy / 8);
        satie_param(P_DRIVE, 6 + energy / 8);
        satie_param(P_EVOLVE, settings->evolve);
        satie_mix();
        return;
    }
    if (settings->mood == 3) {
        satie_param(P_PULSE, 25 + energy / 5);
        satie_param(P_CM_LVL, 52 + energy / 10);
        satie_param(P_ACID, 0);                   /* the third part uses the piano in this mood */
        satie_param(P_BASS, 64);
        satie_param(P_PAD, 0);
        satie_param(P_DENS3, 40 + energy / 10);
        satie_param(P_DRIVE, energy / 30);
        satie_param(P_EVOLVE, settings->evolve);
        satie_mix();
        return;
    }
    satie_param(P_PULSE, (orbit ? 15 : settings->mood ? 31 : 37) + energy / (orbit ? 3 : 2));
    satie_param(P_CM_LVL, orbit ? 50 + energy / 7 : settings->mood ? 46 + energy / 8 : 46 + energy / 6);
    satie_param(P_ACID, (orbit ? 14 : settings->mood ? 10 : 15) + energy / 5);
    satie_param(P_BASS, orbit ? 52 : 60);
    satie_param(P_PAD, orbit ? 54 : settings->mood ? 56 : 40);
    satie_param(P_DENS3, (orbit ? 10 : 20) + energy / (orbit ? 4 : 2));
    satie_param(P_DRIVE, (orbit ? 1 : settings->mood ? 2 : 15) + energy / (orbit ? 12 : 4));
    satie_param(P_EVOLVE, settings->evolve);
    satie_mix();
}

/* Re-derive from the mood on every edit: no cumulative drift when Mix moves. */
static void satie_macros(void)
{
    const uint8_t (*e)[4] = satie_ui.live;
    const int16_t *b = satie_ui.base;
    const int style = e[4][0], amount = e[4][1];
    uint64_t fx = ui.fx_lock & ~(FXB(FX_TAPE) | FXB(FX_VINYL));
    satie_base_macros();
    /* Level is a relative trim: 50 preserves the mood, 0 mutes its role. */
    satie_param(P_CM_LVL, proj.par[P_CM_LVL] * e[0][3] / 50);
    satie_param(P_BASS, proj.par[P_BASS] * e[1][3] / 50);
    satie_param(P_PULSE, proj.par[P_PULSE] * e[2][3] / 50);
    satie_param(P_ACID, proj.par[P_ACID] * e[3][0] * e[3][3] / 2500);
    satie_param(P_PAD, proj.par[P_PAD] * e[3][1] * e[3][3] / 2500);
    satie_param(P_SEND3, b[P_SEND3] * e[0][2] / 50);
    satie_param(P_CM_DEC, b[P_CM_DEC] + (satie_ui.current.mood == 1 ? ((int)e[0][1]-50)/3 : 0));
    satie_param(P_SEND, b[P_SEND] * e[3][2] / 50);
    satie_param(P_SEND2, b[P_SEND2] * e[3][2] / 50);
    /* Bass and percussion stay mostly dry; their original small sends remain. */
    satie_param(P_SWING, b[P_SWING] + (int)e[2][1] - 50);
    satie_param(P_ETIME, e[5][0]);
    satie_param(P_ECHO, e[5][3] <= 50 ? proj.par[P_ECHO] * e[5][3] / 50 : proj.par[P_ECHO] + (100-proj.par[P_ECHO]) * (e[5][3]-50) / 50);
    satie_param(P_REVERB, e[6][0] <= 50 ? proj.par[P_REVERB] * e[6][0] / 50 : proj.par[P_REVERB] + (100-proj.par[P_REVERB]) * (e[6][0]-50) / 50);
    satie_param(P_SIZE, proj.par[P_SIZE] + (int)e[6][1] - 50);
    satie_param(P_RV_DAMP, b[P_RV_DAMP] + 50 - (int)e[6][2]);
    en_satie_edit(ST_LEAD_TONE,e[0][0]); en_satie_edit(ST_LEAD_CHAR,e[0][1]);
    en_satie_edit(ST_BASS_TONE,e[1][0]); en_satie_edit(ST_BASS_ATTACK,e[1][1]);
    en_satie_edit(ST_BASS_LENGTH,e[1][2]); en_satie_edit(ST_DRUM_TONE,e[2][0]);
    en_satie_edit(ST_DRUM_PATTERN,e[2][2]);
    en_satie_edit(ST_INNER_LEVEL,e[3][0] * e[3][3] / 50);
    en_satie_edit(ST_DELAY_FB,e[5][1]); en_satie_edit(ST_DELAY_TONE,e[5][2]);
    en_satie_edit(ST_WIDTH,e[6][3]);
    /* Vinyl is a new SP-303-inspired combination, not a circuit emulation.
     * Dust is independent of compression and pitch movement. */
    if (style) {
        fx |= FXB(FX_TAPE);
        if (e[4][3]) fx |= FXB(FX_VINYL);
    }
    fxs_put(EN_FXS_TP_WOW, style ? e[4][2] : 0);
    fxs_put(EN_FXS_TP_FLUT, style ? e[4][2] / 3 : 0);
    fxs_put(EN_FXS_TP_DRIVE, style == 3 ? amount : style ? amount / 3 : 0);
    fxs_put(EN_FXS_TP_DULL, style == 1 ? 20 + amount / 2 : style == 2 ? amount / 3 : style == 3 ? 30 : 0);
    fxs_put(EN_FXS_VN_LEVEL, style ? e[4][3] / 2 : 0);
    fxs_put(EN_FXS_VN_CRACK, 3 + e[4][3] / 5);
    fxs_put(EN_FXS_VN_HISS, 20 + e[4][3] / 3);
    fxs_put(EN_FXS_VN_TONE, 30);
    satie_param(P_MC_RATIO, style == 1 && amount ? 2 + amount / 25 : b[P_MC_RATIO]);
    satie_param(P_MC_THR, style == 1 && amount ? 65 - amount / 2 : b[P_MC_THR]);
    satie_param(P_MC_MAKE, style == 1 && amount ? 10 + amount / 10 : b[P_MC_MAKE]);
    en_fx(fx);
}

static int satie_knob_max(int page, int knob)
{
    return !page ? (knob ? 100 : 150) : page == 3 && knob == 2 ? 4 :
        page == 5 && knob == 0 ? 3 : page == 6 && knob == 0 ? 2 : 100;
}
static int satie_knob_value(int page, int knob)
{
    const satie_settings_t *s = &satie_ui.current;
    const int home[4] = {s->tempo,s->energy,s->evolve,s->space};
    return page ? s->edit[page-1][knob] : home[knob];
}
static const char *satie_knob_text(int page, int knob, char *number)
{
    const int value = satie_knob_value(page,knob);
    if (page == 3 && knob == 2) return SATIE_PATTERN[value];
    if (page == 5 && knob == 0) return SATIE_COLOUR[value];
    if (page == 6 && knob == 0) return SATIE_TIME[value];
    itoa_u(value,number); return number;
}

static void satie_apply(void)
{
    const satie_settings_t *settings = &satie_ui.current;
    const int genre = settings->mood == 4 ? 15 : settings->mood ? 2 : 16;
    const int16_t (*recipe)[2] = settings->mood == 4 ? SATIE_MOOD_4 : settings->mood == 3 ? SATIE_MOOD_3 : settings->mood == 2 ? SATIE_MOOD_2 : settings->mood ? SATIE_MOOD_1 : SATIE_MOOD_0;
    uint16_t patterns[EN_NPAT][EN_NLANE];
    uint8_t mods[EN_NLANE][16];
    int i, lane;
    en_freeze(0);
    /* Let sounding voices and effect tails decay through a requested change. */
    en_piece_g(genre, 0, proj.par, &proj.pool);
    for (i = 0; recipe[i][0] >= 0; i++)
        proj.par[recipe[i][0]] = recipe[i][1];
    proj.genre = (uint8_t)genre;
    proj.piece = 0;
    proj.par[P_ROOT] = SATIE_SOURCE[settings->source].root;
    proj.par[P_MODE] = SATIE_SOURCE[settings->source].mode;
    proj.pool = EN_MODE_POOL[proj.par[P_MODE]];
    proj.par[P_PAD_MAN] = proj.par[P_DR_MAN] = proj.par[P_AC_MAN] = proj.par[P_CM_MAN] = 1;
    proj.par[P_DD_CHANGE] = proj.par[P_DD_MUTATE] = 0;
    en_piece_pats_g(genre, (settings->seed - 1u) % EN_NPIECES, patterns);
    en_piece_mods_g(genre, 0, mods);
    if (settings->mood == 3) {
        /* Feathered kick, brush circles and foot taps. New track changes
         * the brush accents while keeping the acoustic kit restrained. */
        for (i = 0; i < EN_NPAT; i++) {
            patterns[i][0] = 0x0101;
            patterns[i][1] = (settings->seed & 1u) ? 0x5151 : 0x1551;
            patterns[i][2] = patterns[i][4] = patterns[i][5] = 0;
            patterns[i][3] = 0x1010;
        }
        for (lane = 0; lane < EN_NLANE; lane++)
            for (i = 0; i < 16; i++) mods[lane][i] = 0;
        /* Quiet off-beat brush strokes, delayed 5/12 of a sixteenth:
         * an approximately 60:40 eighth-note swing, with the beat fixed. */
        for (i = 2; i < 16; i += 4) mods[1][i] = (1u << 6) | (5u << 3);
    }
    if (settings->mood == 4) {
        static const uint16_t kicks[4] = {0x0421, 0x4081, 0x0501, 0x0481};
        /* A two-bar broken loop: gaps, displaced kicks and one skipped
         * backbeat answered by a soft off-beat snare. */
        for (i = 0; i < EN_NPAT; i++) {
            patterns[i][0] = kicks[(settings->seed + (unsigned)i) & 3u];
            patterns[i][1] = (i & 1) ? 0x1040 : 0x1010;
            patterns[i][2] = patterns[i][4] = patterns[i][5] = 0;
            patterns[i][3] = (i & 1) ? 0x4414 : 0x4485;
        }
        for (lane = 0; lane < EN_NLANE; lane++)
            for (i = 0; i < 16; i++) mods[lane][i] = 0;
        for (i = 0; i < 16; i++) {
            mods[1][i] = (3u << 3) | (i == 6 ? 1u << 6 : 0u);
            mods[3][i] = (i & 3u) ? 1u << 6 : 2u << 6;
        }
    }
    for (i = 0; i < EN_NPAT; i++)
        for (lane = 0; lane < EN_NLANE; lane++)
            en_pats[i][lane] = patterns[i][lane];
    for (lane = 0; lane < EN_NLANE; lane++)
        for (i = 0; i < 16; i++)
            en_mod[lane][i] = mods[lane][i];
    memcpy(satie_ui.base, proj.par, sizeof satie_ui.base);
    for (i = 0; i < P_NPARAMS; i++)
        en_set(i, proj.par[i]);
    en_pool(proj.pool);
    en_genre(genre);
    en_trim(genre, -1);
    en_trk(0, 0);
    en_intro(0);
    ui.fx_lock = 0;
    genre_fx(genre);
    if (settings->mood == 1 || settings->mood == 2) ui.fx_lock &= ~FXB(FX_PUMP);
    if (settings->mood == 3) ui.fx_lock = 0;
    if (settings->mood == 4) {
        fxs_put(EN_FXS_VN_LEVEL, 10);
        fxs_put(EN_FXS_VN_CRACK, 3);
        fxs_put(EN_FXS_VN_HISS, 12);
        fxs_put(EN_FXS_TP_WOW, 20);
        fxs_put(EN_FXS_TP_FLUT, 10);
        fxs_put(EN_FXS_TP_DRIVE, 18);
        fxs_put(EN_FXS_TP_DULL, 55);
    }
    en_fx(ui.fx_lock);
    satie_macros();
    en_seed(settings->seed);
    en_regrow();
    en_satie_start(settings->source, settings->seed);
    en_satie_arrangement(satie_ui.mix_position);
    satie_ui.pending = 0;
}

static void satie_request(void)
{
    const int mood_only = satie_ui.requested.mood != satie_ui.current.mood &&
        satie_ui.requested.source == satie_ui.current.source &&
        satie_ui.requested.seed == satie_ui.current.seed;
    satie_ui.pending = mood_only ? SATIE_PENDING_MOOD : SATIE_PENDING_TRACK;
    say(mood_only ? "NEW MOOD" : "NEW TRACK", en_playing ? "NEXT BAR" : "READY");
}

/* A single composer: SEL goes straight to collections, then tunes. */
static int satie_browser_count(void)
{
    return satie_ui.browser == 1 ? SATIE_COLLECTIONS :
        SATIE_COLLECTION[satie_ui.browser_collection].count;
}

static void satie_browser_open(void)
{
    satie_ui.browser = 1;
    satie_ui.browser_collection = SATIE_SOURCE[satie_ui.requested.source].collection;
    satie_ui.browser_cursor = satie_ui.browser_collection;
}

static void satie_browser_input(uint32_t pressed, int movement)
{
    if (pressed & (1u << B_HOME)) {
        if (satie_ui.browser == 2) {
            satie_ui.browser = 1;
            satie_ui.browser_cursor = satie_ui.browser_collection;
        } else satie_ui.browser = 0;
        return;
    }
    satie_ui.browser_cursor = (uint8_t)satie_clamp(satie_ui.browser_cursor + movement, 0, satie_browser_count() - 1);
    if (!(pressed & (1u << B_SEL))) return;
    if (satie_ui.browser == 1) {
        const int group = satie_ui.browser_cursor;
        satie_ui.browser_collection = (uint8_t)group;
        satie_ui.browser_cursor = SATIE_SOURCE[satie_ui.requested.source].collection == group ?
            satie_ui.requested.source - SATIE_COLLECTION[group].first : 0;
        satie_ui.browser = 2;
    } else {
        const int source = SATIE_COLLECTION[satie_ui.browser_collection].first + satie_ui.browser_cursor;
        satie_ui.browser = 0;
        if (source != satie_ui.requested.source) {
            satie_ui.requested.source = (uint8_t)source;
            satie_ui.requested.tempo = (uint8_t)satie_tempo(&satie_ui.requested);
            satie_request();
        }
    }
}

static void satie_recall(void)
{
    satie_settings_t saved;
    satie_ui.help = 0;
    if (satie_load(&saved)) {
        satie_ui.requested = saved;
        satie_request();
    } else say("NO SAVED TRACK", "SAVE TO KEEP THIS ONE");
}

static void satie_input(void)
{
    uint32_t buttons = plat_buttons(), pressed = buttons & ~satie_ui.buttons;
    int enc[NE], i;
    satie_ui.buttons = buttons;
    for (i = 0; i < NE; i++)
        enc[i] = (int)plat_enc(i);
    midi_in();
    if (ui.splash || satie_ui.store_wait)
        return;
    if (pressed & (1u << B_PLAY))
        en_play(!en_playing);
    if (pressed & (1u << B_REC))
        en_freeze(!en_frozen);
    if (pressed & (1u << B_GLO)) {
        satie_ui.help = satie_ui.help ? 0 : 1;
        satie_ui.browser = 0;
        return;
    }
    if (satie_ui.help) {
        if (pressed & (1u << B_HOME)) satie_ui.help = 0;
        else {
            satie_ui.help = (uint8_t)satie_clamp(satie_ui.help + enc[EN_SELECT] + enc[EN_ALGO], 1, SATIE_HELP_PAGES);
            if (pressed & (1u << B_SEL)) {
                if (satie_ui.help == SATIE_HELP_PAGES) satie_recall();
                else satie_ui.help++;
            }
        }
        return; /* Reading the guide must never change sounds or overwrite Keep. */
    }
    if (satie_ui.browser) {
        satie_browser_input(pressed, enc[EN_SELECT] + enc[EN_PRESET]);
        return;
    }
    if (pressed & (1u << B_SEL)) {
        satie_browser_open();
        return;
    }
    if (pressed & (1u << B_HOME)) satie_ui.page = 0;
    if (enc[EN_ALGO]) satie_ui.page = (uint8_t)satie_clamp(satie_ui.page + enc[EN_ALGO], 0, SATIE_PAGES-1);
    if (pressed & (1u << B_FX)) satie_ui.page = satie_ui.page < 5 || satie_ui.page == 7 ? 5 : satie_ui.page + 1;
    if ((pressed & (1u << B_EDIT)) && satie_ui.page) {
        memcpy(satie_ui.current.edit[satie_ui.page-1], SATIE_EDIT_DEFAULT[satie_ui.page-1], 4);
        memcpy(satie_ui.requested.edit[satie_ui.page-1], SATIE_EDIT_DEFAULT[satie_ui.page-1], 4);
        say("PAGE RESET", "");
    } else if (pressed & (1u << B_EDIT)) {
        satie_ui.requested.seed = (uint16_t)(satie_ui.requested.seed + 251u);
        if (!satie_ui.requested.seed) satie_ui.requested.seed = 1;
        satie_request();
    }
    if (enc[EN_PRESET]) {
        satie_ui.requested.source = (uint8_t)satie_clamp(satie_ui.requested.source + enc[EN_PRESET], 0, SATIE_SOURCES - 1);
        satie_ui.requested.tempo = (uint8_t)satie_tempo(&satie_ui.requested);
        satie_request();
    }
    if (enc[EN_SELECT]) {
        satie_ui.requested.mood = (uint8_t)satie_clamp(satie_ui.requested.mood + enc[EN_SELECT], 0, SATIE_MOODS - 1);
        satie_ui.requested.tempo = (uint8_t)satie_tempo(&satie_ui.requested);
        satie_request();
    }

    for (i = 0; i < 4; i++) {
        uint8_t *values[4] = {&satie_ui.current.tempo, &satie_ui.current.energy,
                            &satie_ui.current.evolve, &satie_ui.current.space};
        uint8_t *wanted[4] = {&satie_ui.requested.tempo, &satie_ui.requested.energy,
                            &satie_ui.requested.evolve, &satie_ui.requested.space};
        if (satie_ui.page) {
            values[i] = &satie_ui.current.edit[satie_ui.page-1][i];
            wanted[i] = &satie_ui.requested.edit[satie_ui.page-1][i];
        }
        if (enc[EN_K1 + i]) {
            *values[i] = (uint8_t)satie_clamp(*values[i] + enc[EN_K1 + i],
                !satie_ui.page && !i ? 50 : 0, satie_knob_max(satie_ui.page,i));
            *wanted[i] = *values[i];
            if (!satie_ui.page) satie_macros();
        }
    }

    if (pressed & (1u << B_SAVE)) {
        if (satie_ui.pending) {
            say(satie_ui.pending == SATIE_PENDING_MOOD ? "WAIT FOR NEW MOOD" : "WAIT FOR NEW TRACK",
                "THEN PRESS SAVE");
        } else if (!plat_store_locked()) {
            satie_ui.saving = satie_ui.current;
            satie_ui.store_wait = 1;
            en_hush(1);
            say("SAVING", "");
        }
    }
}

static void satie_ui_tick(void)
{
    const uint32_t bar = en_bars >> 4;
    const uint32_t now = plat_ms();
    if (now - satie_ui.mix_ms >= 8u) {
        const int step = satie_clamp((int)((now - satie_ui.mix_ms) / 8u), 1, 8);
        satie_ui.mix_ms = now;
        int changed = 0, p, k;
        for (p = 0; p < 7; p++) for (k = 0; k < 4; k++) {
            const int target = satie_ui.current.edit[p][k];
            if (satie_ui.live[p][k] != target) {
                if (satie_knob_max(p+1,k) < 100) satie_ui.live[p][k] = (uint8_t)target;
                else satie_ui.live[p][k] += satie_clamp(target - satie_ui.live[p][k], -step, step);
                changed = 1;
            }
        }
        if (satie_ui.mix_position != satie_ui.current.space) {
            satie_ui.mix_position += satie_clamp((int)satie_ui.current.space - satie_ui.mix_position, -step, step);
            changed = 1;
        }
        if (changed) satie_macros();
    }
    if (satie_ui.store_wait && en_hushed) {
        const int result = plat_store_save(OBJ_SATIE, &satie_ui.saving, sizeof satie_ui.saving);
        satie_ui.store_wait = 0;
        en_hush(0);
        say(result ? "SAVE FAILED" : "TRACK SAVED", result ? "TRY SAVE AGAIN" : "GLO: KEEP AND RECALL");
    }
    if (satie_ui.pending && (!en_playing || bar != satie_ui.last_bar)) {
        satie_ui.current = satie_ui.requested;
        satie_apply();
    }
    satie_ui.last_bar = bar;
}

static void satie_ui_init(void)
{
    const satie_settings_t defaults = {SATIE_MAGIC, 4, 1, 2, 0, 60, 50, 35, 42, 2, 0, {{0}}};
    memset(&satie_ui, 0, sizeof satie_ui);
    satie_ui.current = defaults;
    if (!satie_load(&satie_ui.current)) {
        satie_ui.current = defaults;
        memcpy(satie_ui.current.edit, SATIE_EDIT_DEFAULT, sizeof satie_ui.current.edit);
    }
    memcpy(satie_ui.live, satie_ui.current.edit, sizeof satie_ui.live);
    satie_ui.requested = satie_ui.current;
    satie_ui.mix_position = satie_ui.current.space;
    satie_ui.mix_ms = plat_ms();
    ui.dj_on = ui.saver_on = ui.saver = 0;
    fxs_put(EN_FXS_AF_AMT, 0);
    ui.view = V_TEMPO;
    satie_apply();
}

static void satie_browser_draw(void)
{
    const uint16_t bg = RGB(12, 17, 23), ink = RGB(226, 233, 229);
    const uint16_t dim = RGB(124, 145, 151), accent = RGB(111, 220, 191);
    const int start = satie_ui.browser_cursor < 5 ? 0 : satie_ui.browser_cursor - 4;
    const int count = satie_browser_count();
    const char *heading = satie_ui.browser == 1 ? "COLLECTION" : "TUNE";
    int band, row;
    for (band = 0; band < 2; band++) {
        cv_begin(240, 120, bg);
        if (!band) {
            cv_text(12, 8, &FONT_B, "CHOOSE", ink);
            cv_text(82, 8, &FONT_B, heading, accent);
            cv_text(12, 34, &FONT_XS, satie_ui.browser == 1 ? "ERIK SATIE" : SATIE_COLLECTION[satie_ui.browser_collection].title, dim);
        }
        for (row = band ? 2 : 0; row < (band ? 5 : 2); row++) {
            const int item = start + row, y = band ? (row - 2) * 26 : 60 + row * 26;
            const char *label;
            const int selected = item == satie_ui.browser_cursor;
            if (item >= count) break;
            if (satie_ui.browser == 1) label = SATIE_COLLECTION[item].title;
            else label = SATIE_SOURCE[SATIE_COLLECTION[satie_ui.browser_collection].first + item].title;
            if (selected) cv_rect(8, y - 2, 224, 23, RGB(37, 65, 59));
            cv_text(12, y, &FONT_B, selected ? ">" : " ", accent);
            cv_text(27, y, &FONT_B, label, selected ? ink : dim);
        }
        if (band) {
            cv_text(12, 84, &FONT_XS, "TURN SELECT   SEL CHOOSE", accent);
            cv_text(12, 102, &FONT_XS, "HOME BACK   MUSIC CONTINUES", dim);
        }
        cv_blit(0, band * 120);
    }
    plat_glow(1);
}

static void satie_help_draw(void)
{
    const uint16_t bg = RGB(12,17,23), ink = RGB(226,233,229), accent = RGB(183,182,244);
    const int page = satie_ui.help-1;
    char number[8];
    int band, row;
    for (band = 0; band < 2; band++) {
        cv_begin(240,120,bg);
        if (!band) {
            cv_text(12,8,&FONT_B,"SATIE-1 GUIDE",accent);
            itoa_u(page+1,number); cv_text(184,10,&FONT_XS,number,ink);
            cv_text(198,10,&FONT_XS,"/8",ink);
            cv_text(12,34,&FONT_B,SATIE_HELP[page].title,ink);
        }
        for (row = band ? 3 : 0; row < (band ? 6 : 3); row++)
            cv_text(12,66 + row*22 - band*120,&FONT_XS,SATIE_HELP[page].line[row],ink);
        if (band) {
            cv_text(12,78,&FONT_XS,"TURN SELECT / ALGO",accent);
            cv_text(12,99,&FONT_XS,page == SATIE_HELP_PAGES-1 ? "HOME CLOSE  SEL RECALL" : "HOME CLOSE  SEL NEXT",accent);
        }
        cv_blit(0,band*120);
    }
    plat_glow(1);
}

static void satie_startup_draw(void)
{
    unsigned band, pixel;
    for (band = 0; band < 2; band++) {
        cv_begin(240,120,0);
        for (pixel = 0; pixel < 240u*120u; pixel++)
            cv_px[pixel] = swap16(SATIE_ART_PALETTE[SATIE_ART[band*240u*120u+pixel]]);
        cv_blit(0,band*120);
    }
    plat_glow(1);
}

static void satie_draw(void)
{
    const satie_settings_t *settings = &satie_ui.current;
    const uint16_t bg = RGB(12, 17, 23), ink = RGB(226, 233, 229);
    const uint16_t dim = RGB(124, 145, 151), accent = settings->mood == 2 ? RGB(183, 182, 244) : settings->mood ? RGB(237, 189, 110) : RGB(111, 220, 191);
    char number[16];
    int i;
    if (ui.frame % 4u)
        return;
    if (satie_ui.help) {
        satie_help_draw();
        return;
    }
    if (satie_ui.browser) {
        satie_browser_draw();
        return;
    }
    cv_begin(240, 120, bg);
    cv_text(12, 8, &FONT_B, "SATIE-1", ink);
    cv_text(166, 10, &FONT_XS, en_playing ? "PLAYING" : "PAUSED", accent);
    cv_rect(12, 30, 216, 1, dim);
    {
        const char *title = SATIE_SOURCE[settings->source].title;
        const felucca_font_t *font = text_w(&FONT_M, title) <= 216 ? &FONT_M : &FONT_B;
        cv_text(12, 38, font, title, ink);
    }
    cv_text(12, 68, &FONT_S, SATIE_MOOD_NAME[settings->mood], accent);
    cv_text(228 - text_w(&FONT_XS, SATIE_VOICE_NAME[en_satie_voices]), 70,
            &FONT_XS, SATIE_VOICE_NAME[en_satie_voices], dim);
    cv_text(12, 89, &FONT_XS, SATIE_COLLECTION[SATIE_SOURCE[settings->source].collection].title, dim);
    itoa_u(settings->seed, number);
    cv_text(140, 89, &FONT_XS, "TRACK", dim);
    cv_text(183, 89, &FONT_XS, number, ink);
    cv_blit(0, 0);

    cv_begin(240, 120, bg);
    if (!satie_ui.page) {
    cv_text(12, 0, &FONT_B, en_frozen ? "HOLDING" : !en_satie_section ? "THEME" : "VARIATION", accent);
    itoa_u(en_satie_section, number);
    if (en_satie_section) cv_text(110, 0, &FONT_B, number, ink);
    for (i = 0; i < 8; i++)
        cv_rect(143 + i * 11, 5, 7, 7, i == en_satie_section ? accent : RGB(43, 61, 67));
    } else {
        cv_text(12, 0, &FONT_B, SATIE_PAGE[satie_ui.page], accent);
        itoa_u(satie_ui.page+1,number);
        cv_text(180,0,&FONT_B,number,ink); cv_text(195,0,&FONT_B,"/8",dim);
    }
    cv_rect(12, 25, 216, 2, RGB(43, 61, 67));
    cv_rect(12, 25, 216 * en_satie_step / (en_satie_length ? en_satie_length : 1), 2, accent);
    for (i = 0; i < 4; i++) {
        const char *value = satie_knob_text(satie_ui.page,i,number);
        text_c(30 + i * 60, 37, &FONT_XS, SATIE_LABEL[satie_ui.page][i], dim);
        text_c(30 + i * 60, 54, text_w(&FONT_M,value) < 56 ? &FONT_M : &FONT_XS, value, ink);
    }

    if (satie_ui.pending)
        text_c(120, 90, &FONT_XS, satie_ui.pending == SATIE_PENDING_MOOD ?
            "NEW MOOD ON NEXT BAR" : "NEW TRACK ON NEXT BAR", accent);
    else if (plat_ms() < ui.msg_until)
        text_c(120, 90, &FONT_XS, ui.msg[0], accent);
    else
        text_c(120, 90, &FONT_XS, satie_ui.page ? "HOME BACK   GLO GUIDE" : "SEL TUNES   GLO GUIDE", dim);
    cv_blit(0, 120);
    plat_glow(1);
    plat_leds((en_playing ? 1u << B_PLAY : 0u) | (en_frozen ? 1u << B_REC : 0u),
              1u << WHITE_KEY[(en_satie_step / 8u) % 16u]);
}
