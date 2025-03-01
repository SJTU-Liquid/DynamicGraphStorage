#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>
#include <limits>
#include <memory>
#include <stdexcept>

#include "utils/bloom_filter/simd-block-fixed-fpp.hpp"
#include "utils/types.hpp"
#include "types/types.hpp"
#include "utils/config.hpp"

namespace container {
    template<typename EdgeEntry>
    struct LogBlockIterator;
    
    int log2(int n) {
        int lg2 = 0;
        while (n > 1) {
            n >>= 1;
            ++lg2;
        }
        return lg2;
    }

    template<typename EdgeEntry>
    struct LogBlockEdgeIndex {
        std::vector<EdgeEntry>* m_block;
        SimdBlockFilterFixed<>* m_filter;  
        size_t m_log_num;
        LogBlockEdgeIndex() {
            m_filter = new SimdBlockFilterFixed<>(16);
            m_block = new std::vector<EdgeEntry>(0);
            m_block->reserve(16);
            m_log_num = 0;
        }
        ~LogBlockEdgeIndex() {
            delete m_block;
            delete m_filter;
        }
    
        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
            if (m_log_num == 0) {
                return false;
            }
    
            if(!m_filter->Find(dest)) {
                return false;
            }
            
            auto iter = m_block->end() - 1;
            for (int64_t idx = 0; idx < m_log_num; idx++) {
                if (iter->get_dest() == dest && iter->check_version(timestamp)) {
                    return true;
                }
                iter--;
            }

            return false;
        }

        bool insert_edge(uint64_t dest, uint64_t timestamp) {
            auto entry = EdgeEntry{dest, timestamp};

            // update old logs
            bool new_edge_flag = true;
            if(m_filter->Find(dest)) {
                // printf("%ld, %ld\n", hit, m_log_num);
                for (int64_t idx = m_log_num - 1; idx > 0; idx--) {
                    auto &cur_log = (*m_block)[idx];
                    if (cur_log.get_dest() == dest && cur_log.check_is_newest(timestamp)) {
                        cur_log.update_version(timestamp);
                        new_edge_flag = false;
                        break;
                    }

                }
            }
            m_filter->Add(dest);
    
            m_block->push_back(entry);
            m_log_num++;
            int log_size = log2(m_log_num);
            if (log_size > 4 && (log_size % 4 == 0) && (1 << log_size) == m_log_num) {
                delete m_filter;
                m_filter = new SimdBlockFilterFixed<>(m_log_num);
                for (uint64_t i = 0; i < m_log_num; i++) {
                    m_filter->Add((*m_block)[i].get_dest());
                }
            }
            return new_edge_flag;
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
            auto size = m_log_num;
            auto block_ptr = m_block;
            uint64_t ret = 0;
            auto iter = block_ptr->end() - 1;
    
            for (int64_t idx = 0; idx < size; idx++) {
                if (__builtin_expect(iter->check_version(timestamp), 1)) {
                    callback(iter->get_dest(), 0.0); // NOTE: Do not support weight now
                    ret += iter->get_dest();
                }
                iter--;
            }
            return ret;
        }

    
        void clear() {
            if (m_block) delete m_block;
            m_block = nullptr;
        }

        LogBlockIterator<EdgeEntry> get_begin(uint64_t timestamp) const {
            return LogBlockIterator<EdgeEntry>(m_block->begin(), m_block, timestamp);
        }

        uint64_t intersect(const LogBlockEdgeIndex &, uint64_t timestamp) const {return 0;}

    };


    template<typename EdgeEntry>
    struct LogBlockIterator {
        typename std::vector<EdgeEntry>::iterator it;
        std::vector<EdgeEntry>* iter_arr;
        uint64_t timestamp;

        LogBlockIterator(typename std::vector<EdgeEntry>::iterator iter, std::vector<EdgeEntry>* arr, uint64_t ts) 
            : it(iter), iter_arr(arr), timestamp(ts) {}
        

        bool valid() {
            return (it != iter_arr->end());
        }

        LogBlockIterator& operator++() {
            do {
                ++it;
            } while (valid() && !it->check_version(timestamp));
            return (*this);
        }

        EdgeEntry* operator->() {
            return &(*it);
        }
    };
}