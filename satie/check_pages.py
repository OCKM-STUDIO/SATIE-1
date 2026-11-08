#!/usr/bin/env python3
"""Exercise the hardware UI path: page edits, limits, persistence and playback."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'build/satie/pages';out.mkdir(parents=True,exist_ok=True)
defaults=[[50,50,50,50],[50,50,50,50],[50,50,2,50],[50,50,50,50],[0,35,15,0],[1,55,50,50],[50,50,50,50]]
values=[[22,71,30,62],[35,18,80,58],[25,74,3,65],[66,30,70,52],[1,45,25,12],[2,67,28,70],[42,68,35,72]]
lines=['wait 3000','expect playing 1','expect satie_step >10','expect satie_seed 1']
for page,(base,vals) in enumerate(zip(defaults,values),1):
 lines += ['spin ALGO 1',f'expect satie_page {page}']
 for k,(before,value) in enumerate(zip(base,vals),1):
  lines += [f'spin K{k} {value-before}',f'expect satie_k{k} {value}']
 lines += ['wait 1000','expect playing 1','expect satie_seed 1','expect satie_pending 0','expect parsbad 0',f'shot page-{page}.png']
lines += ['expect satie_step >30','tap SAVE','wait 1500','reboot','wait 500','expect satie_page 0']
for page,vals in enumerate(values,1):
 lines += ['spin ALGO 1']+[f'expect satie_k{k} {v}' for k,v in enumerate(vals,1)]
lines += ['tap HOME','tap FX','expect satie_page 5','tap FX','expect satie_page 6','tap FX','expect satie_page 7','tap FX','expect satie_page 5','tap EDIT','expect satie_seed 1']
lines += [f'expect satie_k{k} {v}' for k,v in enumerate(defaults[4],1)]
lines += ['tap GLO','spin SELECT 200','tap SEL','waitfor satie_pending 0 5000','expect satie_k1 1','expect satie_k2 45','tap SEL','expect satie_browser 1','tap HOME','expect satie_page 5','tap HOME','expect satie_page 0']
for p in range(1,8):
 lines += ['spin ALGO 1']
 for k in range(1,5):
  maximum=4 if p==3 and k==3 else 3 if p==5 and k==1 else 2 if p==6 and k==1 else 100
  lines += [f'spin K{k} 200',f'expect satie_k{k} {maximum}',f'spin K{k} -200',f'expect satie_k{k} 0']
 lines += ['tap EDIT']+[f'expect satie_k{k} {v}' for k,v in enumerate(defaults[p-1],1)]
lines += ['spin ALGO 200','expect satie_page 7','spin ALGO -200','expect satie_page 0','expect satie_seed 1','expect parsbad 0']
script=out/'check.amb';script.write_text('\n'.join(lines)+'\n')
r=subprocess.run([str(root/'build/host/satie_host'),str(script),str(out)],cwd=root,capture_output=True,text=True)
(out/'check.log').write_text(r.stdout+r.stderr)
assert r.returncode==0,r.stdout+r.stderr
print('All 28 page controls: editing, limits, reset, Keep/reboot/Recall, FX shortcut and uninterrupted page navigation passed.')
