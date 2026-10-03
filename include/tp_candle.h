#pragma once
#include "tp_format.h"
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace tp {

struct Candle {
    int64_t  start_time;  // bucket start (aligned)
    int64_t  end_time;    // bucket start + interval (exclusive)
    double   open, high, low, close;
    uint64_t volume;      // tick count in this candle
};

class CandleBuilder {
public:
    // intervalSeconds > 0. Buckets are aligned to EPOCH: t/interval*interval
    static std::vector<Candle> build(const std::vector<Tick>& ticks, int64_t intervalSeconds) {
        if (intervalSeconds <= 0) throw std::runtime_error("interval must be > 0");
        std::vector<Candle> out;
        if (ticks.empty()) return out;

        int64_t bucket = alignDown(ticks.front().time, intervalSeconds);
        Candle c{};
        c.start_time = bucket;
        c.end_time   = bucket + intervalSeconds;
        c.open = c.high = c.low = c.close = ticks.front().price;
        c.volume = 0;

        for (const auto& t : ticks) {
            int64_t b = alignDown(t.time, intervalSeconds);
            if (b != bucket) {
                out.push_back(c);
                bucket = b;
                c = Candle{};
                c.start_time = bucket;
                c.end_time   = bucket + intervalSeconds;
                c.open = c.high = c.low = c.close = t.price;
                c.volume = 0;
            }
            if (t.price > c.high) c.high = t.price;
            if (t.price < c.low)  c.low  = t.price;
            c.close = t.price;
            ++c.volume;
        }
        out.push_back(c);
        return out;
    }

private:
    static int64_t alignDown(int64_t t, int64_t step) {
        int64_t r = t % step;
        if (r < 0) r += step;
        return t - r;
    }
};

} // namespace tp