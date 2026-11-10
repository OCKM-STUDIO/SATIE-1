/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise retriggers, kit changes and small render chunks under sanitizers.
 * DSP_SOURCE can point at the previous source to compare the output checksum. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifndef DSP_SOURCE
#define DSP_SOURCE "../firmware/src/dsp/amb.c"
#endif
#include DSP_SOURCE
void en_midi_out(uint32_t a, uint32_t b, uint32_t c) { (void)a; (void)b; (void)c; }

int main(void)
{
    const unsigned sizes[] = {1, 17, 63, 128, 255, 256};
    uint32_t hash = 2166136261u, rng = 54321u, blocks = 0;
    volatile uint32_t trace = 0;
    en_init();
    en_satie_trace = &trace;
    pulse_vol = 1.0f;
    for (int t = 0; t < EN_NTRACK; t++) trk_g[t] = trk_g1[t] = 1.0f;
    for (int kit = 0; kit < EN_NKIT; kit++) {
        memset(drum, 0, sizeof drum);
        for (unsigned block = 0; block < 2400; block++) {
            unsigned n = sizes[block % 6];
            /* Change kit with ringing voices; include both brushed and pitched drums. */
            int sound = block % 31 == 0 ? (kit + 1) % EN_NKIT : kit;
            KK = KS = KH = KP = &KITS[sound];
            par[P_KIT_SNARE] = par[P_KIT_HAT] = sound;
            for (int d = 0; d < EN_NDRUM; d++) {
                lane_g[d] = 1.0f;
                lane_tune[d] = block % 3 == 0 ? -24.0f : block % 3 == 1 ? 24.0f : 0.0f;
                lane_dec[d] = block % 2 ? 1.0f : 0.0f;
                rng = rng * 1664525u + 1013904223u;
                if (block == 0 || (rng >> 28) < 3)
                    drum_fire(d, 1.0f);
            }
            memset(mixl, 0, sizeof mixl); memset(mixr, 0, sizeof mixr);
            memset(rsend, 0, sizeof rsend); memset(esend, 0, sizeof esend);
            render_drums(n);
            for (unsigned j = 0; j < n; j++) {
                uint32_t bits;
                assert(isfinite(mixl[j]) && isfinite(mixr[j]));
                assert(isfinite(rsend[j]) && isfinite(esend[j]));
                memcpy(&bits, &mixl[j], sizeof bits);
                hash = (hash ^ bits) * 16777619u;
            }
            for (int d = 0; d < EN_NDRUM; d++) {
                assert(isfinite(drum[d].x1) && isfinite(drum[d].x2));
                assert(isfinite(drum[d].ph) && isfinite(drum[d].ph2));
                assert(drum[d].ph >= 0.0f && drum[d].ph < 1.0f);
            }
            blocks++;
        }
    }
    printf("%u drum blocks, checksum %08x\n", blocks, hash);
    return 0;
}
