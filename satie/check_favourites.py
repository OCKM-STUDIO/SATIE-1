#!/usr/bin/env python3
"""Keep existing Satie favourites and safely retire removed tune identifiers."""
import struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
out=root/'build/satie/favourites';out.mkdir(parents=True,exist_ok=True)
# version, magic, old source, expected source, expected mood, expected tempo
cases=[(2,0x31484342,33+i,i,4,58) for i in range(6)]
cases += [(2,0x31484342,32,0,4,60),(1,0x31484342,2,0,1,60),
          (3,0x31544153,9,9,4,58),(3,0x31544153,10,0,2,60),
          (2,0x31484342,39,0,2,60),(3,0x12345678,1,0,2,60)]
defaults=bytes([50,50,50,50,50,50,50,50,50,50,2,50,50,50,50,50,0,35,15,0,1,55,50,50,50,50,50,50])
# Valid v4 and malformed/truncated page settings must never escape validation.
extras=[defaults,defaults[:10]+bytes([5])+defaults[11:],defaults[:16]+bytes([4])+defaults[17:],defaults[:20]+bytes([3])+defaults[21:],defaults[:-1]]
cases += [(4,0x31544153,9,9 if i==0 else 0,4 if i==0 else 2,58 if i==0 else 60) for i in range(len(extras))]
for case,(version,magic,old,new,mood,tempo) in enumerate(cases):
    folder=out/str(case);folder.mkdir(exist_ok=True)
    data=struct.pack('<IHH8B',magic,version,557,1 if version==1 else 4,old,58,35,20,26,1,0)
    if version==4:data+=extras[case-12]
    blob=folder/'old.bin';blob.write_bytes(data)
    script=folder/'check.amb'
    script.write_text(f'''loadobj 25 {blob.relative_to(root)}
reboot
wait 200
expect satie_source {new}
expect satie_mood {mood}
expect tempo {tempo}
tap SAVE
wait 1000
reboot
wait 200
expect satie_source {new}
expect satie_mood {mood}
expect tempo {tempo}
spin ALGO 5
expect satie_k1 0
expect satie_k4 0
spin ALGO 1
expect satie_k1 1
expect satie_k2 55
expect parsbad 0
''')
    r=subprocess.run([str(root/'build/host/satie_host'),str(script),str(folder)],capture_output=True,text=True,cwd=root)
    (folder/'check.log').write_text(r.stdout+r.stderr)
    assert r.returncode==0,(case,r.stdout,r.stderr)
print(f'{len(cases)} favourite migration/validation/save/reboot cases passed.')
