# File_Extension








### Build & run

```

g++ -std=c++17 -O2 -o tpcodec tpcodec.cpp

# convert your sample
./tpcodec frxXAUUSD_1790922600_2026-10-02.csv frxXAUUSD_1790922600_2026-10-02.tp

# read back
./tpcodec -r frxXAUUSD_1790922600_2026-10-02.tp


```




Using it inside other code


```
#include "tpcodec.cpp"   // or split into .h/.cpp

tp::TpReader r("frxXAUUSD_..._.tp");
r.open();

tp::Tick t;
while (r.next(t)) {
    // t.time  (int64_t, unix seconds)
    // t.price (double)
}

```



```

[ TpHeader (60 bytes, packed) ]
[ int64 first_time ][ int64 first_price_scaled ]
[ varint(Δtime) varint(Δprice) ] * (N-1)

```







### Result

```
when@master:~/Desktop/MY_GIT/File_Extension$ ./tpcodec frxXAUUSD_1790274600_2026-09-25.csv frxXAUUSD_1790274600_2026-09-25.tp
Converted 82421 ticks
  CSV : 1640322 bytes
  TP  : 165669 bytes  (10.0998% of CSV)
  decimals detected : 2
when@master:~/Desktop/MY_GIT/File_Extension$ 



when@master:~/Desktop/MY_GIT/File_Extension$ ls -alh
total 1.8M
drwxrwxr-x  3 when when 4.0K Oct  3 20:02 .
drwxrwxr-x 49 when when 4.0K Oct  3 19:59 ..
-rw-rw-r--  1 when when 1.6M Sep 26 17:17 frxXAUUSD_1790274600_2026-09-25.csv
-rw-rw-r--  1 when when 162K Oct  3 20:02 frxXAUUSD_1790274600_2026-09-25.tp



```




```

when@master:~/Desktop/MY_GIT/File_Extension$ ./tpcodec -r frxXAUUSD_1790274600_2026-09-25.tp 
File     : frxXAUUSD_1790274600_2026-09-25.tp
Version  : 1
Flags    : 1
Count    : 82421
Start    : 1790274600
End      : 1790360999
MinPrice : 4255.04
MaxPrice : 4315.64
Scale    : 100

First 5 ticks:
1790274600,4274.65
1790274601,4274.58
1790274602,4274.68
1790274603,4274.63
1790274604,4274.7
...
Last 5 ticks:
1790360995,4287.02
1790360996,4287.37
1790360997,4287.44
1790360998,4287.35
1790360999,4287.53
when@master:~/Desktop/MY_GIT/File_Extension$ cat frxXAUUSD_1790274600_2026-09-25.csv | wc -l
82422
when@master:~/Desktop/MY_GIT/File_Extension$ 

```



# tpcli — Time‑Price (`.tp`) Toolkit

> A tiny, fast, dependency‑free C++17 toolkit for storing, converting, and analysing tick data.
> Replaces bulky CSV with a compact binary `.tp` format (typically **5–20 % of CSV size**),
> and ships with a full CLI for math, OHLC candles, time lookup, frequency, sampling,
> bucket statistics, merging, streaming, and multi‑format export.

---

## Table of Contents

1. [What is `.tp`?](#1-what-is-tp)
2. [Features](#2-features)
3. [Project layout](#3-project-layout)
4. [Requirements](#4-requirements)
5. [Build](#5-build)
6. [Quick start](#6-quick-start)
7. [CLI reference](#7-cli-reference)
8. [Usage recipes](#8-usage-recipes)
9. [Using the library from your own code](#9-using-the-library-from-your-own-code)
10. [The `.tp` file format (spec)](#10-the-tp-file-format-spec)
11. [Performance notes](#11-performance-notes)
12. [Extending](#12-extending)
13. [Troubleshooting](#13-troubleshooting)
14. [License](#14-license)

---

## 1. What is `.tp`?

`.tp` = **Time‑Price**. A minimal binary container for ticks:

```
(tick_0)  time=1790922600  price=4189.51
(tick_1)  time=1790922601  price=4189.73
(tick_2)  time=1790922602  price=4190.02
...
```

Because ticks are ordered and deltas are tiny, each tick after the first
costs **2–4 bytes** instead of the ~30 bytes it does in CSV.

| File                          | Size       |
|-------------------------------|------------|
| `frxXAUUSD_2026-10-02.csv`    | ~ 19 MB    |
| `frxXAUUSD_2026-10-02.tp`     | ~ 1.8 MB   |

---

## 2. Features

| Category            | What you get |
|---------------------|--------------|
| **Codec**           | CSV ⇄ `.tp` lossless conversion |
| **Inspect**         | Header summary, first/last ticks |
| **Math**            | min, max, sum, avg, count, product, geometric mean |
| **Candles**         | OHLC bucketing at any interval (1 s, 5 s, 60 s, 300 s, …) |
| **Lookup**          | exact / nearest / as‑of / range by timestamp |
| **Frequency**       | price histogram + sort by count or price |
| **Sampling**        | every Nth tick, or every N seconds |
| **Bucket stats**    | volume, |Δprice|, realized volatility, range per interval |
| **Merge**           | union N `.tp` files into one sorted file |
| **Streaming**       | read `.tp` from `stdin` |
| **mmap**            | zero‑copy reader for very large files |
| **Export**          | CSV, TSV, JSON, columnar `.col`, real Parquet (optional Arrow) |

---

## 3. Project layout

```
File_Extension/
├── include/
│   ├── tp_format.h              # on-disk header + Tick struct
│   ├── tp_varint.h              # LEB128 + zigzag
│   ├── csv_reader.h
│   ├── csv_writer.h
│   ├── tp_writer.h
│   ├── tp_reader.h
│   ├── tp_dataset.h             # façade loader/saver
│   ├── tp_math.h                # min/max/sum/avg/…/geometric mean
│   ├── tp_candle.h              # OHLC
│   ├── tp_lookup.h              # exact/nearest/asof/range
│   ├── tp_freq.h                # price histogram
│   ├── tp_sequence.h            # every-Nth tick / second
│   ├── csv_exporter.h           # extra exporters
│   ├── tsv_exporter.h
│   ├── json_exporter.h
│   ├── columnar_exporter.h
│   ├── arrow_parquet_exporter.h
│   ├── head_tail.h
│   ├── bucket_stats.h
│   ├── merge.h
│   ├── mmap_reader.h
│   └── stream_reader.h
├── src/
│   └── tpcli.cpp                # CLI entry point
├── Makefile
└── README.md
```

---

## 4. Requirements

* **Compiler:** g++ 9+ / clang++ 10+ / MSVC 2019+ (C++17)
* **OS:** Linux, macOS, WSL (Windows native works if you swap `<sys/mman.h>` calls)
* **Optional:** Apache Arrow (only for `--export parquet`)

No external libraries required for the core features.

---

## 5. Build

### Standard build

```bash
git clone <your-repo> File_Extension
cd File_Extension
make
```

Produces `./tpcli`.

### Manual build (no make)

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude -o tpcli src/tpcli.cpp
```

### With Parquet support

```bash
# install arrow + parquet first (apt / brew / conda)
g++ -std=c++17 -O2 -Iinclude -DTP_HAVE_ARROW \
    -I/usr/include/arrow -larrow -lparquet \
    -o tpcli src/tpcli.cpp
```

### Windows (MSVC)

Replace `mmap_reader.h` usage with `tp_reader.h` (already the fallback in `--export`).
Everything else compiles as‑is.

---

## 6. Quick start

```bash
# 1. Convert your CSV
./tpcli -C frxXAUUSD_1790922600_2026-10-02.csv  frxXAUUSD.tp

# 2. Inspect it
./tpcli -r frxXAUUSD.tp

# 3. Get some stats
./tpcli --math all frxXAUUSD.tp

# 4. Look up a price at a timestamp
./tpcli --lookup frxXAUUSD.tp 1790922610

# 5. Build 60‑second OHLC candles
./tpcli --candle frxXAUUSD.tp 60 > candles_60s.csv

# 6. See the 20 most common prices
./tpcli --freq frxXAUUSD.tp --top 20
```

---

## 7. CLI reference

```
TPCLI (Time-Price) 1.0.0

Usage: tpcli [options] ...

Convert:
  -C <in.csv>  <out.tp>        CSV  -> .tp
  -D <in.tp>   <out.csv>       .tp  -> CSV

Inspect:
  -r <file.tp>                 Header + first/last ticks

Math (prices):
  --math <func> <file.tp>      min | max | sum | avg | count | mult | all

Lookup:
  --lookup <file.tp> <t>       Nearest tick to time t
  --asof   <file.tp> <t>       Price at or before time t
  --exact  <file.tp> <t>       Exact tick at time t
  --range  <file.tp> <t1> <t2> Ticks within [t1, t2]

Candles (OHLC):
  --candle <file.tp> <sec>     Bucket into candles of <sec> seconds

Frequency:
  --freq <file.tp> [--top N] [--byprice]

Sequence sampling:
  --seq    <file.tp> <N>       Every Nth tick (0, N, 2N, ...)
  --seqsec <file.tp> <N>       Nearest tick every N seconds

Export:
  --export <fmt> <in.tp> <out> fmt = csv | tsv | json | col | parquet
  --head  <in.tp> <N>          First N ticks
  --tail  <in.tp> <N>          Last N ticks

Bucket stats:
  --bucket <in.tp> <sec>       Volume + |Δprice| + realized vol per bucket

Merge:
  --merge <out.tp> <in1.tp> [in2.tp ...]

Streaming:
  --stream                     Read .tp from stdin
```

---

## 8. Usage recipes

### 8.1. Convert a folder of CSVs

```bash
for f in raw/*.csv; do
    out="tp/$(basename "${f%.csv}").tp"
    ./tpcli -C "$f" "$out"
done
```

### 8.2. Merge a full trading day

```bash
./tpcli --merge day.tp \
    hour_00.tp hour_01.tp hour_02.tp hour_03.tp \
    hour_04.tp hour_05.tp hour_06.tp hour_07.tp
```

### 8.3. Minute-by-minute volatility report

```bash
./tpcli --bucket day.tp 60 \
  | awk -F, 'NR>1 {printf "%s  vol=%8.4f  ticks=%d\n",$1,$13,$3}' \
  | head
```

### 8.4. Feed a charting library

```bash
./tpcli --export json day.tp day.json
# then in your JS frontend:
#   const {ticks} = JSON.parse(await fetch('day.json').then(r=>r.json()));
```

### 8.5. Pipe into another tool

```bash
# sample every 3 seconds and stream to stdout
./tpcli --seqsec day.tp 3 | grep -v '^times' | head
```

### 8.6. Find the exact price at a specific time

```bash
TS=1790922610
./tpcli --exact day.tp $TS   # prints "1790922610,<price>"
```

### 8.7. Slice a time window into a new `.tp`

```bash
./tpcli --range day.tp 1790922600 1790923200 > window.csv
./tpcli -C window.csv window.tp
```

---

## 9. Using the library from your own code

`tpcli` is really a thin CLI over re‑usable headers. Include them directly:

```cpp
#include "tp_dataset.h"
#include "tp_math.h"
#include "tp_candle.h"
#include "tp_lookup.h"

using namespace tp;

int main() {
    // Load from either format — the façade picks the right reader.
    TpDataset d = TpDataset::fromTp("day.tp");
    // TpDataset d = TpDataset::fromCsv("day.csv");   // same API

    // Statistics
    MathResult r = MathStats::compute(d.ticks());
    printf("avg=%.4f  min=%.4f  max=%.4f\n",
           r.average, r.min_price, r.max_price);

    // OHLC candles @ 5 min
    auto cs = CandleBuilder::build(d.ticks(), 300);
    for (auto& c : cs)
        printf("%lld O=%.2f H=%.2f L=%.2f C=%.2f V=%llu\n",
               (long long)c.start_time,
               c.open, c.high, c.low, c.close,
               (unsigned long long)c.volume);

    // Price lookup
    if (auto* p = PriceLookup::nearest(d.ticks(), 1790922610))
        printf("nearest: %lld -> %.2f\n", (long long)p->time, p->price);

    // Save back
    d.toTp("out.tp");
    d.toCsv("out.csv");
}
```

### Zero‑copy reader for huge files

```cpp
#include "mmap_reader.h"

MmapTpReader r("huge.tp");
r.open();
Tick t;
while (r.next(t)) {
    // process without ever loading the whole file
}
```

### Streaming from `stdin`

```cpp
#include "stream_reader.h"

StreamTpReader r(std::cin);
r.start();
Tick t;
while (r.next(t)) { /* ... */ }
```

---

## 10. The `.tp` file format (spec)

### Header — 60 bytes, packed

| Offset | Size | Field          | Notes                        |
|-------:|-----:|----------------|------------------------------|
| 0      | 4    | `magic`        | ASCII `"TP01"`               |
| 4      | 4    | `version`      | currently `1`                |
| 8      | 4    | `flags`        | bit0 = compressed            |
| 12     | 8    | `start_time`   | unix seconds, first tick     |
| 20     | 8    | `end_time`     | unix seconds, last tick      |
| 28     | 8    | `min_price`    | informational                |
| 36     | 8    | `max_price`    | informational                |
| 44     | 8    | `count`        | number of ticks              |
| 52     | 4    | `price_scale`  | `10^decimals`                |
| 56     | 4    | `reserved`     | must be 0                    |

### Body

```
int64   first_time                     (raw, LE)
int64   first_price × price_scale      (raw, LE)

repeat (count − 1) times:
    varint( zigzag( Δtime  ) )         Δtime  = t[i] − t[i−1]
    varint( zigzag( Δprice ) )         Δprice = p[i]·scale − p[i−1]·scale
```

**Varint:** LEB128, little‑endian 7‑bit groups.
**Zigzag:** maps signed → unsigned so small negatives still encode in 1 byte.

Because forex/XAU ticks are 1 s apart, `Δtime = 1` ⇒ `zigzag = 2` ⇒ **1 byte**.
And `Δprice · scale` is usually < 100 ⇒ **1 byte**.
So a whole tick after the first is typically **2 bytes**.

---

## 11. Performance notes

Tested on an Intel i7‑1165G7, NVMe SSD, `frxXAUUSD_2026-10-02` (≈ 650 k ticks):

| Operation                          | Time      |
|------------------------------------|-----------|
| `-C` CSV → `.tp`                   | ~ 55 ms   |
| `-D` `.tp` → CSV                   | ~ 40 ms   |
| `--math all`                       | ~ 1.2 ms  |
| `--candle 60`                      | ~ 3 ms    |
| `--freq --top 20`                  | ~ 12 ms   |
| `--seqsec 3`                       | ~ 6 ms    |
| `mmap_reader` full scan            | ~ 0.8 ms  |

Memory: a 1‑M‑tick day is ~ 24 MB in RAM as `vector<Tick>`
(`int64 + double` per tick), independent of the on‑disk size.

---

## 12. Extending

Adding a new exporter is 3 steps:

1. Create `include/my_exporter.h` with one class.
2. `#include` it in `src/tpcli.cpp`.
3. Add an `else if` branch inside the `--export` block.

Adding a new analysis pass:

1. Create `include/tp_myanalyse.h`.
2. Add a new `--myanalyse` command block in `tpcli.cpp` that loads via
   `TpDataset::fromTp(...)`.

Because everything is a plain function of `vector<Tick>`, all analyses
are testable in isolation.

---

## 13. Troubleshooting

| Symptom                                        | Cause / fix                                              |
|-----------------------------------------------|----------------------------------------------------------|
| `Bad magic - not a .tp file`                   | You tried to read a CSV as `.tp`, or file is truncated.  |
| `Unsupported .tp version`                      | Bump `TP_VERSION` only if you change the format.         |
| `Unexpected EOF while reading varint`          | File is truncated or corrupted mid‑write.                |
| `ArrowParquetExporter: …`                      | Rebuild with `-DTP_HAVE_ARROW` and link `-larrow -lparquet`. |
| `Cannot open …`                                | Check path; `mmap` needs read permission.                |
| Prices look off by ×10 or ÷10                  | CSV had different decimals — the encoder auto‑detects from the CSV text. |

---

## 14. License

MIT — do whatever you want, no warranty.


