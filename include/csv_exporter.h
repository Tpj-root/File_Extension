#pragma once
#include "tp_format.h"
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <stdexcept>

namespace tp {

class CsvExporter {
public:
	struct Options {
	    int  decimals = 6;
	    bool header   = true;
	    char sep      = ',';
	};

	explicit CsvExporter(std::string path)
	    : path_(std::move(path)), opt_{} {}

	explicit CsvExporter(std::string path, Options opt)
	    : path_(std::move(path)), opt_(opt) {}

    void write(const std::vector<Tick>& v) const { write(v.begin(), v.end()); }

    // Streaming overload — pipe a HeadTail/Sequencer/Reader directly
    template <class It>
    void write(It begin, It end) const {
        std::ofstream out(path_);
        if (!out) throw std::runtime_error("Cannot create " + path_);
        out << std::fixed << std::setprecision(opt_.decimals);
        if (opt_.header) out << "times" << opt_.sep << "prices\n";
        for (auto it = begin; it != end; ++it)
            out << it->time << opt_.sep << it->price << '\n';
    }

protected:
    std::string path_;
    Options     opt_;
};

} // namespace tp