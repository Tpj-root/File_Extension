#pragma once
#include "tp_format.h"
#include "tp_varint.h"
#include <fstream>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace tp {

class TpWriter {
public:
    explicit TpWriter(std::string path) : path_(std::move(path)) {}

    void write(const std::vector<Tick>& ticks, int decimals = 2) const {
        if (ticks.empty()) throw std::runtime_error("No ticks to write");
        if (decimals < 0) decimals = 0;
        if (decimals > 9) decimals = 9;
        const uint32_t scale = (uint32_t)std::pow(10u, decimals);

        TpHeader h{};
        std::memcpy(h.magic, TP_MAGIC, 4);
        h.version     = TP_VERSION;
        h.flags       = TP_FLAG_COMPRESSED;
        h.start_time  = ticks.front().time;
        h.end_time    = ticks.back().time;
        h.count       = ticks.size();
        h.price_scale = scale;

        double mn = ticks.front().price, mx = ticks.front().price;
        for (const auto& t : ticks) {
            if (t.price < mn) mn = t.price;
            if (t.price > mx) mx = t.price;
        }
        h.min_price = mn;
        h.max_price = mx;

        std::ofstream out(path_, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create " + path_);

        out.write(reinterpret_cast<const char*>(&h), sizeof(h));

        int64_t prevT = ticks.front().time;
        int64_t prevP = (int64_t)std::llround(ticks.front().price * (double)scale);
        out.write(reinterpret_cast<const char*>(&prevT), sizeof(prevT));
        out.write(reinterpret_cast<const char*>(&prevP), sizeof(prevP));

        for (size_t i = 1; i < ticks.size(); ++i) {
            int64_t curT = ticks[i].time;
            int64_t curP = (int64_t)std::llround(ticks[i].price * (double)scale);
            putVarint(out, zigzagEncode(curT - prevT));
            putVarint(out, zigzagEncode(curP - prevP));
            prevT = curT;
            prevP = curP;
        }
    }

private:
    std::string path_;
};

} // namespace tp