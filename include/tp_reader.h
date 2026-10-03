#pragma once
#include "tp_format.h"
#include "tp_varint.h"
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace tp {

class TpReader {
public:
    explicit TpReader(std::string path) : path_(std::move(path)) {}

    void open() {
        in_.open(path_, std::ios::binary);
        if (!in_) throw std::runtime_error("Cannot open " + path_);
        in_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (in_.gcount() != (std::streamsize)sizeof(header_))
            throw std::runtime_error("Truncated .tp header: " + path_);
        if (std::memcmp(header_.magic, TP_MAGIC, 4) != 0)
            throw std::runtime_error("Bad magic - not a .tp file");
        if (header_.version != TP_VERSION)
            throw std::runtime_error("Unsupported .tp version");

        if (header_.count > 0) {
            in_.read(reinterpret_cast<char*>(&curT_), sizeof(curT_));
            in_.read(reinterpret_cast<char*>(&curP_), sizeof(curP_));
            if (in_.gcount() != (std::streamsize)sizeof(curP_))
                throw std::runtime_error("Truncated .tp body");
            idx_ = 1;
        }
    }

    const TpHeader& header() const { return header_; }

    bool next(Tick& outTick) {
        if (idx_ == 0) {
            if (header_.count == 0) return false;
            outTick.time  = curT_;
            outTick.price = double(curP_) / double(header_.price_scale);
            return true;
        }
        if (idx_ >= header_.count) return false;

        const int64_t dT = zigzagDecode(getVarint(in_));
        const int64_t dP = zigzagDecode(getVarint(in_));
        curT_ += dT; curP_ += dP; ++idx_;
        outTick.time  = curT_;
        outTick.price = double(curP_) / double(header_.price_scale);
        return true;
    }

    std::vector<Tick> readAll() {
        std::vector<Tick> v;
        v.reserve((size_t)header_.count);
        Tick t;
        while (next(t)) v.push_back(t);
        return v;
    }

private:
    std::string path_;
    std::ifstream in_;
    TpHeader  header_{};
    int64_t   curT_ = 0, curP_ = 0;
    uint64_t  idx_  = 0;
};

} // namespace tp