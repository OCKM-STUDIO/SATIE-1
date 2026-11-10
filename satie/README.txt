SATIE-1 — Erik. Evolved. — 0.1.3-beta
===============================
Satie's piano scores, reimagined as an evolving electronic working soundtrack.
Default: Gymnopedie 1 / Orbit / 60 BPM / Energy 50 / Evolve 35 / Mix 42 (Duet).

LIBRARY
10 complete score reductions: Gymnopedies 1–3, Gnossiennes 1–3, Ogives 1–4.
The former composer's repertoire and its special cycle player have been removed
from this build. PRESET IDs 0–2 are Gymnopedies; 3–5 Gnossiennes; 6–9 Ogives.
SEL opens collection → tune. SELECT or PRESET moves the highlight, SEL enters
or confirms, HOME returns one level. Browsing does not change the playing tune.
Confirmation schedules the selected tune for the next bar while playing.

Original quarter-note timing is rounded to sixteenths over a 4/4 electronic beat.
Polyphony, ornaments and note lengths are simplified. This is an electronic
arrangement, not a faithful piano performance. With Evolve at zero the lead
follows the reduced source pitches. Otherwise treatments change every eight
bars without resetting the score or cutting off effect tails. Hold repeats
the current eight-bar passage; release continues through the score.
The score data, source hashes, edition credits and licences are documented in
SOURCES.txt, source-notes.json and the original files in sources/.

FIVE MOODS
After Hours: industrial synth lead, driven texture and a steady rhythm.
Ibiza Chill: a synthesized piano/string-like lead and a relaxed backing.
Orbit: rounded floating lead, warm bass, gentle percussion and spacious echoes.
Blue Note: synthesized piano, plucked upright bass and quiet swung brushes.
Ghost: dark rasping lead, a broken dusty beat and dedicated dub sub.
Colour now supplies optional vinyl/tape texture independently of mood.
The four Ogives use the same continuous player, at 56–60 BPM by default.

Mix controls the arrangement and balance: 0–10 lead and drums; 10–40 introduces
bass and atmosphere; 40–55 Duet; 55–80 adds the third musical part; 80–100 Trio
opens into more space and less rhythm. Blue Note's third voice is a quiet piano
answer; other moods use inner score tones or a source-derived melodic answer.
Mix glides between settings. Tune/mood changes and New track retain audio tails.

CONTROLS
SELECT       Mood (or highlight in the tune browser)
PRESET       Tune (or highlight in the tune browser)
SEL          Open tune browser / enter / choose
HOME         Main controls (or back one level in the tune browser)
ALGORITHM    Select one of the eight pages below; ends clamp rather than wrap
K1–K4        The four controls shown on the current page
EDIT         New track on Home; reset only the current page elsewhere
FX           Shortcut cycling Colour → Delay → Space
REC          Hold / release the current eight-bar passage
PLAY         Pause / play
SAVE         Keep favourite, including every instrument and FX setting
GLO          Built-in guide; final page + SEL recalls the favourite
MASTER       Output volume

PAGE         K1             K2             K3             K4
Home         Tempo          Energy         Evolve         Mix
Lead         Tone           Character      Space          Level
Bass         Tone           Attack         Length         Level
Drums        Tone           Swing          Pattern        Level
Layers       Third voice    Atmosphere     Space          Level
Colour       Style          Amount         Wobble         Dust
Delay        Time           Repeats        Tone           Wet
Space        Reverb         Size           Tone           Width

Page selection does not restart playback or change the seed. Continuous edits
are smoothed. Instrument controls work relative to the mood; 50 is neutral,
level 0 mutes. Mix still controls the overall Solo/Duet/Trio arrangement.
Lead Character shapes the mood's own sound: industrial edge in After Hours,
piano/string blend in Ibiza, detuned body in Orbit and darker rasp in Ghost.
Bass Attack softens the onset; Length adjusts the finite pluck/sub decay.
Swing delays off-sixteenth drum hits; patterns without these hits will not
change until you select a busier pattern. Pattern: Sparse/Steady/Mood/Broken/Busy.
Layers balances the inner melody and background atmosphere; its Level trims both.

Colour starts Clean. Vinyl combines compression, softened top end and tape
colour, inspired by SP-303 Vinyl Sim, not an exact hardware emulation. Tape and
Grit supply other saturation colours. Wobble adds pitch movement; Dust adds
crackle and hiss independently, and defaults to zero. Clean bypasses them all.
Delay Time: eighth, dotted eighth or quarter note, synced to 50–150 BPM.
Repeats controls feedback independently of room size, capped below unity.
Tone darkens the feedback; the original short secondary echo remains underneath.
Delay Wet and Reverb: 0 dry, 50 the mood/Mix balance, 100 full send.
Space Width adjusts the stereo effects: 0 mono wet signal, 50 normal, 100 wide.
EDIT resets just the displayed page, so experimenting does not lose other edits.


FAVOURITES
Uses the existing flash slot with a version-4 record (44 bytes). Older 16-byte favourites gain default page settings. Previous Satie choices
33–38 migrate to 0–5, retaining mood, seed, tempo and controls. A retired tune
becomes Gymnopedie 1 at 60 BPM, retaining its other valid settings. Invalid
records return to the default. Migration is written only on an explicit SAVE.
The preview reads the previous browser favourite if no SATIE-1 favourite exists,
then writes future saves under its own key. Device and browser saves are separate.

LISTEN
Open build/satie/SATIE-1.html and press Start listening. The self-contained file
includes the sound engine. If local-file worklets are restricted, serve it:
    python3 -m http.server 8768 --bind 127.0.0.1 --directory build/satie
    http://127.0.0.1:8768/SATIE-1.html
If the browser audio clock stalls, the page leaves controls disabled and offers
Try again rather than presenting an unresponsive instrument as ready.

BUILD AND CHECK
    .venv/bin/python satie/generate.py
    sh satie/build_host.sh
    mkdir -p build/satie/browser
    build/host/satie_host satie/browser-test.amb build/satie/browser
    .venv/bin/python satie/check_favourites.py
    .venv/bin/python satie/check_pages.py
    .venv/bin/python satie/check_audio_pages.py
    .venv/bin/python satie/check_satie.py
    sh satie/build_preview.sh
    node satie/check_web.mjs
The source tools need mido; the host screenshot tools need Pillow.
WASI SDK 27 is used from build/deps; WASM_CC may override its path.
Device build with the existing toolchain and SDK:
    EN_SATIE1=1 PYTHON="$PWD/.venv/bin/python" sh build.sh
Output: build/satie-1-dev.fwsc. Private development identity FM-1_9980100.
The identity is retained for the same FM-1 development update path.
Install from the repository directory using:
    .venv/bin/python tools/fm1_install.py build/satie-1-dev.fwsc
No automatic hardware update is performed. See SHARING.txt for hardware test results.

CREDITS
Score editions and derivative note data: public domain / CC-BY-SA-4.0.
Program: GPL-3.0-only, based on PERF-1 / HYP-1 / AMB-1. See ../LICENSING.md.

EARLIER VALIDATION — 0.1.0-beta, 2026-11-08
All 10 full reduced lead lines match the audio-clock MIDI output. Slow full
arrangements have bounded output and quiet pause tails. Collection navigation,
cancellation, next-bar selection, all five moods, Mix, Hold and New track pass.
All 28 page controls pass editing, bounds, page reset, Keep/reboot/Recall and
navigation tests. Seventeen favourite migration/malformed-record cases pass.
Audio comparisons confirm instrument and FX changes reach the output. All-five-
mood maximum-control renders remain bounded (peak 0.312 at test master gain).
Upright and dub-sub decay/retrigger/release checks pass.
The real worklet/WASM passes audio, display, picker and all-eight-page tests.
In-app preview: Lead Tone, page reset, Vinyl, Delay Time/Repeats and Home verified;
left paused at defaults. Screenshot: build/satie/pages-preview.png.
Device build: image 420612 bytes, RAM 92284/98304 bytes, pool 331176/344064 bytes.
No soft-double routines; update-entry and loader tests pass, including upgrade
from build/satie-1-before-pages.fwsc. No hardware flashing or listening performed.

GUIDE AND STARTUP UPDATE
GLO opens eight short instruction pages; SELECT/ALGO turns, SEL advances.
The final page recalls Keep with SEL. HOME/GLO closes. Reading preserves sound
settings and playback. Erik portrait at startup; a button skips it.
Ibiza Chill: inherited pad chorus and house pumping removed, echo modulation
steadied. Those changes alone did not resolve the reported clicking.
See QUICK-START.txt and SHARING.txt for the shareable beta and portal status.

0.1.1-beta — PAD CLICKING FIX, 2026-11-08
Pad target amplitude, FM index and attack smoothing now persist between audio
chunks until the next 32-sample control tick. Resetting them at each chunk
created discontinuities, most audible in Ibiza Chill as Mix brought up the pad.
The maker installed the listening candidate and confirmed the clicking is fixed.
Final review also corrected pad-state clearing on reset. Address/undefined-
behaviour sanitizer checks cover reset and all thirty partition cases. The
final rebuilt package still needs its on-device installation-route checks.
The Python installer also requests SATIE-1's complete FM-1_9980100 identity,
avoiding a false mismatch after a successful installation.
Thirty held-pad partition tests (five sounds, six block sizes) produce identical
audio after the fix. Score, page/control, audio limits, browser, installer,
update-entry and loader regression checks pass. Device image: 421060 bytes;
RAM 92332/98304 bytes; pool 331176/344064 bytes.
See SHARING.txt for installation routes completed and still outstanding.

0.1.2-beta — MOOD MESSAGE AND HARDWARE TEST RECORD, 2026-11-08
Mood-only changes say NEW MOOD ON NEXT BAR, including the wait-before-save
message. Tune/seed changes retain NEW TRACK, including a queued tune change
followed by a mood change. Audio and update-loader code are unchanged.
The maker confirmed the wording and PERF-1 round-trip with 0.1.2-dev.
See SHARING.txt for the completed tests and exact-release checks still pending.

0.1.3-beta — MONITORED CANDIDATE, 2026-11-10
Based on the 0.1.3-trace2 diagnostic build. Both audio-worker stage waits now
accept a completed job even if its separate stage flag was missed. This fixes
a reproduced host-side wait failure, but is NOT a confirmed explanation of the
hardware clicking/reboot issue. Retained boot/exception and individual-drum
traces stay enabled. The read-only USB monitor is tools/satie_bootlog.py.
It reports fallback/restarts and streams evidence to JSONL as it arrives.

On 9 November, trace2 completed about 4h32 of observed playback with both cores
online, no recorded crashes/underruns and no new timing overruns. A laptop-sleep
gap is excluded, and the final power cycle was confirmed intentional. The cause
of the earlier intermittent failures remains unconfirmed. Exact 0.1.3-beta
hardware checks are still pending; see SHARING.txt and FINAL-CHECKS.txt.
