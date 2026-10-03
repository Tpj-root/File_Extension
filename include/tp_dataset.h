#pragma once
#include "tp_format.h"
#include "csv_reader.h"
#include "csv_writer.h"
#include "tp_reader.h"
#include "tp_writer.h"
#include <string>
#include <vector>

namespace tp {

// Façade: loads either CSV or .tp into RAM, lets you save to either form.
class TpDataset {
public:
    static TpDataset fromCsv(const std::string& path) {
        CsvReader r(path);
        TpDataset d;
        d.ticks_ = r.load();
        d.decimals_ = r.maxDecimals();
        d.rebuildHeader();
        return d;
    }

    static TpDataset fromTp(const std::string& path) {
        TpReader r(path);
        r.open();
        TpDataset d;
        d.header_ = r.header();
        d.decimals_ = decimalsFromScale(d.header_.price_scale);
        d.ticks_ = r.readAll();
        return d;
    }

    void toCsv(const std::string& path) const {
        CsvWriter(path, decimals_).write(ticks_);
    }
    void toTp(const std::string& path) const {
        TpWriter(path).write(ticks_, decimals_);
    }

    const TpHeader&            header() const { return header_; }
    const std::vector<Tick>&   ticks()  const { return ticks_; }
    size_t                     size()   const { return ticks_.size(); }
    bool                       empty()  const { return ticks_.empty(); }
    int                        decimals() const { return decimals_; }

private:
    static int decimalsFromScale(uint32_t s) {
        int d = 0; while (s > 1 && s % 10 == 0) { s /= 10; ++d; }
        return d;
    }

    void rebuildHeader() {
        header_ = TpHeader{};
        std::memcpy(header_.magic, TP_MAGIC, 4);
        header_.version = TP_VERSION;
        header_.flags   = TP_FLAG_COMPRESSED;
        header_.count   = ticks_.size();
        header_.price_scale = (uint32_t)std::pow(10u, decimals_);
        if (!ticks_.empty()) {
            header_.start_time = ticks_.front().time;
            header_.end_time   = ticks_.back().time;
            double mn = ticks_.front().price, mx = mn;
            for (const auto& t : ticks_) { mn = std::min(mn, t.price); mx = std::max(mx, t.price); }
            header_.min_price = mn;
            header_.max_price = mx;
        }
    }

    TpHeader          header_{};
    std::vector<Tick> ticks_;
    int               decimals_ = 2;
};

} // namespace tp