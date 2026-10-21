#ifndef RS2_CLIENT_STREAM_H
#define RS2_CLIENT_STREAM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define INVALID_SOCKET_VAL INVALID_SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET_VAL (-1)
#endif

typedef struct {
    socket_t sock;
    bool connected;
    uint8_t in_buf[65536];
    size_t in_pos;
    size_t in_len;
    // In game the original client appends every packet to `out` (5000 bytes) and
    // writes it once at the end of client.updateGame; buffered mode does the same.
    bool buffered;
    bool out_overflow;
    uint8_t out_buf[5000];
    size_t out_len;
} ClientStream;

bool client_stream_global_init(void);
void client_stream_global_cleanup(void);

bool client_stream_connect(ClientStream *stream, const char *host, int port);
int  client_stream_available(ClientStream *stream);
/* 1 ready, 0 not ready, -1 error. No directory or external network access. */
int  client_stream_wait_readable(ClientStream *stream, int timeout_ms);
/* Reads only currently ready bytes: 0 would block, -1 closed/error. */
int  client_stream_read_some(ClientStream *stream, uint8_t *dst, size_t capacity);
bool client_stream_read(ClientStream *stream, uint8_t *dst, size_t len);
int  client_stream_read_byte(ClientStream *stream);
/* Writes now, or appends to the out buffer in buffered mode (false only on overflow). */
bool client_stream_write(ClientStream *stream, const uint8_t *src, size_t len);
/* Writes and empties the out buffer; true when nothing was pending. */
bool client_stream_flush(ClientStream *stream);
size_t client_stream_pending(const ClientStream *stream);
void client_stream_close(ClientStream *stream);

#endif // RS2_CLIENT_STREAM_H
