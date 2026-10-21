from pathlib import Path
p=Path('src/ui/client_actions.c');s=p.read_text().replace('slot=%d\\\\n','slot=%d\\n');p.write_text(s)
p=Path('include/net/protocol.h');s=p.read_text().replace('bool protocol_send_inv_button(','''bool protocol_send_item_on_item(GameSession *session,int item,int slot,int component,int use_item,int use_slot,int use_component);
bool protocol_send_route(GameSession *session,int base_x,int base_z,const int *x,const int *z,int count,bool ctrl);
bool protocol_send_inv_button(''');p.write_text(s)
p=Path('src/net/protocol.c');s=p.read_text();idx=s.index('bool protocol_send_inv_button(');s=s[:idx]+'''bool protocol_send_item_on_item(GameSession *session,int item,int slot,int component,int use_item,int use_slot,int use_component) {
    if(!session || !session->in_game) return false;
    uint8_t bytes[13];Packet p=packet_wrap(bytes,sizeof(bytes));
    p1(&p,130+isaac_next(&session->out_cipher));
    p2(&p,item);p2(&p,slot);p2(&p,component);
    p2(&p,use_item);p2(&p,use_slot);p2(&p,use_component);
    return client_stream_write(&session->stream,bytes,p.pos);
}

bool protocol_send_route(GameSession *session,int base_x,int base_z,const int *x,const int *z,int count,bool ctrl) {
    if(!x || !z || count<=0 || count>10000) return false;
    int indices[25],points=1;indices[0]=0;
    for(int i=1;i<count-1 && points<24;i++) {
        int dx=x[i]-x[i-1],dz=z[i]-z[i-1];
        if(dx!=x[i+1]-x[i] || dz!=z[i+1]-z[i]) indices[points++]=i;
    }
    if(count>1) indices[points++]=count-1;
    int8_t rx[24],rz[24];
    for(int i=1;i<points;i++) {
        int dx=x[indices[i]]-x[0],dz=z[indices[i]]-z[0];
        if(dx < -128 || dx>127 || dz < -128 || dz>127) return false;
        rx[i-1]=(int8_t)dx;rz[i-1]=(int8_t)dz;
    }
    return protocol_send_walk(session,base_x+x[0],base_z+z[0],points-1,rx,rz,ctrl);
}

'''+s[idx:];p.write_text(s)
p=Path('Makefile');s=p.read_text().replace('UI_SRCS     = src/ui/component_packets.c','UI_SRCS     = src/ui/client_actions.c src/ui/component_packets.c');p.write_text(s)
# Bound context menus to the actual framebuffer, rather than the previous cropped frame.
p=Path('src/ui/context_menu.c');s=p.read_text().replace('if (mx + max_w > 765) mx = 765 - max_w;','if (mx + max_w > pix2d_width) mx = pix2d_width - max_w;').replace('if (my + total_h > 503) my = 503 - total_h;','if (my + total_h > pix2d_height) my = pix2d_height - total_h;');p.write_text(s)
