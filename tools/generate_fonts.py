"""Regenerate Chinese subsets: python tools/generate_fonts.py FONT.ttf LV_FONT_CONV.js"""
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

root = Path(__file__).resolve().parents[1]
font = Path(sys.argv[1]).resolve()
converter = Path(sys.argv[2]).resolve()
source = '\n'.join(path.read_text(encoding='utf-8') for path in sorted((root / 'main').rglob('*.cpp')))
symbols = ''.join(sorted(set(re.findall(r'[^\x00-\x7f]', source))))
out = root / 'main/fonts'
temporary = tempfile.TemporaryDirectory()
render_font = Path(temporary.name) / 'NotoSansSC-Medium.ttf'
tt = TTFont(font)
missing = [char for char in symbols if ord(char) not in tt.getBestCmap()]
if missing:
    raise ValueError(f'Missing font glyphs: {missing}')
if 'fvar' in tt:
    instantiateVariableFont(tt, {'wght': 500}, inplace=True)
tt.save(render_font)
for size in (16, 20, 24, 28):
    subprocess.run(['node', str(converter), '--font', str(render_font), '--size', str(size),
                    '--bpp', '4', '--format', 'lvgl', '--range', '0x20-0x7e',
                    '--symbols', symbols, '--no-compress', '--no-kerning', '--lv-include', 'lvgl.h',
                    '--lv-font-name', f'lobby_zh_{size}',
                    '-o', str(out / f'lobby_zh_{size}.c')], check=True)
    path = out / f'lobby_zh_{size}.c'
    text = path.read_text(encoding='utf-8')
    # The generator's command comment includes a local path; keep generated files portable.
    text = re.sub(r' \* Opts:.*', ' * Source: Noto Sans SC (OFL); see tools/generate_fonts.py', text)
    path.write_text(text, encoding='utf-8')
(out / 'font-manifest.json').write_text(json.dumps({
    'family': 'Noto Sans SC', 'license': 'SIL OFL 1.1', 'converter': 'lv_font_conv 1.5.3',
    'font_sha256': hashlib.sha256(font.read_bytes()).hexdigest(),
    'weight': 500, 'sizes': [16, 20, 24, 28], 'bpp': 4, 'ascii': '0x20-0x7e', 'symbols': symbols
}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
temporary.cleanup()
