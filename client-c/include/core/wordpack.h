#ifndef RS2_WORDPACK_H
#define RS2_WORDPACK_H

#include "core/packet.h"

#define WORDPACK_MAX_INPUT 80
#define WORDPACK_MAX_PACKED 80
#define WORDPACK_MAX_OUTPUT 100

/* Revision225 WordPack, using the client's single-byte Latin-1 text repertoire
 * (including pound sign 0xa3), not UTF-8. Input length is explicit: embedded NUL
 * is an unsupported character and encodes as space. ASCII A-Z are lowercased,
 * matching Java English/root lowercasing on this repertoire. Other unsupported
 * byte values encode as space. No locale-dependent or Unicode conversion here.
 * Only the first80 input bytes are packed. flush_tail matches Java's boolean
 * argument; gameplay uses true. false intentionally discards a pending nibble.
 * Packing does not apply WordFilter and must not bypass it in a caller.
 *
 * Both operations are transactional: failure sets packet.error, leaves packet
 * position and buffers unchanged, and returns false. A pre-existing packet error
 * stays sticky. NULL text is allowed only with zero length. */
bool wordpack_pack(Packet *packet, const char *text, size_t length, bool flush_tail);

/* Decodes exactly packed_length bytes. Output includes original padding spaces,
 * ignores a trailing unmatched extended nibble, and capitalizes initially and
 * after '.'/'!' (not '?'). Capacity includes the final NUL. The original100-char
 * scratch limit is enforced safely; overflow/short input/capacity fails without
 * partial output. out_length is optional and is zeroed on failure. */
bool wordpack_unpack(Packet *packet, size_t packed_length, char *text,
                     size_t capacity, size_t *out_length);

#endif
