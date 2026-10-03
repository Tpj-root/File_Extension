#pragma once
#include "tp_reader.h"
#include <vector>
#include <algorithm>
#include <cstddef>
#include <string>

namespace tp {

class HeadTail {
public:
    // First N ticks — O(N), tiny RAM, does NOT load the file.
    static std::vector<Tick> head(const std::string& path, size_t n) {
        TpReader r(path);
        r.open();
        std::vector<Tick> out;
        out.reserve(n);
        Tick t;
        while (out.size() < n && r.next(t)) out.push_back(t);
        return out;
    }

    // Last N ticks — O(total) time, O(N) memory using a ring buffer.
    static std::vector<Tick> tail(const std::string& path, size_t n) {
        TpReader r(path);
        r.open();
        std::vector<Tick> ring;
        if (n) ring.reserve(n);
        size_t idx = 0;
        Tick t;
        while (r.next(t)) {
            if (ring.size() < n) ring.push_back(t);
            else { ring[idx] = t; idx = (idx + 1) % n; }
        }
        if (ring.size() == n && idx != 0)
            std::rotate(ring.begin(), ring.begin() + idx, ring.end());
        return ring;
    }
};

} // namespace tp