#pragma once
#include "tp_format.h"
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <stdexcept>

namespace tp {

class JsonExporter {
public:
    struct Options {
        int  decimals       = 6;
        bool pretty         = false;
        bool asArrayOfPairs = false;
        bool wrapObject     = false;
    };

    explicit JsonExporter(std::string path)
        : path_(std::move(path)), opt_{} {}

    explicit JsonExporter(std::string path, Options opt)
        : path_(std::move(path)), opt_(opt) {}

    void write(const std::vector<Tick>& v) const {
        std::ofstream out(path_);
        if (!out) throw std::runtime_error("Cannot create " + path_);
        out << std::fixed << std::setprecision(opt_.decimals);

        if (opt_.wrapObject) {
            out << "{";
            if (opt_.pretty) out << "\n  ";
            out << "\"count\":" << v.size();
            if (!v.empty()) {
                if (opt_.pretty) out << ",\n  ";
                out << "\"start_time\":" << v.front().time
                    << ",\"end_time\":"   << v.back().time;
            }
            if (opt_.pretty) out << ",\n  ";
            out << "\"ticks\":";
        }
        out << "[";
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) out << ',';
            if (opt_.pretty) out << (opt_.asArrayOfPairs ? "\n    " : "\n  ");
            if (opt_.asArrayOfPairs)
                out << "[" << v[i].time << "," << v[i].price << "]";
            else
                out << "{\"time\":" << v[i].time
                    << ",\"price\":" << v[i].price << "}";
        }
        if (opt_.pretty && !v.empty()) out << "\n";
        out << "]";
        if (opt_.wrapObject) {
            if (opt_.pretty) out << "\n";
            out << "}";
        }
        out << "\n";
    }

private:
    std::string path_;
    Options     opt_;
};

} // namespace tp