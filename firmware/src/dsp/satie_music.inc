/* SPDX-License-Identifier: GPL-3.0-only */
/* Audio-clock score player. Commands arrive through the engine queue. */
volatile uint8_t en_satie_source, en_satie_section, en_satie_voices;
volatile uint16_t en_satie_seed, en_satie_step, en_satie_length;
static uint8_t satie_on, satie_bass_note, satie_pad_note, satie_answer_note;
static uint16_t satie_evolve_steps, satie_transition;
static uint16_t satie_hold_start, satie_hold_end;
static uint8_t satie_was_frozen;
static uint8_t satie_answer_voice;

static void satie_reset(void)
{
    satie_on = satie_bass_note = satie_pad_note = satie_answer_note = 0;
    en_satie_source = en_satie_section = en_satie_step = 0;
    en_satie_seed = 1;
    en_satie_voices = 2;
    satie_inner_gain = 1.0f;
    satie_inner_trigger = 0;
    en_satie_length = SATIE_SOURCE[0].steps;
    satie_evolve_steps = satie_transition = 0;
    satie_answer_voice = EN_V_ACID;
    satie_hold_start = satie_hold_end = satie_was_frozen = 0;
}

static void satie_release(void)
{
    if (satie_bass_note) note_cmd(EN_V_DRONE, satie_bass_note, 0);
    if (satie_pad_note) note_cmd(EN_V_PAD, satie_pad_note, 0);
    if (satie_answer_note) note_cmd(satie_answer_voice, satie_answer_note, 0);
    satie_bass_note = satie_pad_note = satie_answer_note = 0;
}

static void satie_voicing(int voices)
{
    en_satie_voices = (uint8_t)(voices < 0 ? 0 : voices > 2 ? 2 : voices);
    if (!en_satie_voices) {
        if (satie_bass_note) note_cmd(EN_V_DRONE, satie_bass_note, 0);
        if (satie_pad_note) note_cmd(EN_V_PAD, satie_pad_note, 0);
        satie_bass_note = satie_pad_note = 0;
    }
    if (en_satie_voices < 2 && satie_answer_note) {
        note_cmd(satie_answer_voice, satie_answer_note, 0);
        satie_answer_note = 0;
    }
}

static void satie_start(int source, unsigned seed)
{
    satie_release();
    satie_reset();
    en_satie_source = (uint8_t)((unsigned)source < SATIE_SOURCES ? source : 0);
    en_satie_seed = (uint16_t)(seed ? seed : 1);
    en_satie_length = SATIE_SOURCE[en_satie_source].steps;
    satie_on = 1;
}

static uint32_t satie_hash(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    return value;
}

static void satie_step(void)
{
    const satie_source_t *score;
    unsigned step = en_satie_step, section, variation;
    int note, bass_note, amount;
    if (!satie_on) return;
    if (!par[P_EVOLVE]) en_satie_section = 0;
    /* Full scores continue; treatments develop every eight dance bars without
     * rewinding or clearing sounding voices. */
    if (satie_evolve_steps >= 128) {
        satie_evolve_steps = 0;
        if (!en_frozen && par[P_EVOLVE]) {
            en_satie_section = (uint8_t)((en_satie_section + 1u) % 8u);
            satie_transition = 0;
        }
    }
    section = en_satie_section;
    score = &SATIE_SOURCE[en_satie_source];
    en_satie_length = score->steps;
    if (en_frozen && !satie_was_frozen) {
        satie_hold_start = (uint16_t)(step / 128u * 128u);
        satie_hold_end = (uint16_t)(satie_hold_start + 128u);
        if (satie_hold_end > score->steps) satie_hold_end = score->steps;
    }
    satie_was_frozen = en_frozen;
    /* Bring transformations in over one bar; never mute the master or clear
     * delay/reverb at a section boundary. The underlying clock keeps running. */
    amount = par[P_EVOLVE] * (int)(satie_transition < 16 ? satie_transition : 16) / 16;
    variation = section && amount ? 1u + (section - 1u + en_satie_seed % 6u) % 6u : 0;
    bass_note = score->bass[step];
    if (en_satie_voices && !(step & 3u)) {
        if (bass_note) {
            /* Each quarter is a new pluck, including repeated score tones.
             * Keep the old key only until the new one owns the bass voice. */
            note_cmd(EN_V_DRONE, bass_note, (step & 7u) ? 68 : 78);
            if (satie_bass_note && satie_bass_note != bass_note) note_cmd(EN_V_DRONE, satie_bass_note, 0);
            satie_bass_note = (uint8_t)bass_note;
        } else if (!bass_note && step >= 16 && step + 16u < score->steps && satie_bass_note) {
            note_cmd(EN_V_DRONE, satie_bass_note, 0);
            satie_bass_note = 0;
        }
        if (bass_note && !(step & 15u) && bass_note != satie_pad_note) {
            /* The outgoing pad releases underneath the incoming pad's attack. */
            note_cmd(EN_V_PAD, bass_note, 64);
            if (satie_pad_note) note_cmd(EN_V_PAD, satie_pad_note, 0);
            satie_pad_note = (uint8_t)bass_note;
        }
    }
    {
        const int change = variation && (int)(satie_hash(step + 131u * en_satie_seed + 977u * section) % 100u) < amount;
        note = score->melody[(step + (variation == 4 && change ? 1u : 0u)) % score->steps];
        if (note && change) {
            if (variation == 1 && (step & 3u)) note = 0;
            if (variation == 2) note += 12;
            if (variation == 5) note -= 12;
            if (variation == 6) note += (step & 8u) ? 12 : -12;
        }
    }
    if (note > 0)
        note_cmd(EN_V_PLUCK, fold_note(note, 36, 96), (step & 3u) ? 66 : 86);
    /* Third musical part: actual middle chord tones where available; otherwise
     * a quiet, generated two-beat answer using the source's own melody notes.
     * It is intentionally separate from the lead's evolution amount. */
    if (en_satie_voices == 2 && !(step & 7u)) {
        int answer = score->inner[step];
        if (!answer && step >= 8) {
            unsigned look;
            for (look = 0; look < 4 && !answer; look++)
                answer = score->melody[step - 8u + look];
        }
        if (answer) {
            answer = fold_note(answer, 48, 84);
            if (satie_answer_note != answer) {
                if (satie_answer_note) note_cmd(satie_answer_voice, satie_answer_note, 0);
                satie_answer_voice = par[P_KIT_SNARE] == EN_KIT_BRUSH ? EN_V_PLUCK : EN_V_ACID;
                satie_inner_trigger = 1;
                note_cmd(satie_answer_voice, answer, satie_answer_voice == EN_V_PLUCK ? 38 : 48);
                satie_inner_trigger = 0;
                satie_answer_note = (uint8_t)answer;
            }
        } else if (satie_answer_note) {
            note_cmd(satie_answer_voice, satie_answer_note, 0);
            satie_answer_note = 0;
        }
    }
    en_satie_step = (uint16_t)((step + 1u) % score->steps);
    if (en_frozen && step + 1u >= satie_hold_end)
        en_satie_step = satie_hold_start;
    satie_evolve_steps++;
    if (satie_transition < 16) satie_transition++;
}
