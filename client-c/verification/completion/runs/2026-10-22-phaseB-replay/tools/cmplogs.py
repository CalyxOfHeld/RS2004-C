import sys, difflib
from pathlib import Path
base = Path(__file__).parent
a_dir, b_dir = sys.argv[1], sys.argv[2]
for name in ('scene', 'rebuild', 'walk', 'ui'):
    for ext in ('log', 'err'):
        a = (base / a_dir / f'{name}.{ext}').read_text(errors='replace').replace('\\' + a_dir + '\\', '\\X\\').splitlines()
        b = (base / b_dir / f'{name}.{ext}').read_text(errors='replace').replace('\\' + b_dir + '\\', '\\X\\').splitlines()
        d = [l for l in difflib.unified_diff(a, b, lineterm='', n=0) if l[:1] in '+-' and l[:3] not in ('+++', '---')]
        print(f'{name}.{ext}: {len(a)} vs {len(b)} lines, {len(d)} differing')
        for l in d[:10]:
            print('   ', l)
