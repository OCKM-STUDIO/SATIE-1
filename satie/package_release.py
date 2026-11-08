#!/usr/bin/env python3
"""Make an explicit-source tester bundle, without checkout history or local state."""
from pathlib import Path
import hashlib,json,re,shutil,zipfile
from PIL import Image
root=Path(__file__).resolve().parents[1]
version=(root/'satie/VERSION').read_text().strip()
assert re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-z0-9]+)?',version)
name=f'SATIE-1-{version}'
dest=root/'build/releases'/name;dest.mkdir(parents=True,exist_ok=True)
firmware=root/f'build/satie-1-{version}.fwsc'
assert firmware.is_file()
meta=json.loads((root/'fmhub.json').read_text())
assert set(meta)<=set('what author kinds play editor cheat image color'.split())
assert 20<=len(meta['what'])<=300 and not any(c in meta['what'] for c in '<>{}`')
art=root/meta['image'];im=Image.open(art)
assert im.width==im.height and 240<=im.width<=1024 and art.stat().st_size<=300000
roots=['firmware','host','tools','web','assets','tests','satie','.github/workflows']
files={root/n for n in ['README.md','LICENSE','LICENSING.md','BUILDING.md','build.sh','fmhub.json','.gitignore']}
for folder in roots:
 for p in (root/folder).rglob('*'):
  if p.is_file() and not p.is_symlink() and not {'__pycache__','.DS_Store'}&set(p.parts) and p.suffix not in ('.pyc','.o') and p.name!='SATIE-1-cover.png':files.add(p)
source=dest/f'{name}-source.zip'
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(files):z.write(p,Path('SATIE-1-source')/p.relative_to(root))
 assert z.testzip() is None
copies={firmware:firmware.name,root/'build/satie/SATIE-1.html':'SATIE-1.html',root/'satie/QUICK-START.txt':'QUICK-START.txt',
 root/'satie/SHARING.txt':'SHARING.txt',root/'satie/SOURCES.txt':'SOURCES.txt',root/'LICENSE':'LICENSE',root/'LICENSING.md':'LICENSING.md',
 art:'SATIE-1-card.jpg',root/'satie/art/SATIE-1-Erik-Evolved.png':'SATIE-1-cover.png',root/'satie/art/CREDITS.txt':'ART-CREDITS.txt',
 root/'satie/art/PROMPTS.txt':'ART-PROMPTS.txt',root/'fmhub.json':'fmhub.json',root/'build/satie/guide/startup.png':'startup-screen.png'}
for src,target in copies.items():shutil.copyfile(src,dest/target)
(dest/'START-HERE.txt').write_text(f'''SATIE-1 — Erik. Evolved. — {version}

For listening: open SATIE-1.html and press Start listening. If your browser
restricts local audio worklets, serve this folder over local HTTP.
For your FM-1: use satie-1-{version}.fwsc with a compatible FM-1 installer.
The Python USB installer is inside the source ZIP: tools/fm1_install.py.
Do not unplug while it writes. Back up projects before changing firmware.

GLO opens the guide. Read QUICK-START.txt for the controls and saved-track recall.
The pad-clicking fix preserves tone and envelope values across audio blocks.
The maker confirmed the Ibiza Chill clicking is fixed on a real FM-1.

This candidate passed native/UI/audio, preview and package-update checks.
It has NOT yet completed the Hub's required hardware installation routes.
See SHARING.txt for the remaining hardware checks before a Hub submission.
Source and beta releases: https://github.com/OCKM-STUDIO/SATIE-1

The source ZIP contains all project sources, build scripts, score editions,
art and licences; external toolchain/SDK setup is documented in BUILDING.md.
Browsable source is provided at the matching release tag in that repository.
The cover/card and fmhub.json identify OCKM Studio as the author.
''')
manifest=''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in sorted(dest.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt')
(dest/'SHA256SUMS.txt').write_text(manifest)
bundle=dest.with_suffix('.zip') if '.' not in dest.name else dest.parent/(name+'.zip')
with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(dest.iterdir()):
  if p.is_file():z.write(p,Path(name)/p.name)
 assert z.testzip() is None
print(f'Bundle: {bundle} ({bundle.stat().st_size:,} bytes)')
print(f'Source: {len(files)} explicitly selected files; card {im.width}x{im.height}, {art.stat().st_size:,} bytes; description {len(meta["what"])} characters.')
