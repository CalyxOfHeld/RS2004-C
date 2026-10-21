#include "net/client_stream.h"
#include "core/session_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#endif

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

static bool raw_connect(ClientStream *stream, const char *host, int port) {
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
#elif defined(SO_RCVTIMEO) && defined(SO_SNDTIMEO)
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

static int raw_available(ClientStream *stream) {
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

static int raw_wait_readable(ClientStream *stream, int timeout_ms) {
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

static int raw_read_some(ClientStream *stream, uint8_t *dst, size_t capacity) {
    if (!capacity || !dst) return -1;
    int ready = raw_wait_readable(stream, 0);
    if (ready <= 0) return ready;
    int limit = capacity > INT_MAX ? INT_MAX : (int)capacity;
    int count = recv(stream->sock, (char *)dst, limit, 0);
    if (count <= 0) { stream->connected = false; return -1; }
    return count;
}

static bool raw_read(ClientStream *stream, uint8_t *dst, size_t len) {
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

static bool raw_write(ClientStream *stream, const uint8_t *src, size_t len) {
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

static void raw_close(ClientStream *stream) {
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

// ---------------------------------------------------------------------------
// Session log tap: record each call's outcome, or replay it without a socket.
// A replayed stream is marked connected with no socket; writes must match the
// recorded bytes exactly.

static bool replaying(void) { return session_log_mode() == SESSION_LOG_REPLAY; }
static bool recording(void) { return session_log_mode() == SESSION_LOG_RECORD; }

static void replay_connected(ClientStream *stream) {
    long long connected = 0;
    if (session_log_take_int(&connected)) stream->connected = connected != 0;
}

bool client_stream_connect(ClientStream *stream, const char *host, int port) {
    if (replaying()) {
        memset(stream, 0, sizeof(ClientStream));
        stream->sock = INVALID_SOCKET_VAL;
        long long ok = 0;
        if (!session_log_take("NC") || !session_log_take_int(&ok)) return false;
        stream->connected = ok != 0;
        return ok != 0;
    }
    bool ok = raw_connect(stream, host, port);
    if (recording()) { session_log_begin("NC"); session_log_int(ok); session_log_text(host); session_log_int(port); session_log_end(); }
    return ok;
}

int client_stream_available(ClientStream *stream) {
    if (replaying()) {
        long long count = 0;
        if (!session_log_take("NA") || !session_log_take_int(&count)) return 0;
        return (int)count;
    }
    int count = raw_available(stream);
    if (recording()) { session_log_begin("NA"); session_log_int(count); session_log_end(); }
    return count;
}

int client_stream_wait_readable(ClientStream *stream, int timeout_ms) {
    if (replaying()) {
        long long ready = -1;
        if (!session_log_take("NW") || !session_log_take_int(&ready)) return -1;
        replay_connected(stream);
        return (int)ready;
    }
    int ready = raw_wait_readable(stream, timeout_ms);
    if (recording()) { session_log_begin("NW"); session_log_int(ready); session_log_int(stream && stream->connected); session_log_end(); }
    return ready;
}

int client_stream_read_some(ClientStream *stream, uint8_t *dst, size_t capacity) {
    if (replaying()) {
        long long count = -1;
        size_t length = 0;
        if (!session_log_take("NR") || !session_log_take_int(&count)) return -1;
        replay_connected(stream);
        if (!session_log_take_bytes(dst, capacity, &length)) return -1;
        if (count > 0 && (size_t)count != length) { session_log_desync("read_some length"); return -1; }
        if (length > capacity) { session_log_desync("read_some exceeds the client's buffer"); return -1; }
        return (int)count;
    }
    int count = raw_read_some(stream, dst, capacity);
    if (recording()) {
        session_log_begin("NR"); session_log_int(count); session_log_int(stream && stream->connected);
        session_log_bytes(dst, count > 0 ? (size_t)count : 0); session_log_end();
    }
    return count;
}

bool client_stream_read(ClientStream *stream, uint8_t *dst, size_t len) {
    if (replaying()) {
        long long ok = 0;
        size_t length = 0;
        if (!session_log_take("NB") || !session_log_take_int(&ok)) return false;
        replay_connected(stream);
        if (!session_log_take_bytes(dst, len, &length)) return false;
        if (ok && length != len) { session_log_desync("read length"); return false; }
        return ok != 0;
    }
    bool ok = raw_read(stream, dst, len);
    if (recording()) {
        session_log_begin("NB"); session_log_int(ok); session_log_int(stream && stream->connected);
        session_log_bytes(dst, ok ? len : 0); session_log_end();
    }
    return ok;
}

int client_stream_read_byte(ClientStream *stream) {
    uint8_t b = 0;
    if (client_stream_read(stream, &b, 1)) {
        return (int)b;
    }
    return -1;
}

bool client_stream_write(ClientStream *stream, const uint8_t *src, size_t len) {
    if (replaying()) {
        long long ok = 0;
        size_t length = 0;
        if (!session_log_take("NX") || !session_log_take_int(&ok)) return false;
        replay_connected(stream);
        uint8_t *expected = malloc(len ? len : 1);
        if (!expected) { session_log_desync("out of memory"); return false; }
        bool read = session_log_take_bytes(expected, len, &length);
        bool same = read && length == len && memcmp(expected, src, len) == 0;
        free(expected);
        if (read && !same) {
            char what[96];
            snprintf(what, sizeof(what), "outgoing bytes differ (client wrote %zu, recorded %zu)", len, length);
            session_log_desync(what);
            return false;
        }
        return read && ok != 0;
    }
    bool ok = raw_write(stream, src, len);
    if (recording()) {
        session_log_begin("NX"); session_log_int(ok); session_log_int(stream && stream->connected);
        session_log_bytes(src, len); session_log_end();
    }
    return ok;
}

void client_stream_close(ClientStream *stream) {
    if (replaying()) {
        if (!stream) return;
        if (session_log_take("NZ")) { stream->sock = INVALID_SOCKET_VAL; stream->connected = false; }
        return;
    }
    if (recording() && stream) { session_log_begin("NZ"); session_log_end(); }
    raw_close(stream);
}
