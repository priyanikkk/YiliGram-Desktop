import colorsys, re, sys
from materialyoucolor.hct import Hct
from materialyoucolor.palettes.tonal_palette import TonalPalette
from materialyoucolor.utils.color_utils import lstar_from_argb, argb_from_rgb
seed = Hct.from_int(0xFF8C7A5B)
P = TonalPalette.from_hue_and_chroma(seed.hue, 36)
S = TonalPalette.from_hue_and_chroma(seed.hue, 16)
N = TonalPalette.from_hue_and_chroma(seed.hue, 6)
N2 = TonalPalette.from_hue_and_chroma(seed.hue, 10)
def conv(hx, key):
    rgb = tuple(int(hx[i:i+2], 16) for i in (0, 2, 4))
    h, l, s = colorsys.rgb_to_hls(*[c / 255 for c in rgb])
    h *= 360
    if not (185 <= h <= 232) or s < 0.08 or max(rgb) - min(rgb) < 8:
        return None
    t = lstar_from_argb(argb_from_rgb(*rgb))
    accent = any(w in key for w in ('Out', 'Active', 'Selected'))
    if s <= 0.35:
        pal = N
    elif t < 35 and 'Bg' in key:
        pal = S if accent else N2
    else:
        pal = S if 'Out' in key and 'Fg' not in key else P
    return '%06x' % (pal.tone(t) & 0xffffff)
src = open(sys.argv[1], encoding='utf-8').read().split('\n')
out = []
n = 0
for line in src:
    m = re.match(r'^(\w+)(\s*:\s*#)([0-9a-fA-F]{6})([0-9a-fA-F]{2})?(;.*)$', line)
    if m:
        c = conv(m.group(3).lower(), m.group(1))
        if c:
            line = m.group(1) + m.group(2) + c + (m.group(4) or '') + m.group(5)
            n += 1
    out.append(line)
open(sys.argv[2], 'w', encoding='utf-8').write('\n'.join(out))
print(n, 'changed', file=sys.stderr)
