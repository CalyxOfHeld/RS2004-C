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
} ClientStream;

bool client_stream_global_init(void);
void client_stream_global_cleanup(void);

bool client_stream_connect(ClientStream *stream, const char *host, int port);
int  client_stream_available(ClientStream *stream);
bool client_stream_read(ClientStream *stream, uint8_t *dst, size_t len);
int  client_stream_read_byte(ClientStream *stream);
bool client_stream_write(ClientStream *stream, const uint8_t *src, size_t len);
void client_stream_close(ClientStream *stream);

#endif // RS2_CLIENT_STREAM_H
