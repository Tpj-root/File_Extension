#pragma once
#include "tp_format.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

namespace tp {

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
            if (firstLine) {
                firstLine = false;
                if (!std::isdigit((unsigned char)line[0]) && line[0] != '-' && line[0] != '+')
                    continue;  // textual header row
            }
            parseLine(line, ticks);
        }
        return ticks;
    }

    int maxDecimals() const { return maxDecimals_; }

private:
    void parseLine(const std::string& line, std::vector<Tick>& out) {
        auto comma = line.find(',');
        if (comma == std::string::npos) return;

        Tick t;
        t.time = std::stoll(line.substr(0, comma));

        std::string pstr = line.substr(comma + 1);
        while (!pstr.empty() && std::isspace((unsigned char)pstr.back())) pstr.pop_back();
        if (pstr.empty()) return;
        t.price = std::stod(pstr);

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

} // namespace tp