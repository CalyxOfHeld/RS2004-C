#ifndef RS2_JSTRING_H
#define RS2_JSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

uint64_t jstring_to_base37(const char *str);
void     jstring_from_base37(uint64_t val, char *out_str, size_t max_len);
void     jstring_format_name(char *name);

#endif // RS2_JSTRING_H
