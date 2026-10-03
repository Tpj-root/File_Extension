#pragma once
#include "tp_format.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tp {

struct FreqEntry {
    double   price;
    uint64_t count;
};

class FrequencyAnalyzer {
public:
    // Group by scaled integer (avoids floating point key issues)
    static std::vector<FreqEntry> compute(const std::vector<Tick>& ticks, uint32_t scale) {
        std::unordered_map<int64_t, uint64_t> hist;
        hist.reserve(ticks.size());
        for (const auto& t : ticks) {
            int64_t k = (int64_t)std::llround(t.price * (double)scale);
            ++hist[k];
        }
        std::vector<FreqEntry> out;
        out.reserve(hist.size());
        for (auto& kv : hist)
            out.push_back({ double(kv.first) / double(scale), kv.second });
        return out;
    }

    // sort by count descending, then price ascending
    static std::vector<FreqEntry> sortedByCountDesc(std::vector<FreqEntry> v) {
        std::sort(v.begin(), v.end(), [](const FreqEntry& a, const FreqEntry& b){
            if (a.count != b.count) return a.count > b.count;
            return a.price < b.price;
        });
        return v;
    }

    // sort by price ascending
    static std::vector<FreqEntry> sortedByPrice(std::vector<FreqEntry> v) {
        std::sort(v.begin(), v.end(), [](const FreqEntry& a, const FreqEntry& b){
            return a.price < b.price;
        });
        return v;
    }
};

} // namespace tp