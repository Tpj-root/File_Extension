#pragma once
#include "tp_format.h"
#include <vector>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace tp {

struct BucketStats {
    int64_t  start_time      = 0;
    int64_t  end_time        = 0;
    uint64_t ticks           = 0;
    double   open            = 0;
    double   high            = 0;
    double   low             = 0;
    double   close           = 0;
    double   avg_price       = 0;
    double   sum_price       = 0;
    double   sum_abs_delta   = 0;   // Σ |Δprice| between consecutive ticks
    double   avg_abs_delta   = 0;
    double   max_abs_delta   = 0;
    double   realized_vol    = 0;   // sqrt(Σ (Δprice)^2)
    double   range           = 0;   // high - low
    int64_t  min_delta_time  = 0;
    int64_t  max_delta_time  = 0;
};

class BucketStatsBuilder {
public:
    // intervalSeconds > 0. Buckets aligned to epoch.
    static std::vector<BucketStats> build(const std::vector<Tick>& v,
                                          int64_t intervalSeconds)
    {
        if (intervalSeconds <= 0) throw std::runtime_error("interval must be > 0");
        std::vector<BucketStats> out;
        if (v.empty()) return out;

        auto align = [](int64_t t, int64_t s) {
            int64_t r = t % s; if (r < 0) r += s; return t - r;
        };

        int64_t bucket = align(v.front().time, intervalSeconds);
        BucketStats b;
        auto reset = [&](const Tick& t, int64_t bk) {
            b = BucketStats{};
            b.start_time = bk;
            b.end_time   = bk + intervalSeconds;
            b.open = b.high = b.low = b.close = t.price;
            b.min_delta_time = std::numeric_limits<int64_t>::max();
            b.max_delta_time = std::numeric_limits<int64_t>::min();
        };
        reset(v.front(), bucket);

        bool   havePrev   = false;
        int64_t prevTime  = 0;
        double  prevPrice = 0;

        for (const auto& t : v) {
            int64_t bk = align(t.time, intervalSeconds);
            if (bk != bucket) {
                if (b.ticks) out.push_back(b);
                bucket = bk;
                reset(t, bk);
                // reset delta-chain across bucket boundary
                havePrev = false;
            }

            if (t.price > b.high) b.high = t.price;
            if (t.price < b.low)  b.low  = t.price;
            b.close = t.price;
            b.sum_price += t.price;
            ++b.ticks;

            if (havePrev) {
                double d  = std::fabs(t.price - prevPrice);
                int64_t dt = t.time - prevTime;
                b.sum_abs_delta += d;
                b.realized_vol  += d * d;
                if (d > b.max_abs_delta) b.max_abs_delta = d;
                if (dt < b.min_delta_time) b.min_delta_time = dt;
                if (dt > b.max_delta_time) b.max_delta_time = dt;
            }
            prevTime  = t.time;
            prevPrice = t.price;
            havePrev  = true;
        }
        if (b.ticks) out.push_back(b);

        for (auto& x : out) {
            x.avg_price     = x.sum_price / double(x.ticks);
            x.avg_abs_delta = x.ticks > 1 ? x.sum_abs_delta / double(x.ticks - 1) : 0.0;
            x.realized_vol  = std::sqrt(x.realized_vol);
            x.range         = x.high - x.low;
        }
        return out;
    }
};

} // namespace tp