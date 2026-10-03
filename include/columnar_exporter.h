#pragma once
#include "tp_format.h"
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace tp {

// ---------------------------------------------------------------------------
//  Simple self-describing columnar binary (.col).
//  Fast to write, fast to memmap, easy to parse. Not Parquet spec-compliant.
//
//  Layout:
//    magic     "COL1"                (4)
//    version   uint32                (4)
//    count     uint64                (8)
//    time_col  int64[count]          (8*N, LE)
//    price_col double[count]         (8*N, LE, IEEE754)
// ---------------------------------------------------------------------------
class ColumnarExporter {
public:
    static constexpr char COL_MAGIC[4] = {'C','O','L','1'};

    explicit ColumnarExporter(std::string path) : path_(std::move(path)) {}

    void write(const std::vector<Tick>& v) const {
        std::ofstream out(path_, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create " + path_);

        const uint32_t ver   = 1;
        const uint64_t count = v.size();

        out.write(COL_MAGIC, 4);
        out.write(reinterpret_cast<const char*>(&ver),   4);
        out.write(reinterpret_cast<const char*>(&count), 8);

        for (auto& t : v) out.write(reinterpret_cast<const char*>(&t.time),  8);
        for (auto& t : v) out.write(reinterpret_cast<const char*>(&t.price), 8);
    }

private:
    std::string path_;
};

} // namespace tp