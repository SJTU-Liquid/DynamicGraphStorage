#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>
#include <limits>
#include <memory>
#include <stdexcept>

#include "utils/types.hpp"
#include "utils/config.hpp"
#include "../types/types.hpp"
#include "skiplist/skiplist.hpp"

namespace container {
    template<typename EdgeEntry>
    struct SkipListEdgeIndex {
        SkipList<EdgeEntry>* m_skiplist;

        SkipListEdgeIndex(uint64_t block_size = container::config::BLOCK_SIZE) {
            m_skiplist = new SkipList<EdgeEntry>(block_size);
        }
        
        ~SkipListEdgeIndex() {
            delete m_skiplist;  
        }

        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) const {
            return m_skiplist->has_edge(dest, timestamp);
        }

        uint64_t intersect(const SkipListEdgeIndex<EdgeEntry> & other, uint64_t timestamp) const {
            uint64_t sum = 0;
            auto neighbor_a = m_skiplist;
            auto iter_a = neighbor_a->begin(timestamp);
            auto neighbor_b = other.m_skiplist;
            auto iter_b = neighbor_b->begin(timestamp);

            // TODO: add optimization
            while (iter_a.valid() && iter_b.valid()) {
                if (iter_a->get_dest() < iter_b->get_dest()) ++iter_a;
                else if (iter_a->get_dest() > iter_b->get_dest()) ++iter_b;
                else {
                    sum++;
                    ++iter_a;
                    ++iter_b;
                }
            }
            return sum;
        }


        bool insert_edge(uint64_t dest, uint64_t timestamp) {
            return m_skiplist->insert_edge(dest, timestamp);
        }

        uint64_t insert_edge_batch(const std::vector<uint64_t> &dest_list, uint64_t timestamp) {
            uint64_t sum = 0;
            for (auto & dest : dest_list) {
                sum += insert_edge(dest, timestamp);
            }
            return sum;
        }

        template<typename F>
        uint64_t edges(F&& callback, uint64_t timestamp) const {
            uint64_t sum = 0;
            auto iter = m_skiplist->begin(timestamp);
            while (iter.valid()) {
                bool should_continue = callback(iter->get_dest(), 0.0); // NOTE: Do not support weight now
                sum += iter->get_dest();
                iter++;
                if (!should_continue) break;
            }
            return sum;
        }


        void clear() {
            if (m_skiplist) delete m_skiplist;
            m_skiplist = nullptr;
        }

        SkipListIterator<EdgeEntry> get_begin(uint64_t timestamp) const {
            return m_skiplist->begin(timestamp);
        }
    };
}
