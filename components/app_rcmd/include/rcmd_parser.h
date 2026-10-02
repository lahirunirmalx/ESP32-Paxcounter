// rcmd_parser.h - remote command parsing. Pure C++, host-testable.
//
// Wire format (legacy compatible): a sequence of [opcode][args...] with a
// fixed argument count per opcode. Several commands may be concatenated in
// one downlink, e.g. 0a 1e 21 = "set send cycle 60 s" + "save config".
// Multi-byte arguments are big-endian.

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void (*rcmd_fn_t)(const uint8_t *args, void *ctx);

struct RcmdEntry {
    uint8_t opcode;
    uint8_t nargs;
    rcmd_fn_t fn;
};

enum RcmdStatus {
    RCMD_OK = 0,
    RCMD_EMPTY,             // no bytes
    RCMD_UNKNOWN_OPCODE,    // parsing stopped at an unknown opcode
    RCMD_MISSING_ARGS,      // last command was truncated; it was not run
};

struct RcmdResult {
    RcmdStatus status;
    int executed;           // commands run before stopping
    uint8_t bad_opcode;     // valid for UNKNOWN_OPCODE / MISSING_ARGS
};

// Runs every command in cmd[] in order. Stops at the first unknown opcode or
// truncated argument list; commands before it have already run.
RcmdResult rcmd_execute(const uint8_t *cmd, size_t len, const RcmdEntry *table,
                        size_t table_len, void *ctx);

// Decodes a hex text line ("0a 1e", "0A1E", "0x0a 0x1e") into bytes.
// Returns the byte count, or 0 on invalid input or overflow of cap.
size_t rcmd_hex_decode(const char *line, uint8_t *out, size_t cap);

// Big-endian argument helpers.
static inline uint16_t rcmd_be16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}
static inline uint32_t rcmd_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
