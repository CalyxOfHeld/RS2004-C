#include "core/jstring.h"
#include <string.h>

static const char BASE37_LOOKUP[38] = "_abcdefghijklmnopqrstuvwxyz0123456789";

uint64_t jstring_to_base37(const char *str) {
    if (!str) return 0;
    uint64_t res = 0;
    for (int i = 0; str[i] && i < 12; i++) {
        char c = str[i];
        res *= 37ULL;
        if (c >= 'A' && c <= 'Z') {
            res += (uint64_t)(c + 1 - 'A');
        } else if (c >= 'a' && c <= 'z') {
            res += (uint64_t)(c + 1 - 'a');
        } else if (c >= '0' && c <= '9') {
            res += (uint64_t)(c + 27 - '0');
        }
    }
    while (res % 37ULL == 0 && res != 0) {
        res /= 37ULL;
    }
    return res;
}

void jstring_from_base37(uint64_t val, char *out_str, size_t max_len) {
    if (!out_str || max_len == 0) return;
    if (val <= 0 || val >= 6582952005840035281ULL || val % 37ULL == 0) {
        strncpy(out_str, "invalid_name", max_len);
        out_str[max_len - 1] = '\0';
        return;
    }

    char buf[13];
    int pos = 0;
    while (val != 0) {
        uint64_t prev = val;
        val /= 37ULL;
        buf[11 - pos++] = BASE37_LOOKUP[(int)(prev - val * 37ULL)];
    }

    int len = 0;
    for (int i = 12 - pos; i < 12 && (size_t)len < max_len - 1; i++) {
        out_str[len++] = buf[i];
    }
    out_str[len] = '\0';
}

void jstring_format_name(char *name) {
    if (!name || name[0] == '\0') return;
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++) {
        if (name[i] == '_') {
            name[i] = ' ';
            if (i + 1 < len && name[i + 1] >= 'a' && name[i + 1] <= 'z') {
                name[i + 1] = (char)(name[i + 1] + 'A' - 'a');
            }
        }
    }
    if (name[0] >= 'a' && name[0] <= 'z') {
        name[0] = (char)(name[0] + 'A' - 'a');
    }
}
