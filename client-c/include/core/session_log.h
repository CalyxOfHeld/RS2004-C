#ifndef RS2_SESSION_LOG_H
#define RS2_SESSION_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A line-oriented record of everything nondeterministic a client run consumes:
// frames, client cycles, input, clock reads and every network stream call with
// its result and bytes. Replaying it feeds those values back in the same order,
// so the unchanged client code reproduces the run; any call that differs from
// the recording (including different outgoing bytes) is reported as a desync.
//
// Each line is "<tag> <fields...>". Integers are decimal, byte strings are hex
// with "-" for empty.

typedef enum { SESSION_LOG_OFF, SESSION_LOG_RECORD, SESSION_LOG_REPLAY } SessionLogMode;

bool session_log_record(const char *path);
bool session_log_replay(const char *path);
void session_log_close(void);
SessionLogMode session_log_mode(void);
bool session_log_failed(void);
long session_log_line(void);   // current replay line number (1-based)

// Recording: one line built from a tag and a sequence of fields.
void session_log_begin(const char *tag);
void session_log_int(long long value);
void session_log_bytes(const uint8_t *data, size_t length);
void session_log_text(const char *text);   // a single token without spaces
void session_log_end(void);

// Replay: the next line's tag, or NULL at end of log.
const char *session_log_peek(void);
// Consumes the next line, which must carry this tag; otherwise reports a desync.
bool session_log_take(const char *tag);
bool session_log_take_int(long long *value);
// Reads a byte field; capacity bounds the copy, *length is the field length.
bool session_log_take_bytes(uint8_t *data, size_t capacity, size_t *length);
bool session_log_take_text(char *text, size_t capacity);
void session_log_desync(const char *what);
bool session_log_at_end(void);

#endif // RS2_SESSION_LOG_H
