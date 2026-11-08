/* SPDX-License-Identifier: GPL-3.0-only */
/* Audible bass articulation through the public engine API, including held
 * keys, repeated pitches and release. No score or other instrument masks it. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "amb.h"

static unsigned bass_ons;
static unsigned bass_note;
void en_midi_out(uint32_t status, uint32_t note, uint32_t velocity)
{
    if (status == 0x92 && velocity) { bass_ons++; bass_note = note; }
}

static float rms(float seconds)
{
    int32_t audio[512];
    float sum = 0.0f;
    int b, j, blocks = (int)(seconds * EN_SR / 256.0f);
    for (b = 0; b < blocks; b++) {
        en_render(audio, 256, 2048);
        for (j = 0; j < 512; j++) {
            float x = (float)audio[j] / 8388608.0f;
            assert(isfinite(x) && fabsf(x) < 0.99f);
            sum += x * x;
        }
    }
    return sqrtf(sum / (float)(blocks * 512));
}

int main(void)
{
    const int tempos[] = {50, 84, 150}, keys[] = {52, 60, 79};
    int t, k, sub;
    for (sub = 0; sub < 2; sub++)
    for (t = 0; t < 3; t++) for (k = 0; k < 3; k++) {
        float attack, tail, repeat, release;
        unsigned ons;
        en_init();
        en_set(P_PAD, 0); en_set(P_ACID, 0); en_set(P_CM_LVL, 0);
        en_set(P_PULSE, 0); en_set(P_NOISE, 0);
        en_set(P_REVERB, 0); en_set(P_ECHO, 0);
        en_set(P_DR_MAN, 1); en_set(P_BASS, 70);
        en_set(P_DR_INST, sub ? EN_SATIE_DUB_BASS : 1);
        en_set(P_TEMPO, tempos[t]);
        rms(0.10f);
        ons = bass_ons;
        en_note_v(EN_V_DRONE, keys[k], 78);
        rms(0.02f);
        attack = rms(0.10f);
        rms(0.68f);
        tail = rms(0.20f);               /* still held: must decay anyway */
        assert(bass_note >= 28 && bass_note <= (sub ? 43u : 55u));
        assert(attack > 0.005f && tail < attack * (sub ? 0.30f : 0.06f));
        if (sub) {
            assert(tail > attack * 0.01f); /* rounded decay, distinct from the pluck */
            rms(3.0f);
            assert(rms(0.20f) < attack * 0.002f); /* no held drone */
        }
        en_note_v(EN_V_DRONE, keys[k], 78);
        rms(0.02f);
        repeat = rms(0.10f);
        assert(repeat > attack * 0.75f && bass_ons == ons + 2);
        en_note_v(EN_V_DRONE, keys[k], 0);
        rms(sub ? 0.50f : 0.25f);
        release = rms(0.10f);
        assert(release < attack * 0.005f);
        if (sub) {
            /* Overlapping pitches exercise glide, then a mood switch preserves
             * the old tail and the new upright's articulation. */
            en_note_v(EN_V_DRONE, 52, 78); rms(0.12f);
            en_note_v(EN_V_DRONE, 67, 78); en_note_v(EN_V_DRONE, 52, 0);
            assert(rms(0.12f) > 0.005f);
            en_set(P_DR_INST, 1);
            en_note_v(EN_V_DRONE, 60, 78); en_note_v(EN_V_DRONE, 67, 0);
            assert(rms(0.12f) > 0.005f);
            en_note_v(EN_V_DRONE, 60, 0); rms(0.60f);
            assert(rms(0.10f) < attack * 0.005f);
        }
        printf("%s %d BPM key %d: held tail %.1f dB below attack; retrigger and release OK\n",
               sub ? "sub" : "upright", tempos[t], keys[k], (double)(20.0f * log10f(tail / attack)));
    }
    puts("upright and dub sub bass: OK");
    return 0;
}
