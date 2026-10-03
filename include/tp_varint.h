#pragma once
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace tp {

inline void putVarint(std::ostream& os, uint64_t v) {
    while (v >= 0x80) { os.put(char((v & 0x7F) | 0x80)); v >>= 7; }
    os.put(char(v));
}
inline uint64_t getVarint(std::istream& is) {
    uint64_t v = 0; int shift = 0;
    for (;;) {
        int c = is.get();
        if (c < 0) throw std::runtime_error("Unexpected EOF while reading varint");
        v |= uint64_t(c & 0x7F) << shift;
        if (!(c & 0x80)) break;
        shift += 7;
        if (shift > 63) throw std::runtime_error("varint overflow");
    }
    return v;
}
inline uint64_t zigzagEncode(int64_t v) { return (uint64_t(v) << 1) ^ uint64_t(v >> 63); }
inline int64_t  zigzagDecode(uint64_t v) { return int64_t(v >> 1) ^ -int64_t(v & 1); }

} // namespace tp