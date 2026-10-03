#pragma once
#include "tp_format.h"
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace tp {

class CsvWriter {
public:
    explicit CsvWriter(std::string path, int decimals = 2)
        : path_(std::move(path)), decimals_(decimals) {}

    void write(const std::vector<Tick>& ticks, bool withHeader = true) const {
        std::ofstream out(path_);
        if (!out) throw std::runtime_error("Cannot create " + path_);
        out << std::fixed << std::setprecision(decimals_);
        if (withHeader) out << "times,prices\n";
        for (const auto& t : ticks)
            out << t.time << ',' << t.price << '\n';
    }

private:
    std::string path_;
    int         decimals_;
};

} // namespace tp