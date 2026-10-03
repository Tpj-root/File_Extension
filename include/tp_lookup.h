#pragma once
#include "tp_format.h"
#include <vector>
#include <algorithm>
#include <cstdint>

namespace tp {

class PriceLookup {
public:
    // exact match on time; nullptr if not present
    static const Tick* exact(const std::vector<Tick>& v, int64_t time) {
        auto it = std::lower_bound(v.begin(), v.end(), time,
            [](const Tick& t, int64_t s){ return t.time < s; });
        if (it != v.end() && it->time == time) return &*it;
        return nullptr;
    }

    // nearest tick (by |time diff|)
    static const Tick* nearest(const std::vector<Tick>& v, int64_t time) {
        if (v.empty()) return nullptr;
        auto it = std::lower_bound(v.begin(), v.end(), time,
            [](const Tick& t, int64_t s){ return t.time < s; });
        if (it == v.begin()) return &v.front();
        if (it == v.end())   return &v.back();
        const Tick* a = &*(it - 1);
        const Tick* b = &*it;
        return (time - a->time) <= (b->time - time) ? a : b;
    }

    // last tick with time <= given (as-of)
    static const Tick* atOrBefore(const std::vector<Tick>& v, int64_t time) {
        if (v.empty()) return nullptr;
        auto it = std::upper_bound(v.begin(), v.end(), time,
            [](int64_t s, const Tick& t){ return s < t.time; });
        if (it == v.begin()) return nullptr;
        return &*(it - 1);
    }

    // [t1, t2] inclusive
    static std::vector<Tick> range(const std::vector<Tick>& v, int64_t t1, int64_t t2) {
        std::vector<Tick> out;
        auto lo = std::lower_bound(v.begin(), v.end(), t1,
            [](const Tick& t, int64_t s){ return t.time < s; });
        for (auto it = lo; it != v.end() && it->time <= t2; ++it) out.push_back(*it);
        return out;
    }
};

} // namespace tp