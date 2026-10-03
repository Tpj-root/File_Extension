#pragma once
#include <cstdint>
#include <cstring>

namespace tp {

constexpr char     TP_MAGIC[4]        = {'T','P','0','1'};
constexpr uint32_t TP_VERSION         = 1;
constexpr uint32_t TP_FLAG_COMPRESSED = 1u << 0;

#pragma pack(push,1)
struct TpHeader {
    char     magic[4];      // "TP01"
    uint32_t version;       // 1
    uint32_t flags;         // bit0 = compressed
    int64_t  start_time;    // unix seconds of first tick
    int64_t  end_time;      // unix seconds of last tick
    double   min_price;
    double   max_price;
    uint64_t count;         // number of ticks
    uint32_t price_scale;   // 10^decimals
    uint32_t reserved;      // must be 0
};
#pragma pack(pop)

static_assert(sizeof(TpHeader) == 60, "TpHeader must be 60 bytes");

struct Tick {
    int64_t time;    // unix seconds
    double  price;
};

} // namespace tp