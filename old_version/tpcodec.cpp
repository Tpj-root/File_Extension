// tpcodec.cpp  —  Convert CSV (times,prices) <-> custom .tp (Time-Price) binary
// Build:  g++ -std=c++17 -O2 -o tpcodec tpcodec.cpp
// Usage:
//    ./tpcodec  input.csv  output.tp     # CSV -> TP
//    ./tpcodec  -r  file.tp              # dump header + first/last ticks

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <algorithm>

namespace tp {

// ----------------------------------------------------------------------------
//  File-format constants
// ----------------------------------------------------------------------------
constexpr char     TP_MAGIC[4]  = {'T','P','0','1'};   // "Time-Price v01"
constexpr uint32_t TP_VERSION   = 1;
constexpr uint32_t TP_FLAG_COMPRESSED = 1u << 0;

// ----------------------------------------------------------------------------
//  On-disk header (60 bytes, packed)
// ----------------------------------------------------------------------------
#pragma pack(push, 1)
struct TpHeader {
    char     magic[4];        //  "TP01"
    uint32_t version;         //  1
    uint32_t flags;           //  bit0 = compressed
    int64_t  start_time;      //  unix seconds of first tick
    int64_t  end_time;        //  unix seconds of last tick
    double   min_price;       //  informational
    double   max_price;
    uint64_t count;           //  number of ticks
    uint32_t price_scale;     //  10^decimals  (e.g. 100 for 2 dp)
    uint32_t reserved;        //  must be 0
};
#pragma pack(pop)

static_assert(sizeof(TpHeader) == 60, "TpHeader must be 60 bytes");

// ----------------------------------------------------------------------------
//  In-memory tick
// ----------------------------------------------------------------------------
struct Tick {
    int64_t time;   // unix seconds
    double  price;
};

// ----------------------------------------------------------------------------
//  Varint  +  ZigZag helpers  (LEB128 style)
// ----------------------------------------------------------------------------
inline void putVarint(std::ostream& os, uint64_t v) {
    while (v >= 0x80) { os.put(char((v & 0x7F) | 0x80)); v >>= 7; }
    os.put(char(v));
}
inline uint64_t getVarint(std::istream& is) {
    uint64_t v = 0;
    int shift  = 0;
    for (;;) {
        int c = is.get();
        if (c < 0) throw std::runtime_error("Unexpected EOF while reading varint");
        v |= uint64_t(c & 0x7F) << shift;
        if (!(c & 0x80)) break;
        shift += 7;
        if (shift > 63) throw std::runtime_error("Varint overflow");
    }
    return v;
}
inline uint64_t zigzagEncode(int64_t v) { return (uint64_t(v) << 1) ^ uint64_t(v >> 63); }
inline int64_t  zigzagDecode(uint64_t v) { return int64_t(v >> 1) ^ -int64_t(v & 1); }

// ----------------------------------------------------------------------------
//  CSV Reader
// ----------------------------------------------------------------------------
class CsvReader {
public:
    explicit CsvReader(std::string path) : path_(std::move(path)) {}

    std::vector<Tick> load() {
        std::ifstream in(path_);
        if (!in) throw std::runtime_error("Cannot open CSV: " + path_);

        std::vector<Tick> ticks;
        std::string line;
        bool firstLine = true;

        while (std::getline(in, line)) {
            if (line.empty()) continue;

            // Skip a textual header row (contains non-numeric non-comma chars)
            if (firstLine) {
                firstLine = false;
                if (!std::isdigit((unsigned char)line[0]) && line[0] != '-' && line[0] != '+')
                    continue;   // header row -> skip
            }
            parseLine(line, ticks);
        }
        return ticks;
    }

    int maxDecimals() const { return maxDecimals_; }

private:
    void parseLine(const std::string& line, std::vector<Tick>& out) {
        const auto comma = line.find(',');
        if (comma == std::string::npos) return;

        Tick t;
        t.time = std::stoll(line.substr(0, comma));

        std::string pstr = line.substr(comma + 1);
        while (!pstr.empty() && std::isspace((unsigned char)pstr.back())) pstr.pop_back();
        if (pstr.empty()) return;

        t.price = std::stod(pstr);

        // count decimals in the string form
        auto dot = pstr.find('.');
        if (dot != std::string::npos) {
            int d = 0;
            for (size_t i = dot + 1; i < pstr.size() && std::isdigit((unsigned char)pstr[i]); ++i) ++d;
            if (d > maxDecimals_) maxDecimals_ = d;
        }

        out.push_back(t);
    }

    std::string path_;
    int         maxDecimals_ = 0;
};

// ----------------------------------------------------------------------------
//  .tp Writer
// ----------------------------------------------------------------------------
class TpWriter {
public:
    explicit TpWriter(std::string path) : path_(std::move(path)) {}

    void write(const std::vector<Tick>& ticks, int decimals) {
        if (ticks.empty()) throw std::runtime_error("No ticks to write");

        if (decimals < 0)  decimals = 0;
        if (decimals > 9)  decimals = 9;   // scale must fit in uint32
        const uint32_t scale = (uint32_t)std::pow(10u, decimals);

        // -------- build header --------
        TpHeader h{};
        std::memcpy(h.magic, TP_MAGIC, 4);
        h.version     = TP_VERSION;
        h.flags       = TP_FLAG_COMPRESSED;
        h.start_time  = ticks.front().time;
        h.end_time    = ticks.back().time;
        h.count       = ticks.size();
        h.price_scale = scale;
        h.reserved    = 0;

        double mn = ticks.front().price, mx = ticks.front().price;
        for (const auto& t : ticks) {
            if (t.price < mn) mn = t.price;
            if (t.price > mx) mx = t.price;
        }
        h.min_price = mn;
        h.max_price = mx;

        // -------- write file --------
        std::ofstream out(path_, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create " + path_);

        out.write(reinterpret_cast<const char*>(&h), sizeof(h));

        // First tick stored raw (absolutely) so decoding can start anywhere.
        int64_t prevT = ticks.front().time;
        int64_t prevP = (int64_t)std::llround(ticks.front().price * (double)scale);
        out.write(reinterpret_cast<const char*>(&prevT), sizeof(prevT));
        out.write(reinterpret_cast<const char*>(&prevP), sizeof(prevP));

        // Deltas as zigzag varints
        for (size_t i = 1; i < ticks.size(); ++i) {
            const int64_t curT = ticks[i].time;
            const int64_t curP = (int64_t)std::llround(ticks[i].price * (double)scale);

            putVarint(out, zigzagEncode(curT - prevT));
            putVarint(out, zigzagEncode(curP - prevP));

            prevT = curT;
            prevP = curP;
        }
    }

private:
    std::string path_;
};

// ----------------------------------------------------------------------------
//  .tp Reader  (supports streaming via next(), or bulk via readAll())
// ----------------------------------------------------------------------------
class TpReader {
public:
    explicit TpReader(std::string path) : path_(std::move(path)) {}

    void open() {
        in_.open(path_, std::ios::binary);
        if (!in_) throw std::runtime_error("Cannot open " + path_);

        in_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (in_.gcount() != (std::streamsize)sizeof(header_))
            throw std::runtime_error("Truncated .tp header: " + path_);

        if (std::memcmp(header_.magic, TP_MAGIC, 4) != 0)
            throw std::runtime_error("Bad magic - not a .tp file");
        if (header_.version != TP_VERSION)
            throw std::runtime_error("Unsupported .tp version");
    }

    const TpHeader& header() const { return header_; }

    // Returns false when exhausted.
    bool next(Tick& outTick) {
        if (!primed_) {
            if (header_.count == 0) return false;
            in_.read(reinterpret_cast<char*>(&curT_), sizeof(curT_));
            in_.read(reinterpret_cast<char*>(&curP_), sizeof(curP_));
            if (in_.gcount() != (std::streamsize)sizeof(curP_))
                throw std::runtime_error("Truncated .tp body");
            idx_    = 1;
            primed_ = true;
            outTick.time  = curT_;
            outTick.price = double(curP_) / double(header_.price_scale);
            return true;
        }
        if (idx_ >= header_.count) return false;

        const int64_t dT = zigzagDecode(getVarint(in_));
        const int64_t dP = zigzagDecode(getVarint(in_));
        curT_ += dT;
        curP_ += dP;
        ++idx_;

        outTick.time  = curT_;
        outTick.price = double(curP_) / double(header_.price_scale);
        return true;
    }

    std::vector<Tick> readAll() {
        std::vector<Tick> v;
        v.reserve((size_t)header_.count);
        Tick t;
        while (next(t)) v.push_back(t);
        return v;
    }

private:
    std::string path_;
    std::ifstream in_;
    TpHeader header_{};
    int64_t  curT_  = 0;
    int64_t  curP_  = 0;
    uint64_t idx_   = 0;
    bool     primed_ = false;
};

} // namespace tp

// ----------------------------------------------------------------------------
//  Demo main
// ----------------------------------------------------------------------------
static uintmax_t fileSize(const std::string& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    return f ? (uintmax_t)f.tellg() : 0;
}

int main(int argc, char** argv) {
    using namespace tp;
    try {
        if (argc < 3) {
            std::cerr << "Usage:\n"
                      << "  " << argv[0] << " <input.csv> <output.tp>   # convert\n"
                      << "  " << argv[0] << " -r <file.tp>              # read & print\n";
            return 1;
        }

        // ---------------- READ MODE ----------------
        if (std::string(argv[1]) == "-r") {
            TpReader r(argv[2]);
            r.open();

            const auto& h = r.header();
            std::cout << "File     : " << argv[2]              << "\n"
                      << "Version  : " << h.version             << "\n"
                      << "Flags    : " << h.flags               << "\n"
                      << "Count    : " << h.count               << "\n"
                      << "Start    : " << h.start_time          << "\n"
                      << "End      : " << h.end_time            << "\n"
                      << "MinPrice : " << h.min_price           << "\n"
                      << "MaxPrice : " << h.max_price           << "\n"
                      << "Scale    : " << h.price_scale         << "\n";

            auto all = r.readAll();
            const size_t showN = 5;
            std::cout << "\nFirst " << showN << " ticks:\n";
            for (size_t i = 0; i < std::min(showN, all.size()); ++i)
                std::cout << all[i].time << "," << all[i].price << "\n";

            std::cout << "...\nLast " << showN << " ticks:\n";
            const size_t start = all.size() > showN ? all.size() - showN : 0;
            for (size_t i = start; i < all.size(); ++i)
                std::cout << all[i].time << "," << all[i].price << "\n";
            return 0;
        }

        // ---------------- CONVERT MODE ----------------
        CsvReader reader(argv[1]);
        auto ticks = reader.load();
        if (ticks.empty()) throw std::runtime_error("CSV produced 0 ticks");

        TpWriter writer(argv[2]);
        writer.write(ticks, reader.maxDecimals());

        const uintmax_t csvSz = fileSize(argv[1]);
        const uintmax_t tpSz  = fileSize(argv[2]);
        double ratio = csvSz ? (100.0 * double(tpSz) / double(csvSz)) : 0.0;

        std::cout << "Converted " << ticks.size() << " ticks\n"
                  << "  CSV : " << csvSz << " bytes\n"
                  << "  TP  : " << tpSz  << " bytes  (" << ratio << "% of CSV)\n"
                  << "  decimals detected : " << reader.maxDecimals() << "\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}