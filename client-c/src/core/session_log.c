#include "core/session_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SessionLogMode mode;
static FILE *file;
static bool failed;
static long line_number;
static char *line;          // replay: the current line, fields consumed from cursor
static size_t line_capacity;
static const char *cursor;
static bool line_loaded, at_end;
static char tag[16];

bool session_log_record(const char *path) {
    file = fopen(path, "wb");
    if (!file) return false;
    mode = SESSION_LOG_RECORD;
    return true;
}

bool session_log_replay(const char *path) {
    file = fopen(path, "rb");
    if (!file) return false;
    mode = SESSION_LOG_REPLAY;
    return true;
}

void session_log_close(void) {
    if (file) fclose(file);
    file = NULL;
    mode = SESSION_LOG_OFF;
    free(line); line = NULL; line_capacity = 0;
}

SessionLogMode session_log_mode(void) { return mode; }
bool session_log_failed(void) { return failed; }
long session_log_line(void) { return line_number; }

static bool writing(void) { return mode == SESSION_LOG_RECORD && file; }

void session_log_begin(const char *name) { if (writing()) fputs(name, file); }
void session_log_int(long long value) { if (writing()) fprintf(file, " %lld", value); }
void session_log_text(const char *text) { if (writing()) fprintf(file, " %s", text && *text ? text : "-"); }
void session_log_end(void) { if (writing()) fputc('\n', file); }

void session_log_bytes(const uint8_t *data, size_t length) {
    static const char digits[] = "0123456789abcdef";
    if (!writing()) return;
    fputc(' ', file);
    if (!length) { fputc('-', file); return; }
    for (size_t i = 0; i < length; i++) {
        fputc(digits[data[i] >> 4], file);
        fputc(digits[data[i] & 15], file);
    }
}

void session_log_desync(const char *what) {
    if (!failed) fprintf(stderr, "[REPLAY] desync at log line %ld: %s\n", line_number, what);
    failed = true;
}

// Loads the next line into the buffer and splits off its tag.
static bool load_line(void) {
    if (line_loaded) return !at_end;
    line_loaded = true;
    size_t length = 0;
    int c;
    while ((c = fgetc(file)) != EOF && c != '\n') {
        if (length + 2 > line_capacity) {
            size_t grown = line_capacity ? line_capacity * 2 : 4096;
            char *next = realloc(line, grown);
            if (!next) { session_log_desync("out of memory"); at_end = true; return false; }
            line = next; line_capacity = grown;
        }
        line[length++] = (char)c;
    }
    if (c == EOF && length == 0) { at_end = true; return false; }
    if (!line) { line = malloc(16); line_capacity = 16; }
    if (length && line[length - 1] == '\r') length--;
    line[length] = '\0';
    line_number++;
    size_t t = 0;
    while (line[t] && line[t] != ' ' && t + 1 < sizeof(tag)) { tag[t] = line[t]; t++; }
    tag[t] = '\0';
    cursor = line + t;
    return true;
}

const char *session_log_peek(void) {
    if (mode != SESSION_LOG_REPLAY || failed || !load_line()) return NULL;
    return tag;
}

bool session_log_at_end(void) {
    return mode == SESSION_LOG_REPLAY && !failed && !load_line();
}

bool session_log_take(const char *expected) {
    if (mode != SESSION_LOG_REPLAY || failed) return false;
    if (!load_line()) {
        char what[64]; snprintf(what, sizeof(what), "log ended, client wanted %s", expected);
        session_log_desync(what); return false;
    }
    if (strcmp(tag, expected) != 0) {
        char what[96]; snprintf(what, sizeof(what), "log has %s, client wanted %s", tag, expected);
        session_log_desync(what); return false;
    }
    line_loaded = false;   // the fields stay readable until the next take/peek
    return true;
}

static const char *next_field(void) {
    while (*cursor == ' ') cursor++;
    if (!*cursor) { session_log_desync("missing field"); return NULL; }
    return cursor;
}

bool session_log_take_int(long long *value) {
    if (failed) return false;
    const char *start = next_field();
    if (!start) return false;
    char *end;
    *value = strtoll(start, &end, 10);
    if (end == start) { session_log_desync("bad integer field"); return false; }
    cursor = end;
    return true;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool session_log_take_bytes(uint8_t *data, size_t capacity, size_t *length) {
    if (failed) return false;
    const char *start = next_field();
    if (!start) return false;
    *length = 0;
    if (*start == '-') { cursor = start + 1; return true; }
    const char *p = start;
    while (*p && *p != ' ') {
        int hi = hex_value(p[0]), lo = p[1] ? hex_value(p[1]) : -1;
        if (hi < 0 || lo < 0) { session_log_desync("bad byte field"); return false; }
        if (*length < capacity) data[*length] = (uint8_t)(hi << 4 | lo);
        (*length)++;
        p += 2;
    }
    cursor = p;
    return true;
}

bool session_log_take_text(char *text, size_t capacity) {
    if (failed || !capacity) return false;
    const char *start = next_field();
    if (!start) return false;
    size_t n = 0;
    while (start[n] && start[n] != ' ') n++;
    if (n == 1 && *start == '-') n = 0;
    else if (n >= capacity) { session_log_desync("text field too long"); return false; }
    memcpy(text, start, n);
    text[n] = '\0';
    cursor = start + (n ? n : 1);
    return true;
}
