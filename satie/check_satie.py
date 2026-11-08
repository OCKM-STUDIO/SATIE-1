#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify each complete Satie lead through the audio-clock MIDI output."""
import hashlib,json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((root/'satie/source-notes.json').read_text())
assert len(rows)==10
for source,row in enumerate(rows):
    assert hashlib.sha256((root/'satie/sources'/row['file']).read_bytes()).hexdigest()==row['sha256']
    out=root/f'build/satie/scores/{source}';out.mkdir(parents=True,exist_ok=True)
    script=out/'check.amb'
    script.write_text(f'''wait 100
tap PLAY
spin PRESET {source}
spin SELECT {source%5-2}
spin K3 -200
spin K4 -200
spin K1 200
wait 1000
expect satie_source {source}
expect satie_length {row['steps']}
expect satie_voices 0
expect tempo 150
tap PLAY
wait {row['steps']*100+500}
expect parsbad 0
tap PLAY
wait 100
expect held 0
spin K4 85
spin K1 -200
wait 1000
expect satie_voices 2
master 2800
tap PLAY
peakreset
wav satie.wav
wait 25000
wavstop
expect peak_db_min -40
expect peak_db_max -0.5
tap PLAY
wait 20000
peakreset
wait 4000
expect peak_db_max -48
''')
    result=subprocess.run([str(root/'build/host/satie_host'),str(script),str(out)],capture_output=True,text=True,cwd=root)
    (out/'check.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0,(source,result.stdout,result.stderr)
    notes=[]
    for line in (out/'midi_out.txt').read_text().splitlines():
        m=re.match(r'(\d+) ms: 91 ([0-9A-F]+) ([0-9A-F]+)',line)
        if m and int(m[3],16)>0:notes.append(int(m[2],16))
    expected=[n for n in row['melody'] if n]
    start=next((i for i in range(len(notes)) if notes[i:i+12]==expected[:12]),None)
    assert start is not None,(source,'opening missing')
    assert notes[start:start+len(expected)]==expected,(source,'full melody differs')
    print(f'{row["title"]}: all {len(expected)} source attacks verified; slow full arrangement and pause passed.',flush=True)
