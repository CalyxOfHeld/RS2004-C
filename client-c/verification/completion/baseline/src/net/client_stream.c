#include "net/client_stream.h"
#include <stdio.h>
#include <string.h>

bool client_stream_global_init(void) {
#ifdef _WIN32
    WSADATA wsa;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (res != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", res);
        return false;
    }
#endif
    return true;
}

void client_stream_global_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool client_stream_connect(ClientStream *stream, const char *host, int port) {
    memset(stream, 0, sizeof(ClientStream));
    stream->sock = INVALID_SOCKET_VAL;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        // Fallback for localhost
        if (strcmp(host, "localhost") == 0) {
            server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        } else {
            fprintf(stderr, "Invalid address: %s\n", host);
            return false;
        }
    }

    socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET_VAL) {
        fprintf(stderr, "Socket creation failed\n");
        return false;
    }

    // TCP NODELAY
    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));

    // Set 3 second send/receive timeouts to avoid hanging indefinitely
#ifdef _WIN32
    DWORD timeout = 3000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
#endif

    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
        return false;
    }

    stream->sock = s;
    stream->connected = true;
    return true;
}

int client_stream_available(ClientStream *stream) {
    if (!stream || !stream->connected || stream->sock == INVALID_SOCKET_VAL) {
        return 0;
    }
#ifdef _WIN32
    u_long bytes = 0;
    if (ioctlsocket(stream->sock, FIONREAD, &bytes) == 0) {
        return (int)bytes;
    }
#else
    int bytes = 0;
    if (ioctl(stream->sock, FIONREAD, &bytes) == 0) {
        return bytes;
    }
#endif
    return 0;
}

bool client_stream_read(ClientStream *stream, uint8_t *dst, size_t len) {
    if (!stream || !stream->connected || stream->sock == INVALID_SOCKET_VAL) {
        return false;
    }

    size_t total = 0;
    while (total < len) {
        int r = recv(stream->sock, (char *)dst + total, (int)(len - total), 0);
        if (r <= 0) {
            stream->connected = false;
            return false;
        }
        total += (size_t)r;
    }
    return true;
}

int client_stream_read_byte(ClientStream *stream) {
    uint8_t b = 0;
    if (client_stream_read(stream, &b, 1)) {
        return (int)b;
    }
    return -1;
}

bool client_stream_write(ClientStream *stream, const uint8_t *src, size_t len) {
    if (!stream || !stream->connected || stream->sock == INVALID_SOCKET_VAL) {
        return false;
    }

    size_t total = 0;
    while (total < len) {
        int w = send(stream->sock, (const char *)src + total, (int)(len - total), 0);
        if (w <= 0) {
            stream->connected = false;
            return false;
        }
        total += (size_t)w;
    }
    return true;
}

void client_stream_close(ClientStream *stream) {
    if (!stream) return;
    if (stream->sock != INVALID_SOCKET_VAL) {
#ifdef _WIN32
        closesocket(stream->sock);
#else
        close(stream->sock);
#endif
        stream->sock = INVALID_SOCKET_VAL;
    }
    stream->connected = false;
}
