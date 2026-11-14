# SATIE-1 — Erik. Evolved.

![SATIE-1 cover](satie/art/card.jpg)

**An evolving Satie soundtrack for the M-VAVE FM-1.**

By [OCKM Studio](https://github.com/OCKM-STUDIO).

[Download the beta](https://github.com/OCKM-STUDIO/SATIE-1/releases/tag/v0.1.3-beta)
· [Play in your browser](https://ockm-studio.github.io/SATIE-1/)
· [Quick guide](satie/QUICK-START.txt)

I've kept it simple: choose a tune, choose a mood, and let the music evolve while
you work. The four Home knobs are enough to make it yours. Instrument and FX
pages are there when you want to go further; **GLO opens the built-in guide**.

Ten complete score reductions: **Gymnopédies 1–3, Gnossiennes 1–3 and Ogives 1–4**.
Five moods: **After Hours, Ibiza Chill, Orbit, Blue Note and Ghost**. The lead,
bass and optional inner voice follow the scores, with electronic drums and
atmosphere. These are simplified electronic arrangements, with sixteenth-note
quantisation over a 4/4 beat, rather than literal piano performances.

## Start here

- **PRESET:** tune. **SELECT:** mood. **SEL:** browse collections and tunes.
- **HOME:** main controls — **Tempo, Energy, Evolve, Mix**.
- **Home Mix:** 0 for Solo (lead and drums), 45 for Duet (bass and atmosphere),
  85 for Trio (a third melodic part). The balance blends continuously.
- **ALGORITHM:** Home → Lead → Bass → Drums → Layers → Colour → Delay → Space.
- **EDIT:** new track on Home; reset just the current editing page elsewhere.
- **REC:** hold/release an eight-bar passage. **PLAY:** pause/resume.
- **SAVE:** keep one favourite, including all edits. **GLO:** guide; turn SELECT
  to its final page, then press SEL to recall the saved track.
- **MASTER:** volume. **HOME or GLO:** close the guide.

Colour offers Clean, Vinyl, Tape and Grit, with independent amount, wobble and
dust. Vinyl is inspired by sampler vinyl-simulation effects; it is not an exact
SP-303 emulation. Delay follows the tempo. Its **Wet** control only changes the
echo balance; **Home Mix** controls Solo/Duet/Trio.

[Full controls and build notes](satie/README.txt) · [Score credits](satie/SOURCES.txt)
· [Artwork credits](satie/art/CREDITS.txt)

## Try and install

The release bundle contains a self-contained **SATIE-1.html** listening preview,
a `.fwsc` firmware package, a quick guide, artwork and corresponding source.
The preview and device have separate favourites. If your browser restricts
local audio worklets, serve the preview using a local HTTP server.

For the existing USB installer, install Python dependencies `mido` and
`python-rtmidi`, then run from the source folder:

```sh
python3 tools/fm1_install.py satie-1-0.1.3-beta.fwsc
```

Or select the package in a compatible FM-1 web installer. Keep USB connected
until the update completes. Back up material you want to keep before changing
firmware: SATIE-1 does not import other firmwares' projects or samples, and
switching firmware may overwrite shared flash storage. It migrates older
SATIE-1 favourites and the earlier development build's Satie choices.

**Status:** 0.1.3-beta, based on the monitored 0.1.3-trace2 build.
It retains the crash diagnostics and a defensive audio-worker completion fix.
Sounds, controls, saved-setting format and update-loader logic match trace2.

Earlier builds developed clicking followed by an unexpected reboot, particularly
in Ghost at high Mix and during track changes. **The underlying cause has not
been confirmed.** On 9 November, trace2 completed about 4 hours 32 minutes of
monitored playback with both audio cores online, no recorded crashes or audio
underruns, and no new timing overruns. A laptop-sleep gap was excluded; the final
power cycle was intentional. The short high-Mix/track-change test also passed.

The 0.1.3-beta firmware package is byte-for-byte identical to the tested trace2
package, so those listening results apply to it. Final checks passed on this
package: all five moods, Ghost at high Mix with rapid track changes, saved-favourite
recall after a cold start, stock V15 and PERF-1 round-trips, and interrupted-update
recovery followed by clean startup and track changes. The PERF-1 version was not
recorded. The Hub submission is for review; this is not a Hub approval.
See the [hardware test status](satie/SHARING.txt) for the full record.

The full device/package identity is `FM-1_9980100`; no listed package reports
that exact identity among 172 entries checked on 2026-11-10. Some older catalogue
patterns match wide ranges (including the short `FM-1_998` reply), so the Hub
submission must specify the full identity. This check is not a reservation.

## Build

Use the JieLi toolchain and AC79 SDK described in [BUILDING.md](BUILDING.md):

```sh
EN_SATIE1=1 sh build.sh --release 0.1.3-beta
sh satie/build_host.sh
sh satie/build_preview.sh
```

The package is `build/satie-1-0.1.3-beta.fwsc`. The host uses a native C compiler;
the preview uses the WASI SDK. Source score and artwork data are checked in.
To regenerate them, run `satie/generate.py` (mido) and `satie/gen_art.py` (Pillow).
The startup portrait is stored in flash and reuses the screen canvas.

Tests: `satie/check_pages.py`, `satie/guide-test.amb`, `satie/check_favourites.py`,
`satie/check_satie.py`, `satie/check_audio_pages.py`, `satie/check_web.mjs`,
`satie/pad_continuity_test.c`, `tests/install_test.py`.
The beta includes newer UC25HQ80 flash recognition; this chip has not yet been
physically tested by this project. The update loader already uses common SPI
commands and has no single-chip ID restriction.

## Sharing and credits

Firmware code is **GPL-3.0-only**, based on PERF-1 (deamonSLINKY), HYP-1
(RedtailWorks), AMB-1 (Charles Vestal) and Felucca (Leo Kuroshita / Hügelton
Instruments), with the additional contributors preserved in [LICENSING.md](LICENSING.md).
The complete corresponding source accompanies the package; public releases
must also have browsable source at their GitHub tag for the Firmware Hub.

Satie's compositions are public domain. The score editions used here have their
own notices: Gymnopédies and Ogives editions are public domain; Gnossienne
editions and adapted score data are **CC BY-SA 4.0**. Credits, source links and
reduction details are included. No audio recordings are embedded.

Cover and startup art are AI-assisted adaptations of Henri Manuel's portrait
of Erik Satie, circa 1920; see the artwork credits. Independent community
firmware, not affiliated with M-VAVE. Distributed without warranty.

[Portal preparation and remaining steps](satie/SHARING.txt) ·
[Original PERF-1 README](satie/UPSTREAM-PERF-1.md)

You can also try SATIE-1 in the browser, no flashing needed, in the free community [FM-1 Emulator](https://fm1-emulator.atst.tools/).
