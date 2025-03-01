#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace container {
#define SKIP_LIST_LEVELS 6

    template<typename EdgeEntry>
    struct EdgeBlock {
        EdgeBlock *before;  
        std::vector<EdgeEntry> impl;
        uint16_t size;  
        uint64_t max;
        EdgeBlock *next_levels[SKIP_LIST_LEVELS];  

        EdgeBlock(uint64_t capacity = 256) : before(nullptr), size(0), max(0) {
            impl.resize(capacity);
            for (int i = 0; i < SKIP_LIST_LEVELS; i++) next_levels[i] = nullptr;
        }

        EdgeBlock(EdgeBlock<EdgeEntry> *bf, uint16_t sz, uint64_t mx,uint64_t capacity = 256) : before(bf), size(sz), max(mx) {
            impl.resize(capacity);
            for (int i = 0; i < SKIP_LIST_LEVELS; i++) next_levels[i] = nullptr;
        }

        void split(EdgeBlock<EdgeEntry> &other) {
            auto split = size / 2;
            for (uint64_t i = 0; i < split; i++) {
                other.impl[i] = std::move(impl[i + split]);
            }
            other.size = split;
            size = size - split;
            other.max = other.impl[other.size - 1].get_dest();
            max = impl[size - 1].get_dest();
        }

        bool insert_edge(uint64_t dest, uint64_t timestamp) {
            EdgeEntry value(dest);
            auto pos = std::lower_bound(impl.begin(), impl.begin() + size, value);
            if (pos != impl.begin() + size && pos->get_dest() == dest) {
                pos->update_version(timestamp);
                return false;
            } else {
                std::move_backward(pos, impl.begin() + size, impl.begin() + size + 1);
                *pos = EdgeEntry(dest, timestamp);
                size++;
                max = impl[size - 1].dest;
            }
            return true;
        }
    };
}