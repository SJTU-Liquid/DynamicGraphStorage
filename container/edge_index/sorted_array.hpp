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

namespace container {
    template<typename EdgeEntry>
    struct SortedArrayIterator;

    template<typename EdgeEntry>
    struct SortedArrayEdgeIndex {
        std::vector<EdgeEntry>* m_arr;

        SortedArrayEdgeIndex() {
            m_arr = new std::vector<EdgeEntry>();
        }
        
        ~SortedArrayEdgeIndex() = default;

        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) const {
            EdgeEntry value(dest);
            auto it = std::lower_bound(m_arr->begin(), m_arr->end(), value);
            if (it != m_arr->end() && it->get_dest() == dest && it->check_version(timestamp)) {
                return true;
            }
            return false;
        }

        uint64_t intersect(const SortedArrayEdgeIndex<EdgeEntry> & other, uint64_t timestamp) const {
            uint64_t sum = 0;
            auto neighbor_a = m_arr;
            auto iter_a = neighbor_a->begin();
            auto neighbor_b = other.m_arr;
            auto iter_b = neighbor_b->begin();

            // TODO: add optimization
            while (iter_a != neighbor_a->end() && iter_b != neighbor_b->end()) {
                if (iter_a->get_dest() < iter_b->get_dest()) {
                    ++iter_a;
                } else if (iter_a->get_dest() > iter_b->get_dest()) {
                    ++iter_b;
                } else {
                    sum++;
                    ++iter_a;
                    ++iter_b;
                }
            }
            return sum;
        }


        bool insert_edge(uint64_t dest, uint64_t timestamp) {
            EdgeEntry entry(dest);
            auto iter = std::lower_bound(m_arr->begin(), m_arr->end(), entry);
            auto value = EdgeEntry{dest, timestamp};

            if (iter != m_arr->end() && iter->get_dest() == dest) {
                iter->update_version(timestamp);
                return false;
            } else {
                m_arr->insert(iter, std::move(value));
            }
            return true;
        }

        uint64_t insert_edge_batch(const std::vector<uint64_t> &dest_list, uint64_t timestamp) {
            // uint64_t sum = 0;
            // std::vector<EdgeEntry> result;
            // result.reserve(m_arr->size() + dest_list.size());

            // auto old_iter = m_arr->begin();
            // auto new_iter = dest_list.begin();
            
            // while (old_iter != m_arr->end() && new_iter != dest_list.end()) {
            //     if (old_iter->get_dest() < *new_iter) {
            //         result.push_back(std::move(*old_iter));
            //         old_iter++;
            //     } else if (old_iter->get_dest() > *new_iter) {
            //         result.emplace_back(*new_iter, timestamp);
            //         new_iter++;
            //         sum++;
            //     } else {
            //         old_iter->update_version(timestamp);
            //         result.push_back(std::move(*old_iter));
            //         new_iter++;
            //         old_iter++;
            //     }
            // }

            // while (old_iter != m_arr->end()) {
            //     result.push_back(std::move(*old_iter));
            //     old_iter++;
            // }

            // while (new_iter != dest_list.end()) {
            //     result.emplace_back(*new_iter, timestamp);
            //     new_iter++;
            //     sum++;
            // }

            // m_arr->swap(result);

            // return sum;
            uint64_t sum = 0;
            for (auto & dest : dest_list) {
                sum += insert_edge(dest, timestamp);
            }
            return sum;
        }

        template<typename F>
        uint64_t edges(F&& callback, uint64_t timestamp) const {
            uint64_t sum = 0;
            uint64_t size = m_arr->size();
            for (uint64_t i = 0; i < size; i++) {
                auto & entry = (*m_arr)[i];
                if (__builtin_expect(entry.check_version(timestamp), 1)) {
                    bool should_continue = callback(entry.get_dest(), 0.0); // NOTE: Do not support weight now
                    sum += entry.get_dest();
                    if (!should_continue) break;
                }
            }
            return sum;
        }

        void init_graph(std::vector<uint64_t> &dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg = EdgeDriverConfig(), std::vector<uint64_t> timestamp_arr = std::vector<uint64_t>{}) {
            std::sort(dest.begin() + start, dest.begin() + end);
            std::vector<uint64_t> unique;
            uint64_t prev = 0;
            for (uint64_t ptr = start; ptr < end; ptr++) {
                if (ptr == start || dest[ptr] != prev) unique.push_back(dest[ptr]);
                prev = dest[ptr];
            }
            // m_arr->resize(unique.size());
            for (uint64_t i = 0; i < unique.size(); i++) {
                auto entry = EdgeEntry{unique[i], i + start};
                m_arr->push_back(std::move(entry)); 
            }
        }

        void clear() {
            if (m_arr) delete m_arr;
            m_arr = nullptr;
        }

        SortedArrayIterator<EdgeEntry> get_begin(uint64_t timestamp) const {
            return SortedArrayIterator<EdgeEntry>(m_arr->begin(), m_arr, timestamp);
        }
    };

    template<typename EdgeEntry>
    struct SortedArrayIterator {
        typename std::vector<EdgeEntry>::iterator it;
        std::vector<EdgeEntry>* iter_arr;
        uint64_t timestamp;

        SortedArrayIterator(typename std::vector<EdgeEntry>::iterator iter, std::vector<EdgeEntry>* arr, uint64_t ts) 
            : it(iter), iter_arr(arr), timestamp(ts) {}
        

        bool valid() {
            return (it != iter_arr->end());
        }

        SortedArrayIterator& operator++() {
            do {
                ++it;
            } while (valid() && !it->check_version(timestamp));
            return (*this);
        }

        EdgeEntry* operator->() {
            return &(*it);
        }

        EdgeEntry& operator*() {
            return *it;
        }
    };
}
