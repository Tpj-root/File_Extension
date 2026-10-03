#pragma once
#include "tp_reader.h"
#include "tp_writer.h"
#include "tp_format.h"
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace tp {

class TpMerger {
public:
    struct Options {
        bool dedupe          = true;    // same timestamp -> keep last
        bool alreadySorted   = false;   // skip sort if inputs already ordered
        int  decimals        = 6;
    };

	static void mergeToFile(const std::vector<std::string>& inputs,
	                        const std::string& output)
	{
	    mergeToFile(inputs, output, Options{});
	}

	static void mergeToFile(const std::vector<std::string>& inputs,
	                        const std::string& output,
	                        Options opt)
    {
        if (inputs.empty()) throw std::runtime_error("no input files");

        // Streaming k-way merge by tick count (memory O(total) but simple)
        std::vector<Tick> all;
        size_t total = 0;
        for (auto& p : inputs) {
            TpReader r(p); r.open();
            total += (size_t)r.header().count;
        }
        all.reserve(total);

        for (auto& p : inputs) {
            TpReader r(p); r.open();
            Tick t;
            while (r.next(t)) all.push_back(t);
        }

        if (!opt.alreadySorted) {
            std::stable_sort(all.begin(), all.end(),
                [](const Tick& a, const Tick& b){ return a.time < b.time; });
        }

        if (opt.dedupe && all.size() > 1) {
            std::vector<Tick> ded;
            ded.reserve(all.size());
            for (auto& t : all) {
                if (!ded.empty() && ded.back().time == t.time)
                    ded.back() = t;   // keep last occurrence
                else
                    ded.push_back(t);
            }
            all.swap(ded);
        }

        TpWriter(output).write(all, opt.decimals);
    }
};

} // namespace tp