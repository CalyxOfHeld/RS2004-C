from pathlib import Path
p=Path('include/core/packet.h');s=p.read_text().replace('    Isaac *random;','    Isaac *random;\n    bool error; /* Sticky bounds/format error; consumers must reject malformed data. */');p.write_text(s)
p=Path('src/core/packet.c');s=p.read_text().replace('    pkt.random = NULL;', '    pkt.random = NULL;\n    pkt.error = false;');idx=s.index('int32_t g1(Packet *pkt)');s=s[:idx]+'''static bool packet_require(Packet *pkt,size_t bytes) {
    if(!pkt || pkt->error) return false;
    if(pkt->pos>pkt->length || bytes>pkt->length-pkt->pos || (bytes && !pkt->data)) {
        pkt->error=true;return false;
    }
    return true;
}

'''+s[idx:]
for name,count in [('g1',1),('g1b',1),('g2',2),('g2b',2),('g3',3),('g4',4),('gsmart',1),('gsmarts',1)]:
    import re
    pat=r'((?:int32_t|int8_t|int16_t) '+name+r'\(Packet \*pkt\) \{)'
    s,n=re.subn(pat,r'\1\n    if(!packet_require(pkt,'+str(count)+')) return 0;',s);assert n==1,(name,n)
s=s.replace('''    dst[i] = '\\0';
}''','''    else pkt->error=true;
    if(max_len && dst) dst[i] = '\\0';
}''',1).replace('if (i + 1 < max_len) {','if (dst && i + 1 < max_len) {',1)
s=s.replace('void gdata(Packet *pkt, uint8_t *dst, size_t len) {','void gdata(Packet *pkt, uint8_t *dst, size_t len) {\n    if(!packet_require(pkt,len)) return;')
s=s.replace('''int32_t gbit(Packet *pkt, int32_t n) {
''','''int32_t gbit(Packet *pkt, int32_t n) {
    if(!pkt || pkt->error) return 0;
    if(n<0 || n>32 || pkt->bit_pos>pkt->length*8 || (size_t)n>pkt->length*8-pkt->bit_pos) {
        pkt->error=true;return 0;
    }
    if(!n) return 0;
''');p.write_text(s)
p=Path('tests/test_packet.c');s=p.read_text();idx=s.index('    printf("[Test Packet] ALL TESTS PASSED!');s=s[:idx]+'''    uint8_t truncated[]={0xff};
    Packet bad=packet_wrap(truncated,sizeof(truncated));
    assert(g2(&bad)==0 && bad.error && bad.pos==0);
    bad=packet_wrap(truncated,sizeof(truncated));access_bits(&bad);
    assert(gbit(&bad,9)==0 && bad.error && bad.bit_pos==0);
    bad=packet_wrap(truncated,sizeof(truncated));char no_string[4];
    gjstr(&bad,no_string,sizeof(no_string));assert(bad.error);
    puts("  Truncated byte/bit/string reads report bounds errors without overread");

'''+s[idx:];p.write_text(s)
