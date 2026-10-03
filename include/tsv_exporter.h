#pragma once
#include "csv_exporter.h"

namespace tp {

class TsvExporter : public CsvExporter {
public:
    explicit TsvExporter(std::string path, Options opt = {})
        : CsvExporter(std::move(path), fix(opt)) {}

private:
    static Options fix(Options o) { o.sep = '\t'; return o; }
};

} // namespace tp