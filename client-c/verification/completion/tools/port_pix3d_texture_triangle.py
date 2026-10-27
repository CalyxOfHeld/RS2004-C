"""Reproduce the C99 triangle-edge port from bundled Pix3D.textureTriangle."""
import argparse
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[4]
SOURCE=ROOT/'javaclient/src/main/java/jagex2/graphics/Pix3D.java'
TARGET=ROOT/'client-c/src/gfx/pix3d_texture_triangle225.c'
PREFIX=Path(__file__).with_name('pix3d_texture_triangle225_prefix.c.txt')

def generate():
    body=SOURCE.read_text(encoding='utf-8').split('public static final void textureTriangle(',1)[1]
    body=body.split('\n\t@ObfuscatedName',1)[0]
    body=body[body.index('{')+1:].rstrip()
    assert body.endswith('}')
    body=body[:-1]
    body=body.replace('int[] var19 = getTexels(arg18);','const uint32_t *var19 = texels;')
    body=body.replace('opaque = !textureTranslucent[arg18];','')
    body=body.replace('Pix2D.boundBottom','pix2d_clip_bottom').replace('Pix2D.width2d','pix2d_width')
    body=body.replace('lineOffset','pix3d_scanline').replace('centerH3D','pix3d_center_y')
    body=body.replace('int var','int32_t var')
    body,pre=re.subn(r'int32_t (var\d+) = (var\d+) \* (\w+) - (var\d+) \* (\w+) << (\d+);',
        r'int32_t \1 = jshl(jsub(jmul(\2, \3), jmul(\4, \5)), \6);',body)
    assert pre==9
    body,initial=re.subn(r'int32_t (var\d+) = (arg\d+) - (arg\d+);',
        r'int32_t \1 = jsub(\2, \3);',body)
    assert initial==6
    body,slopes=re.subn(r'\((arg\d+) - (arg\d+) << 16\) / \((arg\d+) - (arg\d+)\)',
        r'jdiv(jshl(jsub(\1, \2), 16), jsub(\3, \4))',body)
    assert slopes==6
    body,chain=re.subn(r'(arg\d+) = (arg\d+) <<= 0x10;',
        r'\2 = jshl(\2, 16); \1 = \2;',body)
    assert chain>0
    body=re.sub(r'(arg\d+) <<= 0x10;',r'\1 = jshl(\1, 16);',body)
    body=re.sub(r'(arg\d+) -= (var\d+) \* (arg\d+);',
        r'\1 = jsub(\1, jmul(\2, \3));',body)
    body=re.sub(r'(var\d+) \+= (var\d+) \* (var\d+);',
        r'\1 = jadd(\1, jmul(\2, \3));',body)
    body=re.sub(r'((?:arg|var)\d+) ([+-])= ((?:arg|var)\d+|pix2d_width);',
        lambda m:f'{m[1]} = j{"add" if m[2]=="+" else "sub"}({m[1]}, {m[3]});',body)
    body=re.sub(r'(arg\d+)--;',r'\1 = jsub(\1, 1);',body)
    body=re.sub(r'(var\d+) = (arg\d+) - pix3d_center_y;',
        r'\1 = jsub(\2, pix3d_center_y);',body)
    body=re.sub(r'(arg\d+) >> (8|16)',r'jshr(\1, \2)',body)
    body,dispatch=re.subn(r'textureRaster\(Pix2D.data, var19, 0, 0, (.*)\);',
        r'pix3d225_texture_raster(pix2d_pixels, var19, 0, 0, \1, opaque);',body)
    assert dispatch==24
    body=re.sub(r'if \(([^\n]+?) && ([^\n]+?) \|\| ([^\n]+?) && ([^\n]+?)\) \{',
        r'if ((\1 && \2) || (\3 && \4)) {',body)
    assert '<<' not in body and '>>' not in body and 'Pix2D.' not in body
    assert not re.search(r'\b(arg|var)\d+\s*[+*\-]=',body)
    body='\n'.join(line[1:].replace('\t','    ') if line.startswith('\t') else line
        for line in body.splitlines()).strip('\n')
    return PREFIX.read_text(encoding='utf-8')+body.rstrip()+'\n}\n'

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    output=generate()
    if args.check:
        if TARGET.read_text(encoding='utf-8')!=output:
            raise SystemExit('generated Pix3D texture triangle source differs')
        print('PASS reproducible Pix3D texture triangle source')
    else:
        TARGET.write_text(output,encoding='utf-8',newline='\n')
