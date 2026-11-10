/* SPDX-License-Identifier: GPL-3.0-only */
/* A completed worker job must release the stage wait even if its stage marker
 * is missing. Compare the audio with a normal completed job. */
#include <stdio.h>
#include <stdlib.h>
#include "../firmware/src/dsp/amb.c"
void en_midi_out(uint32_t a, uint32_t b, uint32_t c) { (void)a; (void)b; (void)c; }
static int missing;
static unsigned joins;
static int submit_job(void (*fn)(void *), void *ctx)
{
    fn(ctx);
    if (missing == 2) stage2_done = 0;
    if (missing == 3) stage3_done = 0;
    return 1;
}
static int join_job(void) { joins++; return 1; }
static int alive_job(void) { return 0; } /* deadline passed, but join confirms completion */
static void fence_job(void) {}
int main(int argc, char **argv)
{
    int32_t out[512];
    uint32_t hash = 2166136261u;
    missing = argc > 1 ? atoi(argv[1]) : 0;
    en_init();
    en_core2_attach(submit_job, join_job, fence_job, alive_job);
    en_balance_force(2);
    en_play(1);
    for (int block = 0; block < 100; block++) {
        en_render(out, 256, 2048);
        for (int i = 0; i < 512; i++) hash = (hash ^ (uint32_t)out[i]) * 16777619u;
    }
    if (!joins || en_c2_lost) return 1;
    printf("%08x\n", hash);
    return 0;
}
