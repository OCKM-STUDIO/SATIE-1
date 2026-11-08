#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host build/gen
PY="${PYTHON:-python3}"
[ -f build/gen/felucca_font.h ] || "$PY" tools/gen_font.py build/gen/felucca_font.h
${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra \
    -Wno-unused-function -Wno-unused-parameter -DEN_HOST -DEN_SATIE1 \
    -D_POSIX_C_SOURCE=200809L -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o build/host/satie_host host/amb_host.c firmware/src/dsp/amb.c -lm
