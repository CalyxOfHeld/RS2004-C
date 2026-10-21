from pathlib import Path
p=Path('include/net/client_stream.h');s=p.read_text().replace('int  client_stream_available(ClientStream *stream);','''int  client_stream_available(ClientStream *stream);
/* 1 ready, 0 not ready, -1 error. No directory or external network access. */
int  client_stream_wait_readable(ClientStream *stream, int timeout_ms);
/* Reads only currently ready bytes: 0 would block, -1 closed/error. */
int  client_stream_read_some(ClientStream *stream, uint8_t *dst, size_t capacity);''');p.write_text(s)
p=Path('src/net/client_stream.c');s=p.read_text().replace('#include <string.h>','#include <string.h>\n#include <limits.h>\n#ifndef _WIN32\n#include <sys/ioctl.h>\n#include <sys/select.h>\n#include <netinet/tcp.h>\n#endif');idx=s.index('bool client_stream_read(');s=s[:idx]+'''int client_stream_wait_readable(ClientStream *stream, int timeout_ms) {
    if (!stream || !stream->connected || stream->sock == INVALID_SOCKET_VAL) return -1;
    fd_set readfds; FD_ZERO(&readfds); FD_SET(stream->sock, &readfds);
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
#ifdef _WIN32
    int ready = select(0, &readfds, NULL, NULL, &timeout);
#else
    int ready = select(stream->sock + 1, &readfds, NULL, NULL, &timeout);
#endif
    if (ready < 0) { stream->connected = false; return -1; }
    return ready ? 1 : 0;
}

int client_stream_read_some(ClientStream *stream, uint8_t *dst, size_t capacity) {
    if (!capacity || !dst) return -1;
    int ready = client_stream_wait_readable(stream, 0);
    if (ready <= 0) return ready;
    int limit = capacity > INT_MAX ? INT_MAX : (int)capacity;
    int count = recv(stream->sock, (char *)dst, limit, 0);
    if (count <= 0) { stream->connected = false; return -1; }
    return count;
}

'''+s[idx:];p.write_text(s)
p=Path('include/net/protocol.h');s=p.read_text().replace('    uint32_t archive_crcs[9];','''    uint32_t archive_crcs[9];
    int packet_opcode; /* -1 until the next encrypted opcode is available */
    int packet_size;
    uint8_t packet_header[2];
    size_t packet_header_read;
    size_t packet_payload_read;''');s=s.replace('uint8_t *out_payload, size_t *out_len','uint8_t *out_payload, size_t capacity, size_t *out_len');s=s.replace('bool protocol_send_idle(GameSession *session);','bool protocol_send_idle(GameSession *session);\nbool protocol_send_keepalive(GameSession *session);');p.write_text(s)
p=Path('src/net/protocol.c');s=p.read_text().replace('server_packet_sizes[i] = 0;','server_packet_sizes[i] = -3; // unknown opcode must never be treated as empty');idx=s.index('    packet_sizes_init = true;');s=s[:idx]+'''    // ServerGameZoneProt.ts: these may also arrive as standalone packets.
    server_packet_sizes[23]  = 14; // LOC_MERGE
    server_packet_sizes[42]  = 4;  // LOC_ANIM
    server_packet_sizes[49]  = 3;  // OBJ_DEL
    server_packet_sizes[50]  = 7;  // OBJ_REVEAL
    server_packet_sizes[59]  = 4;  // LOC_ADD_CHANGE
    server_packet_sizes[69]  = 15; // MAP_PROJANIM
    server_packet_sizes[76]  = 2;  // LOC_DEL
    server_packet_sizes[151] = 7;  // OBJ_COUNT
    server_packet_sizes[191] = 6;  // MAP_ANIM
    server_packet_sizes[223] = 5;  // OBJ_ADD

'''+s[idx:];s=s.replace('    init_packet_sizes();\n}', '    session->stream.sock = INVALID_SOCKET_VAL;\n    session->packet_opcode = -1;\n    init_packet_sizes();\n}',1)
start=s.index('int protocol_read_packet(');end=s.index('bool protocol_send_idle(',start)
s=s[:start]+'''static int protocol_receive_error(GameSession *session, const char *reason) {
    fprintf(stderr, "[Protocol] Receive failed: %s\\n", reason);
    protocol_close_session(session);
    return -1;
}

int protocol_poll_packet(GameSession *session, int *out_opcode, uint8_t *out_payload,
                         size_t capacity, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!session || !session->in_game || !out_opcode || !out_len || (!out_payload && capacity)) return -1;
    if (session->packet_opcode < 0) {
        uint8_t raw;
        int count = client_stream_read_some(&session->stream, &raw, 1);
        if (count < 0) return protocol_receive_error(session, "connection closed or socket error");
        if (!count) return 0;
        session->packet_opcode = (raw - isaac_next(&session->in_cipher)) & 255;
        session->packet_size = server_packet_sizes[session->packet_opcode];
        session->packet_header_read = session->packet_payload_read = 0;
        if (session->packet_size == -3) return protocol_receive_error(session, "unknown server opcode");
    }
    if (session->packet_size < 0) {
        size_t required = (size_t)-session->packet_size;
        int count = client_stream_read_some(&session->stream,
            session->packet_header + session->packet_header_read, required - session->packet_header_read);
        if (count < 0) return protocol_receive_error(session, "connection closed in packet length");
        session->packet_header_read += (size_t)count;
        if (session->packet_header_read < required) return 0;
        session->packet_size = required == 1 ? session->packet_header[0] :
            (session->packet_header[0] << 8) | session->packet_header[1];
    }
    size_t length = (size_t)session->packet_size;
    if (length > capacity || length > sizeof(session->stream.in_buf))
        return protocol_receive_error(session, "payload exceeds destination capacity");
    if (session->packet_payload_read < length) {
        int count = client_stream_read_some(&session->stream,
            session->stream.in_buf + session->packet_payload_read, length - session->packet_payload_read);
        if (count < 0) return protocol_receive_error(session, "connection closed in packet payload");
        session->packet_payload_read += (size_t)count;
        if (session->packet_payload_read < length) return 0;
    }
    if (length) memcpy(out_payload, session->stream.in_buf, length);
    *out_opcode = session->packet_opcode;
    *out_len = length;
    session->packet_opcode = -1;
    return 1;
}

int protocol_read_packet(GameSession *session, uint8_t *out_payload, size_t capacity, size_t *out_len) {
    int opcode;
    for (;;) {
        int result = protocol_poll_packet(session, &opcode, out_payload, capacity, out_len);
        if (result < 0) return -1;
        if (result > 0) return opcode;
        if (client_stream_wait_readable(&session->stream, 3000) <= 0)
            return protocol_receive_error(session, "timed out waiting for a complete packet");
    }
}

bool protocol_send_keepalive(GameSession *session) {
    if (!session || !session->in_game) return false;
    uint8_t opcode = (uint8_t)(108 + isaac_next(&session->out_cipher)); // NO_TIMEOUT
    return client_stream_write(&session->stream, &opcode, 1);
}

'''+s[end:];s=s.replace('    size_t payload_len = 1 + 2 + 2 + count * 2;','    if (count < 0 || count > 24 || (count && (!rel_x || !rel_z))) return false;\n    size_t payload_len = 1 + 2 + 2 + (size_t)count * 2;');s=s.replace('if (len + 3 > sizeof(buf)) return false;', 'if (len + 3 > sizeof(buf) || len + 1 > 255) return false;').replace('p1(&pkt, (int32_t)len);\n    for (size_t i = 0; i < len; i++) {','p1(&pkt, (int32_t)len + 1);\n    for (size_t i = 0; i < len; i++) {').replace('''    return client_stream_write(&session->stream, buf, pkt.pos);
}

void protocol_close_session''','''    p1(&pkt, 10); // ClientCheatDecoder.gjstr expects the revision-225 delimiter.
    return client_stream_write(&session->stream, buf, pkt.pos);
}

void protocol_close_session''');s=s.replace('    session->in_game = false;\n}', '    session->in_game = false;\n    session->packet_opcode = -1;\n    session->packet_header_read = session->packet_payload_read = 0;\n}');p.write_text(s)
p=Path('platform/sdl2_main.c');s=p.read_text().replace('protocol_poll_packet(&session, &opcode, payload, &pkt_len)','protocol_poll_packet(&session, &opcode, payload, sizeof(payload), &pkt_len)');p.write_text(s)
p=Path('tests/test_net_login.c');s=p.read_text().replace('protocol_read_packet(&session, payload, &pkt_len)','protocol_read_packet(&session, payload, sizeof(payload), &pkt_len)');p.write_text(s)
