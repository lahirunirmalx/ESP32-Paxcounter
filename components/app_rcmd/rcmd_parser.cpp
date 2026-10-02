// rcmd_parser.cpp - opcode table dispatch and hex decoding.

#include "rcmd_parser.h"

namespace {

const RcmdEntry *find(const RcmdEntry *table, size_t n, uint8_t opcode) {
    for (size_t i = 0; i < n; i++) {
        if (table[i].opcode == opcode) return &table[i];
    }
    return nullptr;
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}  // namespace

RcmdResult rcmd_execute(const uint8_t *cmd, size_t len, const RcmdEntry *table,
                        size_t table_len, void *ctx) {
    RcmdResult res = {RCMD_OK, 0, 0};
    if (cmd == nullptr || len == 0) {
        res.status = RCMD_EMPTY;
        return res;
    }
    size_t cursor = 0;
    while (cursor < len) {
        const uint8_t op = cmd[cursor];
        const RcmdEntry *e = find(table, table_len, op);
        if (e == nullptr) {
            res.status = RCMD_UNKNOWN_OPCODE;
            res.bad_opcode = op;
            return res;
        }
        cursor++;
        if (cursor + e->nargs > len) {
            res.status = RCMD_MISSING_ARGS;
            res.bad_opcode = op;
            return res;
        }
        if (e->fn != nullptr) e->fn(cmd + cursor, ctx);
        cursor += e->nargs;
        res.executed++;
    }
    return res;
}

size_t rcmd_hex_decode(const char *line, uint8_t *out, size_t cap) {
    if (line == nullptr || out == nullptr) return 0;
    size_t n = 0;
    int hi = -1;
    for (const char *p = line; *p != '\0'; p++) {
        const char c = *p;
        if (c == ' ' || c == '\t' || c == ',' || c == '\r' || c == '\n') {
            if (hi >= 0) return 0;  // odd number of digits in a token
            continue;
        }
        if (c == '0' && (p[1] == 'x' || p[1] == 'X') && hi < 0) {
            p++;  // skip "0x" prefix
            continue;
        }
        const int v = hex_value(c);
        if (v < 0) return 0;
        if (hi < 0) {
            hi = v;
        } else {
            if (n >= cap) return 0;
            out[n++] = static_cast<uint8_t>((hi << 4) | v);
            hi = -1;
        }
    }
    return (hi >= 0) ? 0 : n;
}
