#pragma once
#include "tp_format.h"
#include <vector>
#include <cmath>
#include <limits>
#include <cstdint>

namespace tp {

struct MathResult {
    uint64_t    count     = 0;
    int64_t     min_time  = 0;
    int64_t     max_time  = 0;
    double      min_price = 0;
    double      max_price = 0;
    double      sum       = 0;
    double      average   = 0;
    long double product   = 1.0L;    // can overflow for large N
    bool        productOverflow = false;
    double      geometricMean   = 0; // exp(mean(log(p)))
    double      sumTimesCount   = 0; // sum * count (cheap helper)
};

class MathStats {
public:
    static MathResult compute(const std::vector<Tick>& ticks) {
        MathResult r;
        if (ticks.empty()) return r;

        r.count     = ticks.size();
        r.min_time  = ticks.front().time;
        r.max_time  = ticks.back().time;
        r.min_price = ticks.front().price;
        r.max_price = ticks.front().price;

        long double logSum = 0.0L;
        for (const auto& t : ticks) {
            r.sum += t.price;
            if (t.price < r.min_price) r.min_price = t.price;
            if (t.price > r.max_price) r.max_price = t.price;

            if (!r.productOverflow) {
                r.product *= (long double)t.price;
                if (!std::isfinite((double)r.product)) r.productOverflow = true;
            }
            if (t.price > 0) logSum += std::log((long double)t.price);
        }
        r.average        = r.sum / double(r.count);
        r.geometricMean  = (double)std::exp(logSum / (long double)r.count);
        r.sumTimesCount  = r.sum * (double)r.count;
        return r;
    }
};

} // namespace tp