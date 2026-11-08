#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/satie
SATIE_VERSION="${SATIE_VERSION:-$(cat satie/VERSION)}"
WASM_CC="${WASM_CC:-$PWD/build/deps/wasi-sdk-27.0-arm64-macos/bin/clang}"
"$WASM_CC" --target=wasm32 -O2 -ffp-contract=off -std=gnu99 -nostdlib -ffreestanding -w \
    -DEN_HOST -DEN_WEB -DEN_SATIE1 "-DEN_VERSION=\"SATIE-1 $SATIE_VERSION\"" \
    -Iweb/emu/shim -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -Wl,--no-entry -Wl,--export-dynamic -Wl,--initial-memory=33554432 -Wl,-z,stack-size=1048576 \
    -o build/satie/satie.wasm web/emu/amb_web.c firmware/src/dsp/amb.c web/emu/shim/libc.c
python3 satie/package_preview.py
