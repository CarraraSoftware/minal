// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
// DOC:
// int c_utf8_buf_to_utf32_char(uint32_t* out_char32, const char* utf8_buf, const char* utf8_buf_end, int* opt_error);
//      convert utf8 to a 32-bit unicode character
//      return number of bytes processed from the input stream
//      optional error argument is provided
//      regardless of the error argument the function will never return 0
//      at least 1 byte will always be skipped
//
// int c_utf32_char_to_utf8_buf(char* out_utf8_buf, const char* utf8_buf_end, const uint32_t char32);
//      convert a 32-bit unicode character to utf8 string
//      return number of bytes written to buf or 0 on error (invalid unicode char, or buf too small)
//      will not write on utf8_buf_end or beyond
//
// typedef struct c_utf8_char_t {
//     char buf[5];
//     uint16_t len;
// } c_utf8_char;
//
// int c_utf8_char_to_buf(char* out_utf8_buf, const char* utf8_buf_end, c_utf8_char_t char_utf8);
//      write a c_utf8_char_t to a buffer
//      returns number of bytes written on 0 if buffer is not big enough
//      no bytes will be written if the buffer is not big enough
//
// c_utf8_char_t c_utf32_char_to_utf8_char(const uint32_t char32);
//      convert a 32-bit unicode character to a utf8 character
//      returns a char with len = 0 on error (invalid unicode char)
//
// uint32_t c_utf8_char_to_utf32_char(const c_utf8_char_t char_utf8, int* opt_error);
//      conver a utf8 character to a 32-but unicode character
//      optional error argument is provided
//
// int c_utf8_buf_to_utf32_char_b(uint32_t* out_char32, const char* utf8_buf, int* opt_error);
//      branchless convert utf8 to a 32-bit unicode character
//      ! utf8_buf must point to at least 4 bytes with zeroes after the end of the string
//      return number of bytes processed from the output stream
//      optional error argument is provided
//      regardless of the error argument the function will never return 0
//      at least 1 byte will always be skipped

#pragma once
#include <stdint.h>
#include <assert.h>

#define C_UTF8_EMPTY_VAL {0}
#define C_UTF8_INLINE_FUNC static inline
#define C_UTF8_INLINE_VAR static
#define C_UTF8_CONST const

C_UTF8_INLINE_VAR C_UTF8_CONST uint32_t C_UTF32_MAX_CODEPOINT = 0x10FFFF;
C_UTF8_INLINE_VAR C_UTF8_CONST uint32_t C_UTF32_INVALID_CODEPOINT = 0xFFFD;

typedef struct c_utf8_char_t {
    char buf[5];  // 4 chars + 1 for zero termination to be nice
    uint16_t len; // uint16 so struct fits in 8 bytes
} c_utf8_char_t;

// based on https://github.com/skeeto/branchless-utf8 by Christopher Wellons
C_UTF8_INLINE_FUNC int c_utf8_buf_to_utf32_char_b(uint32_t* out_char32, const char* utf8_buf, int* opt_error) {
    static C_UTF8_CONST char lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };
    static C_UTF8_CONST int masks[] = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
    static C_UTF8_CONST int shiftc[] = {0, 18, 12, 6, 0};

    const uint8_t* const s = (const uint8_t*)utf8_buf;
    int len = lengths[s[0] >> 3];

    /* Assume a four-byte character and load four bytes. Unused bits are
     * shifted out.
     */
    *out_char32 = (uint32_t)(s[0] & masks[len]) << 18;
    *out_char32 |= (uint32_t)(s[1] & 0x3f) << 12;
    *out_char32 |= (uint32_t)(s[2] & 0x3f) << 6;
    *out_char32 |= (uint32_t)(s[3] & 0x3f) << 0;
    *out_char32 >>= shiftc[len];

    if (opt_error) {
        static C_UTF8_CONST uint32_t mins[] = {4194304, 0, 128, 2048, 65536};
        static C_UTF8_CONST int shifte[] = {0, 6, 4, 2, 0};
        int* const e = opt_error;
        const uint32_t c = *out_char32;
        /* Accumulate the various error conditions. */
        *e = (c < mins[len]) << 6; // non-canonical encoding
        *e |= ((c >> 11) == 0x1b) << 7;  // surrogate half?
        *e |= (c > C_UTF32_MAX_CODEPOINT) << 8;  // out of range?
        *e |= (s[1] & 0xc0) >> 2;
        *e |= (s[2] & 0xc0) >> 4;
        *e |= (s[3]) >> 6;
        *e ^= 0x2a; // top two bits of each tail byte correct?
        *e >>= shifte[len];
    }

    return len + !len;
}

C_UTF8_INLINE_FUNC uint32_t c_utf8_char_to_utf32_char(const c_utf8_char_t char_utf8, int* opt_error) {
    uint32_t ret;
    c_utf8_buf_to_utf32_char_b(&ret, char_utf8.buf, opt_error);
    return ret;
}

// Based on stb_to_utf8() from github.com/nothings/stb/
C_UTF8_INLINE_FUNC c_utf8_char_t c_utf32_char_to_utf8_char(const uint32_t char32) {
    c_utf8_char_t ret = C_UTF8_EMPTY_VAL;
    if (char32 < 0x80)
    {
        ret.buf[0] = (char)char32;
        ret.len = 1;
    }
    else if (char32 < 0x800)
    {
        ret.buf[0] = (char)(0xc0 + (char32 >> 6));
        ret.buf[1] = (char)(0x80 + (char32 & 0x3f));
        ret.len = 2;
    }
    else if (char32 < 0x10000)
    {
        ret.buf[0] = (char)(0xe0 + (char32 >> 12));
        ret.buf[1] = (char)(0x80 + ((char32 >> 6) & 0x3f));
        ret.buf[2] = (char)(0x80 + ((char32) & 0x3f));
        ret.len = 3;
    }
    else if (char32 <= 0x10FFFF)
    {
        ret.buf[0] = (char)(0xf0 + (char32 >> 18));
        ret.buf[1] = (char)(0x80 + ((char32 >> 12) & 0x3f));
        ret.buf[2] = (char)(0x80 + ((char32 >> 6) & 0x3f));
        ret.buf[3] = (char)(0x80 + ((char32) & 0x3f));
        ret.len = 4;
    }
    return ret;
}

C_UTF8_INLINE_FUNC int c_utf8_char_to_buf(char* out_utf8_buf, const char* utf8_buf_end, c_utf8_char_t char_utf8) {
    int size = (int)(utf8_buf_end - out_utf8_buf);
    if (size < char_utf8.len) return 0;
    char* psrc = char_utf8.buf;
    char* ptgt = out_utf8_buf;
    while (*psrc) { // assume input is valid
        *ptgt++ = *psrc++;
    }
    return char_utf8.len;
}

C_UTF8_INLINE_FUNC int c_utf8_buf_to_utf32_char(uint32_t* out_char32, const char* utf8_buf, const char* utf8_buf_end, int* opt_error) {
    char s[4] = C_UTF8_EMPTY_VAL;
    if (utf8_buf_end - utf8_buf < 4) {
        int i = 0;
        const char* ptr = utf8_buf;
        while (ptr != utf8_buf_end) {
            s[i++] = *ptr++;
        }
        utf8_buf = s;
    }
    return c_utf8_buf_to_utf32_char_b(out_char32, utf8_buf, opt_error);
}

C_UTF8_INLINE_FUNC int c_utf32_char_to_utf8_buf(char* out_utf8_buf, const char* utf8_buf_end, const uint32_t char32) {
    c_utf8_char_t utf8_char = c_utf32_char_to_utf8_char(char32);
    return c_utf8_char_to_buf(out_utf8_buf, utf8_buf_end, utf8_char);
}

