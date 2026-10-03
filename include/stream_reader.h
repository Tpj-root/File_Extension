#pragma once
#include "tp_format.h"
#include "tp_varint.h"
#include <istream>
#include <string>
#include <stdexcept>
#include <cstring>

namespace tp {

// Pipe-friendly reader: works with std::cin, network stream, etc.
class StreamTpReader {
public:
    explicit StreamTpReader(std::istream& in) : in_(in) {}

    void start() {
        in_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (in_.gcount() != (std::streamsize)sizeof(header_))
            throw std::runtime_error("Truncated header");
        if (std::memcmp(header_.magic, TP_MAGIC, 4) != 0)
            throw std::runtime_error("Bad magic");
        if (header_.version != TP_VERSION)
            throw std::runtime_error("Unsupported version");
        if (header_.count > 0) {
            in_.read(reinterpret_cast<char*>(&curT_), 8);
            in_.read(reinterpret_cast<char*>(&curP_), 8);
            if (in_.gcount() != 8) throw std::runtime_error("Truncated body");
            idx_ = 1;
        }
    }

    const TpHeader& header() const { return header_; }

    bool next(Tick& t) {
        if (idx_ == 0) {
            if (header_.count == 0) return false;
            t.time = curT_; t.price = double(curP_) / double(header_.price_scale);
            return true;
        }
        if (idx_ >= header_.count) return false;
        const int64_t dT = zigzagDecode(getVarint(in_));
        const int64_t dP = zigzagDecode(getVarint(in_));
        curT_ += dT; curP_ += dP; ++idx_;
        t.time = curT_; t.price = double(curP_) / double(header_.price_scale);
        return true;
    }

private:
    std::istream& in_;
    TpHeader      header_{};
    int64_t       curT_ = 0, curP_ = 0;
    uint64_t      idx_  = 0;
};

} // namespace tp