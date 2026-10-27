"""Mechanical, reproducible port of bundled rev-225 Pix3D.flatTriangle.

Preserve every branch and edge update; retain Java argument/local names for
source review. Only the three slopes and integer operations are rewritten to
defined C99 Java-int helpers. The hand-ported flatRaster prefix is kept in a
separate source template; it preserves Java's increment-before-RHS alpha read.
Run from any directory. --check verifies the generated checked-in C source.
"""
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[4]
SOURCE = ROOT / 'javaclient/src/main/java/jagex2/graphics/Pix3D.java'
TARGET = ROOT / 'client-c/src/gfx/pix3d_flat225.c'
PREFIX = Path(__file__).with_name('pix3d_flat225_prefix.c.txt')


def generate():
    java = SOURCE.read_text(encoding='utf-8')
    body = java.split('public static final void flatTriangle(', 1)[1]
    body = body.split('\n\t@ObfuscatedName', 1)[0]
    body = body[body.index('{') + 1:].rstrip()
    assert body.endswith('}')
    body = body[:-1]
    body = body.replace('Pix2D.boundBottom', 'pix2d_clip_bottom')
    body = body.replace('Pix2D.width2d', 'pix2d_width')
    body = body.replace('lineOffset', 'pix3d_scanline')
    body = body.replace('int var', 'int32_t var')
    # Java shifts after subtraction; wrap subtraction and shift before division.
    body, slopes = re.subn(r'\((arg\d+) - (arg\d+) << 16\) / \((arg\d+) - (arg\d+)\)',
        r'jdiv(jshl16(jsub(\1, \2)), jsub(\3, \4))', body)
    assert slopes == 3
    body = re.sub(r'(arg\d+) \* (arg\d+)', r'jmul(\1, \2)', body)
    body = re.sub(r'(var\d+) \* (arg\d+)', r'jmul(\1, \2)', body)
    body = re.sub(r'(arg\d+) <<= 0x10', r'\1 = jshl16(\1)', body)
    body = re.sub(r'(arg\d+) ([+-])= ([^;]+);',
        lambda m: f'{m[1]} = j{"add" if m[2] == "+" else "sub"}({m[1]}, {m[3]});', body)
    body = re.sub(r'(arg\d+) >> 16', r'jshr16(\1)', body)
    body = re.sub(r'flatRaster\(Pix2D.data, (arg\d+), arg6, 0, (.*)\);',
        r'pix3d225_flat_raster(pix2d_pixels, \1, (uint32_t)arg6, \2);', body)
    # Explicit grouping matches Java precedence and avoids C -Wparentheses.
    body = re.sub(r'if \(([^\n]+?) && ([^\n]+?) \|\| ([^\n]+?) && ([^\n]+?)\) \{',
        r'if ((\1 && \2) || (\3 && \4)) {', body)
    assert '<<' not in body and '>>' not in body and 'Pix2D.' not in body
    assert body.count('pix3d225_flat_raster(') == 24
    body = '\n'.join(line[1:].replace('\t', '    ') if line.startswith('\t') else line
                     for line in body.splitlines()).strip('\n')
    signature = '''void pix3d225_flat_triangle(int32_t xA, int32_t xB, int32_t xC,
                           int32_t yA, int32_t yB, int32_t yC, uint32_t rgb) {
    int32_t arg0=yA, arg1=yB, arg2=yC, arg3=xA, arg4=xB, arg5=xC;
    int32_t arg6=jint(rgb);
'''
    return PREFIX.read_text(encoding='utf-8') + signature + body.rstrip() + '\n}\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = generate()
    if args.check:
        if TARGET.read_text(encoding='utf-8') != result:
            raise SystemExit('generated Pix3D flat source differs')
        print('PASS reproducible Pix3D flat source')
    else:
        TARGET.write_text(result, encoding='utf-8', newline='\n')
