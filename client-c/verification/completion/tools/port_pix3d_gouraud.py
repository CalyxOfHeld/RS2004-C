"""Reproduce the C99 port of bundled revision-225 Pix3D.gouraudTriangle.

Retain Java branches, edge updates and all raster calls. Replace integer
arithmetic with defined Java-int helpers. Raster prefix is a reviewed compact
port of gouraudRaster. --check verifies the generated checked-in source.
"""
import argparse
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[4]
SOURCE=ROOT/'javaclient/src/main/java/jagex2/graphics/Pix3D.java'
TARGET=ROOT/'client-c/src/gfx/pix3d_gouraud225.c'
PREFIX=Path(__file__).with_name('pix3d_gouraud225_prefix.c.txt')


def generate():
    body=SOURCE.read_text(encoding='utf-8').split('public static final void gouraudTriangle(',1)[1]
    body=body.split('\n\t@ObfuscatedName',1)[0]
    body=body[body.index('{')+1:].rstrip()
    assert body.endswith('}')
    body=body[:-1].replace('Pix2D.boundBottom','pix2d_clip_bottom')
    body=body.replace('Pix2D.width2d','pix2d_width').replace('lineOffset','pix3d_scanline')
    body=body.replace('int var','int32_t var')
    body,slopes=re.subn(r'\((arg\d+) - (arg\d+) << (15|16)\) / \((arg\d+) - (arg\d+)\)',
        r'jdiv(jshl(jsub(\1, \2), \3), jsub(\4, \5))',body)
    assert slopes==6
    body=re.sub(r'((?:var|arg)\d+) \* (arg\d+)',r'jmul(\1, \2)',body)
    body=re.sub(r'(arg\d+) <<= (0x[0-9A-F]+)',
        lambda m:f'{m[1]} = jshl({m[1]}, {int(m[2],16)})',body)
    body=re.sub(r'(arg\d+) ([+-])= ([^;]+);',
        lambda m:f'{m[1]} = j{"add" if m[2]=="+" else "sub"}({m[1]}, {m[3]});',body)
    body=re.sub(r'(arg\d+)--;',r'\1 = jsub(\1, 1);',body)
    body=re.sub(r'(arg\d+) >> (7|16)',r'jshr(\1, \2)',body)
    body=re.sub(r'gouraudRaster\(Pix2D.data, (arg\d+), 0, 0, (.*)\);',
        r'pix3d225_gouraud_raster(pix2d_pixels, \1, \2);',body)
    body=re.sub(r'if \(([^\n]+?) && ([^\n]+?) \|\| ([^\n]+?) && ([^\n]+?)\) \{',
        r'if ((\1 && \2) || (\3 && \4)) {',body)
    assert '<<' not in body and '>>' not in body and 'Pix2D.' not in body
    assert body.count('pix3d225_gouraud_raster(')==24
    body='\n'.join(line[1:].replace('\t','    ') if line.startswith('\t') else line
        for line in body.splitlines()).strip('\n')
    signature='''void pix3d225_gouraud_triangle(int32_t xA,int32_t xB,int32_t xC,
                              int32_t yA,int32_t yB,int32_t yC,
                              int32_t colourA,int32_t colourB,int32_t colourC) {
    int32_t arg0=yA,arg1=yB,arg2=yC,arg3=xA,arg4=xB,arg5=xC;
    int32_t arg6=colourA,arg7=colourB,arg8=colourC;
'''
    return PREFIX.read_text(encoding='utf-8')+signature+body.rstrip()+'\n}\n'


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    result=generate()
    if args.check:
        if TARGET.read_text(encoding='utf-8')!=result:
            raise SystemExit('generated Pix3D Gouraud source differs')
        print('PASS reproducible Pix3D Gouraud source')
    else:
        TARGET.write_text(result,encoding='utf-8',newline='\n')
