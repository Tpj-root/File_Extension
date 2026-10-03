#pragma once
// ---------------------------------------------------------------------------
//  Real Parquet via Apache Arrow. Compile with:
//      -DTP_HAVE_ARROW
//      -I/path/to/arrow/include
//      -larrow -lparquet
//  If TP_HAVE_ARROW is not defined, this class is a no-op stub so the rest of
//  the project still compiles.
// ---------------------------------------------------------------------------
#include "tp_format.h"
#include <string>
#include <vector>

#ifdef TP_HAVE_ARROW
  #include <arrow/api.h>
  #include <arrow/io/file.h>
  #include <parquet/arrow/writer.h>
#endif

namespace tp {

class ArrowParquetExporter {
public:
    struct Options {
        int compression_level = 6;
        int row_group_size    = 65536;
    };

    explicit ArrowParquetExporter(std::string path)
        : path_(std::move(path)), opt_{} {}

    explicit ArrowParquetExporter(std::string path, Options opt)
        : path_(std::move(path)), opt_(opt) {}

    void write(const std::vector<Tick>& v) const;

private:
    std::string path_;
    Options     opt_;
};

#ifdef TP_HAVE_ARROW
// ---- definition only when Arrow is available ----
inline void ArrowParquetExporter::write(const std::vector<Tick>& v) const {
    arrow::Int64Builder tbuilder;
    arrow::DoubleBuilder pbuilder;
    (void)tbuilder.Reserve(v.size());
    (void)pbuilder.Reserve(v.size());
    for (auto& t : v) {
        (void)tbuilder.Append(t.time);
        (void)pbuilder.Append(t.price);
    }
    std::shared_ptr<arrow::Array> ta, pa;
    (void)tbuilder.Finish(&ta);
    (void)pbuilder.Finish(&pa);

    auto schema = arrow::schema({
        arrow::field("time",  arrow::int64()),
        arrow::field("price", arrow::float64()),
    });
    auto table = arrow::Table::Make(schema, {ta, pa});

    auto outfile = arrow::io::FileOutputStream::Open(path_).ValueOrDie();
    parquet::WriterProperties::Builder b;
    b.compression(parquet::Compression::SNAPPY);
    b.max_row_group_length(opt_.row_group_size);
    parquet::arrow::WriteTable(*table, arrow::default_memory_pool(),
                               outfile, opt_.row_group_size, b.build());
}
#else
inline void ArrowParquetExporter::write(const std::vector<Tick>&) const {
    throw std::runtime_error(
        "ArrowParquetExporter: rebuild with -DTP_HAVE_ARROW and link "
        "-larrow -lparquet to enable Parquet output.");
}
#endif

} // namespace tp