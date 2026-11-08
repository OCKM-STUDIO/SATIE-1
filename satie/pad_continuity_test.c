/* SPDX-License-Identifier: GPL-3.0-only */
/* A held pad must sound identical when MIDI/clock events split audio blocks. */
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/dsp/amb.c"
void en_midi_out(uint32_t s, uint32_t n, uint32_t v) { (void)s;(void)n;(void)v; }
#define COUNT 8192
static float reference[COUNT];
static void setup(int sound) {
    en_init();
    memset(pad,0,sizeof pad);
    par[P_PADTONE]=sound;par[P_PAD_MAN]=1;par[P_PL_CHO]=0;
    pad_vol=0.8f;pad_gain=1.0f;motion=0;arcm=0;bright=28;
    pad_pan=pad_pitch=pad_filt=0;
    pad[0].voices=PAD_SOUND[sound].osc;pad[0].play=1;
    pad[0].note=pad[0].target=60;pad[0].pamp=.7f;pad[0].hf=1;
}
int main(void) {
    const int sounds[]={0,1,2,3,5};
    const unsigned splits[]={1,17,31,47,63,127};
    int failures=0;
    /* Both lifecycle paths must clear every voice, without writing past it. */
    en_init();
    for(unsigned i=0;i<EN_NPAD;i++) {
        pad[i].ctl_target=pad[i].ctl_index=pad[i].ctl_attack=1.0f;
        pad_ctl[i]=17;
    }
    quiet();
    for(unsigned i=0;i<EN_NPAD;i++) {
        assert(pad[i].ctl_target==0 && pad[i].ctl_index==0 && pad[i].ctl_attack==0 && pad_ctl[i]==0);
        pad[i].ctl_target=pad[i].ctl_index=pad[i].ctl_attack=1.0f;
        pad_ctl[i]=17;
    }
    en_init();
    for(unsigned i=0;i<EN_NPAD;i++)
        assert(pad[i].ctl_target==0 && pad[i].ctl_index==0 && pad[i].ctl_attack==0 && pad_ctl[i]==0);
    for(unsigned s=0;s<sizeof sounds/sizeof *sounds;s++) {
        setup(sounds[s]);
        for(unsigned i=0;i<COUNT;i+=256) {
            render_pad(256);
            memcpy(reference+i,padl,256*sizeof(float));
        }
        for(unsigned k=0;k<sizeof splits/sizeof *splits;k++) {
            float maxdiff=0;
            setup(sounds[s]);
            for(unsigned i=0;i<COUNT;) {
                unsigned n=splits[k];if(n>COUNT-i)n=COUNT-i;
                render_pad(n);
                for(unsigned j=0;j<n;j++) {
                    float d=fabsf(reference[i+j]-padl[j]);
                    if(!isfinite(padl[j]))return 2;
                    if(d>maxdiff)maxdiff=d;
                }
                i+=n;
            }
            printf("Pad %d, %u-sample blocks: peak difference %.8f\n",sounds[s],splits[k],(double)maxdiff);
            failures+=maxdiff>1e-6f;
        }
    }
    return failures?1:0;
}
