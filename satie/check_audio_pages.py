#!/usr/bin/env python3
"""Render page controls through the real UI/engine; check audible effects and bounds."""
from pathlib import Path
import subprocess,wave,array,math
root=Path(__file__).resolve().parents[1];out=root/'build/satie/audio-pages';out.mkdir(parents=True,exist_ok=True)
def render(name,edits,mood=2):
 folder=out/name;folder.mkdir(exist_ok=True)
 # Identical seed, schedule and setup duration; only parameter choices differ.
 script=['wait 100','tap PLAY',f'spin SELECT {mood-2}','wait 100','spin K3 -200','spin K4 43']
 for p in range(1,8):
  script+=['spin ALGO 1']
  for k in range(1,5):script += [f'spin K{k} {edits.get((p,k),0)}']
 script+=['wait 2000','tap PLAY','wav sound.wav','wait 12000','wavstop','expect parsbad 0']
 f=folder/'check.amb';f.write_text('\n'.join(script)+'\n')
 r=subprocess.run([str(root/'build/host/satie_host'),str(f),str(folder)],cwd=root,capture_output=True,text=True)
 assert r.returncode==0,r.stdout+r.stderr
 with wave.open(str(folder/'sound.wav')) as w:a=array.array('h',w.readframes(w.getnframes()))
 peak=max(abs(x) for x in a)/32768
 assert 0.005<peak<.99,(name,peak)
 return a,peak
base,_=render('reference',{})
repeat,_=render('repeat',{})
assert base==repeat,'Fixture must be deterministic'
cases={'lead-tone':{(1,1):-50},'lead-character':{(1,2):50},'lead-space':{(1,3):-50},'lead-level':{(1,4):-50},
 'bass-tone':{(2,1):-50},'bass-attack':{(2,2):50},'bass-length':{(2,3):50},'bass-level':{(2,4):-50},
 'drum-tone':{(3,1):-50},'drum-swing':{(3,2):50},'drum-pattern':{(3,3):1},'drum-level':{(3,4):-50},
 'inner':{(4,1):-50},'atmosphere':{(4,2):-50},'layer-space':{(4,3):-50},'layer-level':{(4,4):-50},
 'vinyl':{(5,1):1},'tape':{(5,1):2},'grit':{(5,1):3},'dust':{(5,1):1,(5,4):75},
 'delay-time':{(6,1):1},'delay-repeats':{(6,2):45},'delay-tone':{(6,3):-50},'delay-mix':{(6,4):50},
 'reverb':{(7,1):50},'size':{(7,2):50},'room-tone':{(7,3):-50},'width':{(7,4):-50}}
for name,edits in cases.items():
 reference=base
 if name=='drum-swing':
  # Swing acts on off-sixteenths; Orbit's default sparse beat has none.
  reference,_=render('swing-reference',{(3,3):2})
  edits={**edits,(3,3):2}
 a,peak=render(name,edits)
 delta=math.sqrt(sum((x-y)**2 for x,y in zip(a,reference))/len(a))/32768
 assert delta>.00001,(name,delta)
 print(f'{name}: changed audio RMS {delta:.5f}, peak {peak:.3f}')
for mood in range(5):
 _,peak=render(f'extreme-{mood}',{(p,k):200 for p in range(1,8) for k in range(1,5)},mood)
 print(f'Mood {mood}, all controls maximum: peak {peak:.3f}')
print('Audible instrument/FX controls and extreme settings passed.')
