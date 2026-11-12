# SATIE-1 browser player

Published with GitHub Pages from main:/docs at https://ockm-studio.github.io/SATIE-1/.

The self-contained index.html uses the tested v0.1.3-beta audio engine and worklet.
Its dedicated SATIE interface includes Erik’s portrait, rotary controls, ten
repertoire keys and responsive desktop/mobile layouts.

Edit satie/preview.html, satie/player.css and satie/player.js. For interface-only
updates, run `python3 satie/package_preview.py` with the existing build/satie/satie.wasm.
For an audio-engine release, first run `sh satie/build_preview.sh`.
Validate with `node satie/check_web.mjs` and browser checks, then copy
build/satie/SATIE-1.html here as index.html. Keep the source/download links on
the matching firmware release. The firmware release process does not automatically
replace the live player.
