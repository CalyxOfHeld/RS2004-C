#ifndef RS2_WORDFILTER_H
#define RS2_WORDFILTER_H
#include "core/jagfile.h"

/* Cache-backed revision225 WordFilter.
 * Original chars are g1 values; pair bytes are signed as in Java byte[][]. */
typedef struct {
    uint8_t *chars;
    size_t length;
    int8_t (*combinations)[2];
    size_t combination_count;
    int type;
} WordFilterWord;
typedef struct {
    uint16_t *fragments;
    size_t fragment_count;
    WordFilterWord *bad_words, *domains, *tlds;
    size_t bad_count, domain_count, tld_count;
} WordFilterTables;

/* Initialize to zero. On success replace/free existing tables; failure leaves
 * them unchanged. free is idempotent. Valid data follows original reads; trailing
 * bytes are ignored as original. Invalid/truncated counts/allocations fail. */
bool wordfilter_tables_decode(WordFilterTables *tables, const uint8_t *fragments, size_t fragments_size,
    const uint8_t *bad, size_t bad_size, const uint8_t *domains, size_t domains_size,
    const uint8_t *tlds, size_t tlds_size);
bool wordfilter_tables_load(WordFilterTables *tables, Jagfile *archive);
void wordfilter_tables_free(WordFilterTables *tables);
/* Original UTF-16 char[] primitives, not the complete filter. normalize keeps
 * original length, compacts repeated literal spaces and pads tail with spaces.
 * Tab/newline,ASCII32..127,pound0xa3 and Euro0x20ac are allowed. Case helpers use
 * original ASCII predicates, preserve masked '*', and do not require NUL.
 * NULL arrays fail only when length is nonzero. */
bool wordfilter_normalize(uint16_t *text,size_t length);
bool wordfilter_restore_case(uint16_t *filtered,const uint16_t *original,size_t length);
bool wordfilter_format_case(uint16_t *text,size_t length);
/* Complete pipeline using unmodified successfully loaded real-cache tables.
 * UTF-16 units and English/root casing; output capacity includes a NUL unit.
 * Input/output may overlap. On failure output/input/tables stay unchanged and
 * optional out_length becomes0. Empty input may be NULL. Invalid table metadata,
 * zero-length patterns/missing fragment table, NULL output, insufficient capacity,
 * lengths above INT_MAX-2, size overflow or allocation failure return false.
 * Caller-owned table storage must remain valid/unmodified during this call.
 * out_length, if provided, must not alias input/output/table storage.
 * This is filtering only; transport/composition and text conversion are separate. */
bool wordfilter_apply(const WordFilterTables *tables,const uint16_t *input,size_t length,
    uint16_t *output,size_t capacity,size_t *out_length);
#endif
