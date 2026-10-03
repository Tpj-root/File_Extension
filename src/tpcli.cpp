#include "tp_dataset.h"
#include "tp_math.h"
#include "tp_candle.h"
#include "tp_lookup.h"
#include "tp_freq.h"
#include "tp_sequence.h"
// v2
#include "csv_exporter.h"
#include "tsv_exporter.h"
#include "json_exporter.h"
#include "columnar_exporter.h"
#include "arrow_parquet_exporter.h"
#include "head_tail.h"
#include "bucket_stats.h"
#include "merge.h"
#include "mmap_reader.h"
#include "stream_reader.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <chrono>

using namespace tp;

// ---------------------------------------------------------------------------
static const char* VERSION = "tpcli 1.0.0 (TP Time-Price codec)";

static void printHelp(const char* prog) {
    std::cout <<
"TPCLI (Time-Price) " << VERSION << "\n"
"Convert, inspect and analyse .tp (Time-Price) binary tick files.\n"
"\n"
"Usage: " << prog << " [options] ...\n"
"\n"
"Convert:\n"
"  -C <in.csv>  <out.tp>       Convert CSV  -> .tp\n"
"  -D <in.tp>   <out.csv>      Convert .tp  -> CSV\n"
"\n"
"Inspect:\n"
"  -r <file.tp>                Show header + first/last ticks\n"
"\n"
"Math (on prices):\n"
"  --math <func> <file.tp>     func = min|max|sum|avg|count|mult|all\n"
"\n"
"Lookup:\n"
"  --lookup <file.tp> <t>      Nearest price to time t\n"
"  --asof   <file.tp> <t>      Price at or before time t\n"
"  --exact  <file.tp> <t>      Exact tick at time t\n"
"  --range  <file.tp> <t1> <t2> Ticks in [t1,t2]\n"
"\n"
"Candles (OHLC):\n"
"  --candle <file.tp> <sec>    Bucket ticks into OHLC candles of <sec> seconds\n"
"\n"
"Frequency:\n"
"  --freq   <file.tp> [--top N] [--byprice]  Most common prices\n"
"\n"
"Sequence sampling:\n"
"  --seq    <file.tp> <N>      Every Nth tick (0, N, 2N, ...)\n"
"  --seqsec <file.tp> <N>      Every N seconds\n"
"\n"
"Export:\n"
"  --export <fmt> <in.tp> <out>   fmt = csv|tsv|json|col|parquet\n"
"  --head  <in.tp> <N>            First N ticks\n"
"  --tail  <in.tp> <N>            Last N ticks\n"
"\n"
"Bucket stats:\n"
"  --bucket <in.tp> <sec>         Volume + |Δ| + realized vol per bucket\n"
"\n"
"Merge:\n"
"  --merge <out.tp> <in1.tp> [in2.tp ...]\n"
"\n"
"Streaming (stdin):\n"
"  --stream                       Read .tp from stdin\n"
"Misc:\n"
"  --help, --version\n";
}

// ---------------------------------------------------------------------------
static void printHeader(const TpHeader& h) {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Header:\n"
              << "  magic        : " << std::string(h.magic, 4) << "\n"
              << "  version      : " << h.version        << "\n"
              << "  flags        : " << h.flags          << (h.flags & TP_FLAG_COMPRESSED ? " (compressed)" : "") << "\n"
              << "  start_time   : " << h.start_time     << "\n"
              << "  end_time     : " << h.end_time       << "\n"
              << "  count        : " << h.count          << "\n"
              << "  min_price    : " << h.min_price      << "\n"
              << "  max_price    : " << h.max_price      << "\n"
              << "  price_scale  : " << h.price_scale    << "\n";
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    try {
        if (argc < 2) { printHelp(argv[0]); return 1; }

        std::string cmd = argv[1];
        if (cmd == "--help" || cmd == "-h") { printHelp(argv[0]); return 0; }
        if (cmd == "--version") { std::cout << VERSION << "\n"; return 0; }

        // ---------------- Convert CSV -> TP ----------------
        if (cmd == "-C" && argc == 4) {
            auto t0 = std::chrono::steady_clock::now();
            TpDataset d = TpDataset::fromCsv(argv[2]);
            d.toTp(argv[3]);
            auto t1 = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            std::cout << "Wrote " << d.size() << " ticks -> " << argv[3]
                      << "  (" << ms << " ms)\n";
            return 0;
        }

        // ---------------- Convert TP -> CSV ----------------
        if (cmd == "-D" && argc == 4) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            d.toCsv(argv[3]);
            std::cout << "Wrote " << d.size() << " ticks -> " << argv[3] << "\n";
            return 0;
        }

        // ---------------- Inspect ----------------
        if (cmd == "-r" && argc == 3) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            printHeader(d.header());
            const auto& v = d.ticks();
            const size_t N = 5;
            std::cout << "\nFirst " << N << " ticks (time, price):\n";
            for (size_t i = 0; i < std::min(N, v.size()); ++i)
                std::cout << "  " << v[i].time << ", " << v[i].price << "\n";
            std::cout << "...\nLast " << N << " ticks:\n";
            size_t s = v.size() > N ? v.size() - N : 0;
            for (size_t i = s; i < v.size(); ++i)
                std::cout << "  " << v[i].time << ", " << v[i].price << "\n";
            return 0;
        }

        // ---------------- Math ----------------
        if (cmd == "--math" && argc == 4) {
            std::string func = argv[2];
            TpDataset d = TpDataset::fromTp(argv[3]);
            MathResult r = MathStats::compute(d.ticks());

            std::cout << std::fixed << std::setprecision(6);
            auto all = [&]() {
                std::cout << "count        : " << r.count          << "\n"
                          << "min_price    : " << r.min_price      << "\n"
                          << "max_price    : " << r.max_price      << "\n"
                          << "sum          : " << r.sum            << "\n"
                          << "average      : " << r.average        << "\n"
                          << "product      : " << (r.productOverflow ? "overflow" : std::to_string((double)r.product)) << "\n"
                          << "geometricMean: " << r.geometricMean  << "\n"
                          << "sum*count    : " << r.sumTimesCount  << "\n"
                          << "min_time     : " << r.min_time       << "\n"
                          << "max_time     : " << r.max_time       << "\n";
            };

            if (func == "min")        std::cout << r.min_price     << "\n";
            else if (func == "max")   std::cout << r.max_price     << "\n";
            else if (func == "sum")   std::cout << r.sum           << "\n";
            else if (func == "avg" || func == "average") std::cout << r.average << "\n";
            else if (func == "count") std::cout << r.count         << "\n";
            else if (func == "mult" || func == "product")
                std::cout << (r.productOverflow ? "overflow" : std::to_string((double)r.product)) << "\n";
            else if (func == "all")   all();
            else { std::cerr << "Unknown math func: " << func << "\n"; return 1; }
            return 0;
        }

        // ---------------- Lookup ----------------
        if ((cmd == "--lookup" || cmd == "--asof" || cmd == "--exact") && argc == 4) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            int64_t t = std::stoll(argv[3]);
            const Tick* p = nullptr;
            if (cmd == "--lookup") p = PriceLookup::nearest  (d.ticks(), t);
            if (cmd == "--asof")   p = PriceLookup::atOrBefore(d.ticks(), t);
            if (cmd == "--exact")  p = PriceLookup::exact    (d.ticks(), t);
            if (!p) { std::cout << "not found\n"; return 3; }
            std::cout << std::fixed << std::setprecision(6)
                      << p->time << "," << p->price << "\n";
            return 0;
        }

        if (cmd == "--range" && argc == 5) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            int64_t t1 = std::stoll(argv[3]);
            int64_t t2 = std::stoll(argv[4]);
            auto v = PriceLookup::range(d.ticks(), t1, t2);
            std::cout << "times,prices\n";
            std::cout << std::fixed << std::setprecision(6);
            for (auto& x : v) std::cout << x.time << "," << x.price << "\n";
            std::cerr << "# " << v.size() << " ticks\n";
            return 0;
        }

        // ---------------- Candles ----------------
        if (cmd == "--candle" && argc == 4) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            int64_t iv = std::stoll(argv[3]);
            auto cs = CandleBuilder::build(d.ticks(), iv);
            std::cout << "start_time,end_time,open,high,low,close,volume\n";
            std::cout << std::fixed << std::setprecision(6);
            for (auto& c : cs)
                std::cout << c.start_time << ',' << c.end_time << ','
                          << c.open << ',' << c.high << ',' << c.low << ',' << c.close << ','
                          << c.volume << "\n";
            std::cerr << "# " << cs.size() << " candles @ " << iv << "s\n";
            return 0;
        }

        // ---------------- Frequency ----------------
        if (cmd == "--freq" && argc >= 3) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            size_t topN = 20;
            bool   byPrice = false;
            for (int i = 3; i < argc; ++i) {
                if (!std::strcmp(argv[i], "--top") && i + 1 < argc) topN = (size_t)std::stoul(argv[++i]);
                else if (!std::strcmp(argv[i], "--byprice")) byPrice = true;
            }
            auto f = FrequencyAnalyzer::compute(d.ticks(), d.header().price_scale);
            auto s = byPrice ? FrequencyAnalyzer::sortedByPrice(std::move(f))
                             : FrequencyAnalyzer::sortedByCountDesc(std::move(f));
            std::cout << "price,count\n";
            std::cout << std::fixed << std::setprecision(6);
            const size_t lim = std::min(topN, s.size());
            for (size_t i = 0; i < lim; ++i)
                std::cout << s[i].price << ',' << s[i].count << "\n";
            std::cerr << "# unique prices: " << s.size() << "\n";
            return 0;
        }

        // ---------------- Sequence ----------------
        if (cmd == "--seq" && argc == 4) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            size_t n = (size_t)std::stoul(argv[3]);
            auto v = Sequencer::everyNthTick(d.ticks(), n);
            std::cout << "times,prices\n";
            std::cout << std::fixed << std::setprecision(6);
            for (auto& x : v) std::cout << x.time << ',' << x.price << "\n";
            std::cerr << "# " << v.size() << " sampled ticks (every " << n << ")\n";
            return 0;
        }
        if (cmd == "--seqsec" && argc == 4) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            int64_t n = std::stoll(argv[3]);
            auto v = Sequencer::everyNSeconds(d.ticks(), n);
            std::cout << "times,prices\n";
            std::cout << std::fixed << std::setprecision(6);
            for (auto& x : v) std::cout << x.time << ',' << x.price << "\n";
            std::cerr << "# " << v.size() << " sampled ticks (every " << n << "s)\n";
            return 0;
        }
        // ---------------- Export: CSV / TSV / JSON / COL / PARQUET ----------------
        if (cmd == "--export" && argc >= 5) {
            std::string fmt = argv[2];      // csv|tsv|json|col|parquet
            std::string in  = argv[3];
            std::string out = argv[4];

            // mmap loader for speed (falls back to normal reader if mmap fails)
            std::vector<Tick> ticks;
            try { MmapTpReader r(in); r.open(); ticks = r.readAll(); }
            catch (...) { TpDataset d = TpDataset::fromTp(in); ticks = d.ticks(); }

            if      (fmt == "csv")     CsvExporter(out).write(ticks);
            else if (fmt == "tsv")     TsvExporter(out).write(ticks);
            else if (fmt == "json")    JsonExporter(out, {6, true, false, true}).write(ticks);
            else if (fmt == "col")     ColumnarExporter(out).write(ticks);
            else if (fmt == "parquet") ArrowParquetExporter(out).write(ticks);
            else { std::cerr << "Unknown export format: " << fmt << "\n"; return 1; }

            std::cout << "Wrote " << ticks.size() << " ticks -> " << out
                      << "  (" << fmt << ")\n";
            return 0;
        }

        // ---------------- Head / Tail ----------------
        if (cmd == "--head" && argc == 4) {
            auto v = HeadTail::head(argv[2], (size_t)std::stoul(argv[3]));
            std::cout << "times,prices\n" << std::fixed << std::setprecision(6);
            for (auto& t : v) std::cout << t.time << "," << t.price << "\n";
            return 0;
        }
        if (cmd == "--tail" && argc == 4) {
            auto v = HeadTail::tail(argv[2], (size_t)std::stoul(argv[3]));
            std::cout << "times,prices\n" << std::fixed << std::setprecision(6);
            for (auto& t : v) std::cout << t.time << "," << t.price << "\n";
            return 0;
        }

        // ---------------- Bucket stats ----------------
        if (cmd == "--bucket" && argc == 4) {
            TpDataset d = TpDataset::fromTp(argv[2]);
            int64_t iv = std::stoll(argv[3]);
            auto bs = BucketStatsBuilder::build(d.ticks(), iv);
            std::cout << "start,end,ticks,open,high,low,close,avg,range,"
                         "sum_abs_delta,avg_abs_delta,max_abs_delta,"
                         "realized_vol,min_dt,max_dt\n";
            std::cout << std::fixed << std::setprecision(6);
            for (auto& b : bs)
                std::cout << b.start_time << ',' << b.end_time << ','
                          << b.ticks      << ',' << b.open      << ',' << b.high << ','
                          << b.low        << ',' << b.close     << ',' << b.avg_price << ','
                          << b.range      << ',' << b.sum_abs_delta << ',' << b.avg_abs_delta << ','
                          << b.max_abs_delta << ',' << b.realized_vol << ','
                          << b.min_delta_time << ',' << b.max_delta_time << "\n";
            std::cerr << "# " << bs.size() << " buckets @ " << iv << "s\n";
            return 0;
        }

        // ---------------- Merge ----------------
        if (cmd == "--merge" && argc >= 3) {
            // syntax:  --merge out.tp in1.tp in2.tp [in3.tp ...]
            std::string out = argv[2];
            std::vector<std::string> ins;
            for (int i = 3; i < argc; ++i) ins.emplace_back(argv[i]);
            TpMerger::mergeToFile(ins, out, {true, false, 6});
            std::cout << "Merged " << ins.size() << " files -> " << out << "\n";
            return 0;
        }

        // ---------------- Stream from stdin ----------------
        if (cmd == "--stream") {
            StreamTpReader r(std::cin);
            r.start();
            std::cout << "times,prices\n" << std::fixed << std::setprecision(6);
            Tick t;
            while (r.next(t)) std::cout << t.time << "," << t.price << "\n";
            return 0;
        }



        std::cerr << "Unknown or malformed command. Try --help.\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}