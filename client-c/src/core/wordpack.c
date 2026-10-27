/* Bundled revision225 jagex2.wordenc.WordPack.java; no later revision sources. */
#include "core/wordpack.h"
#include <string.h>

static const unsigned char table[] = {
    ' ', 'e', 't', 'a', 'o', 'i', 'h', 'n', 's', 'r', 'd', 'l', 'u', 'm',
    'w', 'c', 'y', 'f', 'g', 'p', 'b', 'v', 'k', 'x', 'j', 'q', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '!', '?', '.',
    ',', ':', ';', '(', ')', '-', '&', '*', '\\', '\'', '@', '#', '+', '=',
    0xa3, '$', '%', '"', '[', ']'
};

static bool fail(Packet *packet) {
    if (packet) packet->error = true;
    return false;
}

bool wordpack_pack(Packet *packet, const char *text, size_t length, bool flush_tail) {
    if (!packet || packet->error || packet->pos > packet->length || (!text && length)) return fail(packet);
    if (length > WORDPACK_MAX_INPUT) length = WORDPACK_MAX_INPUT;
    unsigned char packed[WORDPACK_MAX_PACKED];
    size_t count = 0;
    int carry = -1;
    for (size_t i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch >= 'A' && ch <= 'Z') ch = (unsigned char)(ch + 'a' - 'A');
        int current = 0;
        for (size_t j = 0; j < sizeof(table); j++) if (ch == table[j]) { current = (int)j; break; }
        if (current > 12) current += 195;
        if (carry == -1) {
            if (current < 13) carry = current;
            else packed[count++] = (unsigned char)current;
        } else if (current < 13) {
            packed[count++] = (unsigned char)((carry << 4) + current);
            carry = -1;
        } else {
            packed[count++] = (unsigned char)((carry << 4) + (current >> 4));
            carry = current & 15;
        }
    }
    if (flush_tail && carry != -1) packed[count++] = (unsigned char)(carry << 4);
    if (count > packet->length - packet->pos || (count && !packet->data)) return fail(packet);
    if (count) memcpy(packet->data + packet->pos, packed, count);
    packet->pos += count;
    return true;
}

bool wordpack_unpack(Packet *packet, size_t packed_length, char *text,
                     size_t capacity, size_t *out_length) {
    if (out_length) *out_length = 0;
    if (!packet || packet->error || packet->pos > packet->length || !text || !capacity ||
        packed_length > packet->length - packet->pos || packed_length > WORDPACK_MAX_OUTPUT ||
        (packed_length && !packet->data)) return fail(packet);
    char decoded[WORDPACK_MAX_OUTPUT + 1];
    size_t count = 0;
    int carry = -1;
    for (size_t i = 0; i < packed_length; i++) {
        unsigned char value = packet->data[packet->pos + i];
        for (int shift = 4; shift >= 0; shift -= 4) {
            int nibble = (value >> shift) & 15;
            int index;
            if (carry != -1) { index = (carry << 4) + nibble - 195; carry = -1; }
            else if (nibble < 13) index = nibble;
            else { carry = nibble; continue; }
            if (count >= WORDPACK_MAX_OUTPUT) return fail(packet);
            decoded[count++] = (char)table[index];
        }
    }
    bool uppercase = true;
    for (size_t i = 0; i < count; i++) {
        char ch = decoded[i];
        if (uppercase && ch >= 'a' && ch <= 'z') {
            decoded[i] = (char)(ch + 'A' - 'a'); uppercase = false;
        }
        if (ch == '.' || ch == '!') uppercase = true;
    }
    if (count >= capacity) return fail(packet);
    decoded[count] = 0;
    memcpy(text, decoded, count + 1);
    packet->pos += packed_length;
    if (out_length) *out_length = count;
    return true;
}
