#pragma once
#include "tp_format.h"
#include <vector>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace tp {

class Sequencer {
public:
    // index based: 0, N, 2N, 3N ...
    static std::vector<Tick> everyNthTick(const std::vector<Tick>& v, size_t n) {
        if (n == 0) throw std::runtime_error("N must be >= 1");
        std::vector<Tick> out;
        out.reserve((v.size() + n - 1) / n);
        for (size_t i = 0; i < v.size(); i += n) out.push_back(v[i]);
        return out;
    }

    // time based: at t0, t0+N, t0+2N, ... pick tick nearest to each sample time
    static std::vector<Tick> everyNSeconds(const std::vector<Tick>& v, int64_t n) {
        if (n <= 0) throw std::runtime_error("N seconds must be >= 1");
        std::vector<Tick> out;
        if (v.empty()) return out;

        int64_t t = v.front().time;
        int64_t tEnd = v.back().time;
        while (t <= tEnd) {
            const Tick* nearest = nullptr;
            auto it = std::lower_bound(v.begin(), v.end(), t,
                [](const Tick& x, int64_t s){ return x.time < s; });
            if (it == v.begin()) nearest = &v.front();
            else if (it == v.end()) nearest = &v.back();
            else {
                const Tick* a = &*(it - 1);
                const Tick* b = &*it;
                nearest = (t - a->time) <= (b->time - t) ? a : b;
            }
            out.push_back(*nearest);
            t += n;
        }
        return out;
    }
};

} // namespace tp