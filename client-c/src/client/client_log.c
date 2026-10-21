#include "client/client_log.h"
#include "core/session_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned held_bits;

static char *copy_text(const char *text) {
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy) memcpy(copy, text, size);
    return copy;
}

static unsigned held_pack(const ClientHeldKeys *k) {
    return (unsigned)k->left | (unsigned)k->right << 1 | (unsigned)k->up << 2 | (unsigned)k->down << 3 |
           (unsigned)k->w << 4 | (unsigned)k->a << 5 | (unsigned)k->s << 6 | (unsigned)k->d << 7 |
           (unsigned)k->ctrl << 8;
}

static void held_unpack(unsigned bits, ClientHeldKeys *k) {
    k->left = bits & 1; k->right = bits >> 1 & 1; k->up = bits >> 2 & 1; k->down = bits >> 3 & 1;
    k->w = bits >> 4 & 1; k->a = bits >> 5 & 1; k->s = bits >> 6 & 1; k->d = bits >> 7 & 1;
    k->ctrl = bits >> 8 & 1;
}

bool client_log_write_header(int argc, char **argv, int64_t seed, const char *username) {
    session_log_begin("RS2REC"); session_log_int(1); session_log_end();
    session_log_begin("S"); session_log_int(seed); session_log_end();
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--record") && i + 1 < argc) { i++; continue; }
        session_log_begin("A"); session_log_bytes((const uint8_t *)argv[i], strlen(argv[i])); session_log_end();
    }
    session_log_begin("L");
    session_log_bytes((const uint8_t *)(username ? username : ""), username ? strlen(username) : 0);
    session_log_end();
    session_log_begin("E0"); session_log_end();
    return true;
}

bool client_log_read_header(ClientLogHeader *header) {
    memset(header, 0, sizeof(*header));
    long long value;
    if (!session_log_take("RS2REC") || !session_log_take_int(&value) || value != 1) {
        fprintf(stderr, "[REPLAY] not an RS2REC 1 session log\n"); return false;
    }
    if (!session_log_take("S") || !session_log_take_int(&value)) return false;
    header->seed = value;
    header->argv[header->argc++] = copy_text("rs2replay");
    const char *tag;
    while ((tag = session_log_peek()) && !strcmp(tag, "A")) {
        uint8_t text[1024]; size_t length = 0;
        session_log_take("A");
        if (!session_log_take_bytes(text, sizeof(text) - 1, &length) || length >= sizeof(text) ||
            header->argc >= CLIENT_LOG_MAX_ARGS) { session_log_desync("bad argument record"); return false; }
        text[length] = 0;
        header->argv[header->argc++] = copy_text((const char *)text);
    }
    uint8_t name[sizeof(header->username)]; size_t length = 0;
    if (!session_log_take("L") || !session_log_take_bytes(name, sizeof(name) - 1, &length) || length >= sizeof(name)) return false;
    memcpy(header->username, name, length);
    header->username[length] = 0;
    return session_log_take("E0");
}

void client_log_free_header(ClientLogHeader *header) {
    for (int i = 0; i < header->argc; i++) free(header->argv[i]);
    header->argc = 0;
}

void client_log_frame(int frame) {
    session_log_begin("F"); session_log_int(frame); session_log_end();
}

void client_log_event(const ClientEvent *e) {
    session_log_begin("E");
    session_log_int(e->type); session_log_int(e->x); session_log_int(e->y);
    session_log_int(e->button); session_log_int(e->key); session_log_int(e->wheel);
    session_log_bytes((const uint8_t *)e->text, e->type == CLIENT_EVENT_TEXT ? strlen(e->text) : 0);
    session_log_end();
}

void client_log_held(const ClientHeldKeys *keys) {
    unsigned bits = held_pack(keys);
    if (bits == held_bits) return;
    held_bits = bits;
    session_log_begin("K"); session_log_int(bits); session_log_end();
}

void client_log_update(void) { session_log_begin("U"); session_log_end(); }

void client_log_draw(uint64_t hash) {
    session_log_begin("D"); session_log_int((long long)hash); session_log_end();
}

uint64_t client_log_time(uint64_t (*clock)(void)) {
    if (session_log_mode() == SESSION_LOG_REPLAY) {
        long long value = 0;
        if (!session_log_take("T") || !session_log_take_int(&value)) return 0;
        return (uint64_t)value;
    }
    uint64_t now = clock();
    if (session_log_mode() == SESSION_LOG_RECORD) { session_log_begin("T"); session_log_int((long long)now); session_log_end(); }
    return now;
}

bool client_log_take_event(ClientEvent *e) {
    long long v[6];
    memset(e, 0, sizeof(*e));
    if (!session_log_take("E")) return false;
    for (int i = 0; i < 6; i++) if (!session_log_take_int(&v[i])) return false;
    size_t length = 0;
    if (!session_log_take_bytes((uint8_t *)e->text, sizeof(e->text) - 1, &length) || length >= sizeof(e->text)) {
        session_log_desync("bad event text"); return false;
    }
    e->text[length] = 0;
    e->type = (ClientEventType)v[0]; e->x = (int)v[1]; e->y = (int)v[2];
    e->button = (int)v[3]; e->key = (int)v[4]; e->wheel = (int)v[5];
    return true;
}

bool client_log_take_held(ClientHeldKeys *keys) {
    long long bits = 0;
    if (!session_log_take("K") || !session_log_take_int(&bits)) return false;
    held_bits = (unsigned)bits;
    held_unpack(held_bits, keys);
    return true;
}
