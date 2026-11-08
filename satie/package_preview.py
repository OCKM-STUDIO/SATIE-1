#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import re
import base64
import json
import html as html_module
from pathlib import Path
root = Path(__file__).resolve().parents[1]
html = (root / 'satie/preview.html').read_text()
html = html.replace('__SATIE_WASM__', base64.b64encode((root / 'build/satie/satie.wasm').read_bytes()).decode())
html = html.replace('__SATIE_WORKLET__', json.dumps((root / 'web/emu/worklet.js').read_text()).replace('</', '<\\/'))
catalog = json.loads((root / 'satie/catalog.json').read_text())
options = []
for group in dict.fromkeys(t['group'] for t in catalog):
    options.append('<optgroup label="'+html_module.escape(next(t['composer'] for t in catalog if t['group']==group)+' · '+group)+'">')
    for tune in catalog:
        if tune['group'] == group:
            options.append(f'<option value="{tune["id"]}">'+html_module.escape(tune['title']+(' · slow' if tune['slow'] else ''))+'</option>')
    options.append('</optgroup>')
html = html.replace('__SATIE_OPTIONS__', ''.join(options)).replace('__SATIE_CATALOG__',json.dumps(catalog))
guide=(root/'firmware/src/app/satie_ui.inc').read_text().split('SATIE_HELP[SATIE_HELP_PAGES] = {',1)[1].split('\n};',1)[0]
rows=re.findall(r'\{"([^"]+)", \{([^}]+)\}\}',guide)
help_pages=[dict(title=title,lines=re.findall(r'"([^"]*)"',body)) for title,body in rows]
assert len(help_pages)==8 and all(len(p['lines'])==6 for p in help_pages)
html=html.replace('__SATIE_GUIDE__',json.dumps(help_pages))
path = root / 'build/satie/SATIE-1.html'
path.write_text(html)
print(f'Playable preview: {path} ({path.stat().st_size:,} bytes)')
