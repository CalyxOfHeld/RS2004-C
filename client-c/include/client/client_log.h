#ifndef RS2_CLIENT_LOG_H
#define RS2_CLIENT_LOG_H

#include <stdbool.h>
#include <stdint.h>
#include "client/client.h"

// Client-level records in the session log (core/session_log.h). A recording is
//   RS2REC 1, S <seed>, A <arg>... , L <username hex>, E0 (end of header)
// then per frame
//   F <frame>, E <event>..., K <held bits>?, U per client cycle, D <hash>
// with T <ms> clock reads and N* stream calls interleaved where they occurred.

#define CLIENT_LOG_MAX_ARGS 64

typedef struct {
    int64_t seed;
    int argc;                 // includes a program name at argv[0]
    char *argv[CLIENT_LOG_MAX_ARGS];
    char username[16];
} ClientLogHeader;

bool client_log_write_header(int argc, char **argv, int64_t seed, const char *username);
bool client_log_read_header(ClientLogHeader *header);
void client_log_free_header(ClientLogHeader *header);

void client_log_frame(int frame);
void client_log_event(const ClientEvent *event);
// Writes K only when the held keys change; replays them the same way.
void client_log_held(const ClientHeldKeys *keys);
void client_log_update(void);
void client_log_draw(uint64_t hash);
// The platform clock with each read recorded or replayed.
uint64_t client_log_time(uint64_t (*clock)(void));

bool client_log_take_event(ClientEvent *event);
bool client_log_take_held(ClientHeldKeys *keys);

#endif // RS2_CLIENT_LOG_H
