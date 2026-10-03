#pragma once
#include "tp_format.h"
#include "tp_varint.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <cstdint>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace tp {

// Zero-copy, read-only mmap reader. Ideal for very large .tp files.
class MmapTpReader {
public:
    explicit MmapTpReader(std::string path) : path_(std::move(path)) {}
    ~MmapTpReader() { close(); }
    MmapTpReader(const MmapTpReader&)            = delete;
    MmapTpReader& operator=(const MmapTpReader&) = delete;

    void open() {
        fd_ = ::open(path_.c_str(), O_RDONLY);
        if (fd_ < 0) throw std::runtime_error("Cannot open " + path_);

        struct stat st{};
        if (::fstat(fd_, &st) != 0) { ::close(fd_); fd_ = -1; throw std::runtime_error("fstat"); }
        size_ = (size_t)st.st_size;
        if (size_ < sizeof(TpHeader)) { ::close(fd_); fd_=-1; throw std::runtime_error("Truncated header"); }

        void* p = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
        if (p == MAP_FAILED) { ::close(fd_); fd_ = -1; throw std::runtime_error("mmap failed"); }
        base_ = static_cast<const uint8_t*>(p);

        std::memcpy(&header_, base_, sizeof(TpHeader));
        if (std::memcmp(header_.magic, TP_MAGIC, 4) != 0)
            throw std::runtime_error("Bad magic");
        if (header_.version != TP_VERSION)
            throw std::runtime_error("Unsupported version");

        pos_ = sizeof(TpHeader);
        if (header_.count > 0) {
            if (pos_ + 16 > size_) throw std::runtime_error("Truncated body");
            std::memcpy(&curT_, base_ + pos_, 8); pos_ += 8;
            std::memcpy(&curP_, base_ + pos_, 8); pos_ += 8;
            idx_ = 1;
        }
    }

    void close() {
        if (base_) { ::munmap(const_cast<uint8_t*>(base_), size_); base_ = nullptr; }
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
        size_ = pos_ = 0;
    }

    const TpHeader& header() const { return header_; }

    bool next(Tick& t) {
        if (idx_ == 0) {
            if (header_.count == 0) return false;
            t.time  = curT_;
            t.price = double(curP_) / double(header_.price_scale);
            return true;
        }
        if (idx_ >= header_.count) return false;
        const int64_t dT = zigzagDecode(getVarintMem());
        const int64_t dP = zigzagDecode(getVarintMem());
        curT_ += dT; curP_ += dP; ++idx_;
        t.time  = curT_;
        t.price = double(curP_) / double(header_.price_scale);
        return true;
    }

    std::vector<Tick> readAll() {
        std::vector<Tick> v; v.reserve((size_t)header_.count);
        Tick t; while (next(t)) v.push_back(t);
        return v;
    }

private:
    uint64_t getVarintMem() {
        uint64_t v = 0; int shift = 0;
        while (pos_ < size_) {
            uint8_t c = base_[pos_++];
            v |= uint64_t(c & 0x7F) << shift;
            if (!(c & 0x80)) return v;
            shift += 7;
            if (shift > 63) throw std::runtime_error("varint overflow");
        }
        throw std::runtime_error("EOF reading varint");
    }

    std::string     path_;
    int             fd_   = -1;
    const uint8_t*  base_ = nullptr;
    size_t          size_ = 0;
    size_t          pos_  = 0;
    TpHeader        header_{};
    int64_t         curT_ = 0, curP_ = 0;
    uint64_t        idx_  = 0;
};

} // namespace tp