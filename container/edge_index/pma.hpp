#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <chrono>
#include <cstring>
#include <immintrin.h>

#include "utils/types.hpp"
#include "utils/unordered_dense/include/ankerl/unordered_dense.h"
#include "types/types.hpp"


int log2(int n) {
    int lg2 = 0;
    while (n > 1) {
        n >>= 1;
        ++lg2;
    }
    return lg2;
}

// Definition
namespace container {
    template<typename EdgeEntry>
    struct PMAIndex;

    template<typename EdgeEntry>
    struct PMAIterator;

    template<typename EdgeEntry>
    struct PMALeaf {
        // PMA
        std::vector<EdgeEntry> impl;
        std::vector<uint64_t> sizes;
        // std::vector<EdgeEntry> tmp;
        // std::vector<char> exist;
        
        uint64_t element_num;     // number of elements in the pma
        uint64_t segment_size;
        uint64_t segment_num;
        uint64_t level_num;
        uint64_t lgn;

        PMALeaf(uint64_t capacity = container::config::BLOCK_SIZE, uint64_t segment_size = container::config::BLOCK_SIZE);

        ~PMALeaf() = default;

        /// Returns the upper threshold for the given level. The upper threshold limits the number of elements in a segment.
        double upper_threshold_at(uint64_t level) const;

        void init_vars(uint64_t capacity);

        /// Returns the left boundary of the interval that contains the element at index 'i'.
        /// @example
        /// left_interval_boundary(54, 8) returns 48
        static uint64_t left_interval_boundary(uint64_t i, uint64_t interval_size);

        /// Grow the array to the given capacity.
        void resize(int capacity);

        /// Check if the interval start at 'left' is within limits. And count the number of elements in the interval.
        void get_interval_stats(uint64_t left, uint64_t level, bool &in_limit, uint64_t &sz);

        // Check if the value is in the segment
        int lb_in_segment(uint64_t l, uint64_t v);

        /// @param v the value to find the lower bound for
        /// @return the left index of the segment that contains the first element that is >= 'v' if exists, otherwise the size of the array.
        uint64_t lower_bound(uint64_t dest);


        /// @return true if insert a new entry, false if insert a version
        bool insert_merge(uint64_t left, uint64_t dest, uint64_t timestamp);

        void rebalance_interval(uint64_t left, uint64_t level);

        bool insert(uint64_t dest, uint64_t timestamp);

        PMAIterator<EdgeEntry> begin(uint64_t);


        [[nodiscard]] uint64_t size() const;

        void print();

        void clear();
    };

    template<typename EdgeEntry>
    struct PMAIterator {
        PMALeaf<EdgeEntry> *m_leaf;
        uint64_t block_id;
        uint64_t element_id;
        uint64_t timestamp;

        PMAIterator(PMALeaf<EdgeEntry>* leaf, uint64_t ts) : m_leaf(leaf), timestamp(ts), block_id(0), element_id(0) {}
        PMAIterator(PMALeaf<EdgeEntry>* leaf, uint64_t ts, uint64_t block, uint64_t element) : m_leaf(leaf), timestamp(ts), block_id(block), element_id(element) {}
        PMAIterator(const PMAIterator &rhs) {
            this->m_leaf = rhs.m_leaf;
            this->block_id = rhs.block_id;
            this->element_id = rhs.element_id;
            this->timestamp = rhs.timestamp;
        }

        PMAIterator& operator=(PMAIterator &rhs) {
            this->m_leaf = rhs.m_leaf;
            this->block_id = rhs.block_id;
            this->element_id = rhs.element_id;
            this->timestamp = rhs.timestamp;
            return *this;
        }

        PMAIterator& operator++() {
            do {
                element_id++;
                if (element_id == m_leaf->sizes[block_id]) {
                    block_id++;
                    element_id = 0;
                }
            } while (valid() && !m_leaf->impl[block_id * m_leaf->segment_size + element_id].check_version(timestamp));
            return *this;
        }

        PMAIterator operator++(int) {
            PMAIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const PMAIterator & rhs) {
            return this->m_leaf == rhs.m_leaf && this->block_id == rhs.block_id && this->element_id == rhs.element_id;
        }

        bool operator!=(const PMAIterator & rhs) {
            return !(*this == rhs);
        }

        EdgeEntry& operator*() {
            return m_leaf->impl[block_id * m_leaf->segment_size + element_id];
        }

        EdgeEntry* operator->() {
            return &(m_leaf->impl[block_id * m_leaf->segment_size + element_id]);
        }

        bool valid() {
            return (block_id < m_leaf->segment_num) && (element_id < m_leaf->sizes[block_id]);
        }
    };

    template<typename EdgeEntry>
    struct PMAIndex {
        RWSpinLock index_lock{};
        std::vector<RequiredLock> required_locks{};
        PMALeaf<EdgeEntry>* m_leaf;

        PMAIndex();

        ~PMAIndex() {}
        
        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp);

        template<typename F>
        uint64_t edges(F&& callback, uint64_t timestamp);

        bool insert_edge(uint64_t dest, uint64_t timestamp);

        uint64_t insert_edge_batch(const std::vector<uint64_t> &dest_list, uint64_t timestamp);

        uint64_t init_graph(std::vector<uint64_t> &dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg);

        void clear_neighbor();
        void clear() {}

        uint64_t intersect(const PMAIndex<EdgeEntry> & other, uint64_t timestamp) const;

        PMAIterator<EdgeEntry> get_begin(uint64_t timestamp) const {
            return m_leaf->begin(timestamp);
        }
    };
}


// Implementation
namespace container {
    template<typename EdgeEntry>
    PMALeaf<EdgeEntry>::PMALeaf(uint64_t capacity, uint64_t segment_size) : element_num(0) {
        assert(capacity > 1);
        assert(1 << log2(capacity) == capacity);
        assert(1 << log2(segment_size) == segment_size);
        assert(segment_size <= capacity);

        this->segment_size = segment_size;
        this->init_vars(capacity);
        this->impl.resize(capacity);
        this->sizes.resize(this->segment_num, 0);
    }

    /// Returns the upper threshold for the given level. The upper threshold limits the number of elements in a segment.
    template<typename EdgeEntry>
    double PMALeaf<EdgeEntry>::upper_threshold_at(uint64_t level) const {
        assert(level <= this->level_num);
        double threshold = 1.0 - ((1.0 - 0.5) * level) / (double) this->lgn;
        return threshold;
    }

    template<typename EdgeEntry>
    void PMALeaf<EdgeEntry>::init_vars(uint64_t capacity) {
        assert(this->segment_size == (1 << log2(this->segment_size)));
        this->segment_num = capacity / this->segment_size;
        this->level_num = log2(this->segment_num);
        this->lgn = log2(capacity);
    }

    /// Returns the left boundary of the interval that contains the element at index 'i'.
    /// @example
    /// left_interval_boundary(54, 8) returns 48
    template<typename EdgeEntry>
    inline uint64_t PMALeaf<EdgeEntry>::left_interval_boundary(uint64_t i, uint64_t interval_size) {
        assert(interval_size == (1 << log2(interval_size)));

        uint64_t q = i / interval_size;
        uint64_t boundary = q * interval_size;
        return boundary;
    }

    /// Grow the array to the given capacity.
    template<typename EdgeEntry>
    void PMALeaf<EdgeEntry>::resize(int capacity) {
        assert(capacity > this->impl.size());
        assert(1 << log2(capacity) == capacity);
        uint64_t new_segment_num = capacity / this->segment_size;

        std::vector<EdgeEntry> tmp_buffer(this->element_num);
        uint64_t tmp_size = 0;
        
        for (uint64_t i = 0; i < this->segment_num; i++) {
            for (uint64_t j = 0; j < this->sizes[i]; j++) {
                tmp_buffer[tmp_size++] = std::move(this->impl[i * segment_size + j]);
            }
        }
        assert(tmp_size == this->element_num);

        std::vector<EdgeEntry> new_impl(capacity);
        std::vector<uint64_t> new_sizes(new_segment_num);

        uint64_t tmp_idx = 0;
        uint64_t average_size = (tmp_size + new_segment_num - 1) / new_segment_num;

        for (uint64_t i = 0; i < new_segment_num; i++) {
            uint64_t j = 0;
            uint64_t remain_size = std::min(average_size, tmp_size - tmp_idx);
            for (j = 0; j < remain_size; j++) {
                new_impl[i * segment_size + j] = std::move(tmp_buffer[tmp_idx++]);
            }
            new_sizes[i] = j;
        }
        assert(tmp_idx == this->element_num);

        this->impl.swap(new_impl);
        this->sizes.swap(new_sizes);
        this->init_vars(capacity);
    }

    /// Check if the interval start at 'left' is within limits. And count the number of elements in the interval.
    template<typename EdgeEntry>
    void PMALeaf<EdgeEntry>::get_interval_stats(uint64_t left, uint64_t level, bool &in_limit, uint64_t &sz) {
        double t = upper_threshold_at(level);
        uint64_t w = (1 << level) * this->segment_size;
        sz = 0;
        for (uint64_t i = left; i < left + w; i += segment_size) {
            uint64_t idx = i / segment_size;
            sz += this->sizes[idx];
        }
        double q = (double) (sz + 1) / double(w);
        in_limit = q < t;
    }


    // @param v the value to find the lower bound for
    // @return 1 if the right side, -1 if the left side, 0 if in this segment
    template<typename EdgeEntry>
    int PMALeaf<EdgeEntry>::lb_in_segment(uint64_t left, uint64_t v) {
        if (this->sizes[left / segment_size] == 0) return -1;
        uint64_t right = left + this->sizes[left / segment_size] - 1;
        if (this->impl[right].get_dest() < v) return 1;
        if (this->impl[left].get_dest() > v) return -1;
        return 0;
    }

    /// @param v the value to find the lower bound for
    /// @return the left index of the segment that contains the first element that is >= 'v' if exists, otherwise the size of the array.
    template<typename EdgeEntry>
    uint64_t PMALeaf<EdgeEntry>::lower_bound(uint64_t dest) {
        if (this->element_num == 0) {
            return impl.size();
        }
        int l = 0, r = this->segment_num - 1;
        int m;

        if (l == r) return 0;

        while (l <= r) {
            m = l + (r - l) / 2;
            // TODO: remove left_interval_boundary
            uint64_t left = left_interval_boundary(m * segment_size, segment_size);
            // uint64_t left = m * segment_size;
            int pos = lb_in_segment(left, dest);

            if (pos == 1) { // find in the right side
                l = m + 1;
            } else if (pos == -1) { // find in the left side
                r = m - 1;
            } else {
                return m * segment_size;
            }
        }
        return l * segment_size;
    }


    /// @return true if insert a new entry, false if insert a version
    template<typename EdgeEntry>
    bool PMALeaf<EdgeEntry>::insert_merge(uint64_t left, uint64_t dest, uint64_t timestamp) {
        uint64_t segment_id = left / segment_size;
        uint64_t cur_segment_size = sizes[segment_id];
        assert(cur_segment_size < segment_size);
        auto segment_begin = this->impl.begin() + left;

        EdgeEntry value(dest);
        auto iter = std::lower_bound(segment_begin, segment_begin + cur_segment_size, value);
        bool insert_entry_flag = false;
        if (iter != segment_begin + cur_segment_size && iter->get_dest() == dest) {
            iter->update_version(timestamp);
        } else {
            if (iter == segment_begin + cur_segment_size) {
                this->impl[left + cur_segment_size] = EdgeEntry(dest, timestamp);
            } else {
                uint64_t offset = std::distance(segment_begin, iter);
                std::move_backward(segment_begin + offset, segment_begin + cur_segment_size, segment_begin + cur_segment_size + 1);
                *iter = EdgeEntry(dest, timestamp);
            }
            ++this->element_num;
            ++this->sizes[segment_id];
            insert_entry_flag = true;
        }
        return insert_entry_flag;
    }

    template<typename EdgeEntry>
    void PMALeaf<EdgeEntry>::rebalance_interval(uint64_t left, uint64_t level) {
        uint64_t w = (1 << level) * this->segment_size;
        std::vector<EdgeEntry> tmp_buffer(w);
        size_t tmp_size = 0;
        
        for (uint64_t i = left; i < left + w; i += segment_size) {
            for (uint64_t j = 0; j < this->sizes[i / this->segment_size]; j++) {
                tmp_buffer[tmp_size++] = std::move(this->impl[i + j]);
            }
        }

        size_t average_size = (tmp_size + (1 << level) - 1) / (1 << level);
        size_t tmp_idx = 0;
        
        for (uint64_t i = left; i < left + w; i += segment_size) {
            uint64_t j = 0;
            uint64_t remain_size = std::min(average_size, tmp_size - tmp_idx);
            for (j = 0; j < remain_size; j++) {
                this->impl[i + j] = std::move(tmp_buffer[tmp_idx++]);
            }
            this->sizes[i / this->segment_size] = j;
        }
        assert(tmp_idx == tmp_size);
    }

    template<typename EdgeEntry>
    bool PMALeaf<EdgeEntry>::insert(uint64_t dest, uint64_t timestamp) {
        uint64_t i = lower_bound(dest);
        if (i == this->impl.size()) {
            --i;
        }
        assert(i < this->impl.size());

        // Check in a window of size 'w'
        uint64_t w = segment_size;
        uint64_t level = 0;
        
        uint64_t l = left_interval_boundary(i, w);
        uint64_t idx = l / segment_size;
        // Number of elements in current window. We just need sz to be
        // less than w -- we don't need the exact value of 'sz' here.
        uint64_t sz = w - 1;
        bool in_limit = false;

        // If the current segment has space, then the last element will
        // be unused (with significant probability). First check that
        // as a quick check.
        if (this->sizes[idx] < segment_size) {
            return this->insert_merge(l, dest, timestamp);
        } else {
            // No space in this interval. Find an interval above this
            // interval that is within limits, re-balance, and
            // re-start insertion.
            while (!in_limit) {
                w *= 2;
                level += 1;
                // assert(level <= this->level_num);
                if (level > this->level_num) {
                    // Root node is out of balance. Resize array.
                    this->resize(2 * this->impl.size());
                    return this->insert(dest, timestamp);
                }

                l = left_interval_boundary(i, w);
                get_interval_stats(l, level, in_limit, sz);
            }
            this->rebalance_interval(l, level);
            return this->insert(dest, timestamp);
        }
    }

    template<typename EdgeEntry>
    [[nodiscard]] uint64_t PMALeaf<EdgeEntry>::size() const {
        return this->element_num;
    }

    template<typename EdgeEntry>
    PMAIterator<EdgeEntry> PMALeaf<EdgeEntry>::begin(uint64_t timestamp) {
        return PMAIterator(this, timestamp);
    }

    // void PMALeaf::print() {
    //     for (size_t i = 0; i < this->impl.size(); ++i) {
    //         // print the segment boundary and fence
    //         if (i % segment_size == 0) {
    //             std::cout << "\033[1;31m" << "|" << "\033[0m";
    //         }

    //         // print element
    //         if (this->exist[i]) {
    //             std::cout << "\033[1;34m" << this->impl[i].dest << "\033[0m ";
    //         } else {
    //             std::cout << "-1 ";
    //         }
    //     }
    //     printf("\n");
    // }

    template<typename EdgeEntry>
    void PMALeaf<EdgeEntry>::clear() {
        impl.clear();
        sizes.clear();
    }

    template<typename EdgeEntry>
    PMAIndex<EdgeEntry>::PMAIndex() {
        m_leaf = new PMALeaf<EdgeEntry>();
    }

    template<typename EdgeEntry>
    void PMAIndex<EdgeEntry>::clear_neighbor() {
        if (m_leaf) {
            m_leaf->clear();
            delete m_leaf;
            m_leaf = nullptr;
        }
    }

    template<typename EdgeEntry>
    bool PMAIndex<EdgeEntry>::has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        uint64_t i = m_leaf->lower_bound(dest);
        if (i == m_leaf->impl.size()) 
            return false;
        
        int left = m_leaf->left_interval_boundary(i, m_leaf->segment_size);
        int right = left + m_leaf->sizes[left / m_leaf->segment_size] - 1;

        EdgeEntry value(dest);
        auto pos = std::lower_bound(m_leaf->impl.begin() + left, m_leaf->impl.begin() + right, value);
        if (pos != m_leaf->impl.end() && pos->get_dest() == dest) {
            return (pos->check_version(timestamp));
        }
        return false;
    }

    template<typename EdgeEntry>
    uint64_t PMAIndex<EdgeEntry>::intersect(const PMAIndex<EdgeEntry> & other, uint64_t timestamp) const {
        auto neighbor_a = m_leaf;
        auto neighbor_b = other.m_leaf;
        
        uint64_t sum = 0;
        auto iter_a = neighbor_a->begin(timestamp);
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

    template<typename EdgeEntry>
    template<typename F>
    uint64_t PMAIndex<EdgeEntry>::edges(F &&callback, uint64_t timestamp) {
        uint64_t scan_sum = 0;

        for (uint64_t i = 0; i < m_leaf->segment_num; i++) {
            uint64_t size = m_leaf->sizes[i];
            for (uint64_t j = 0; j < size; j++) {
                auto & entry = m_leaf->impl[i * m_leaf->segment_size + j];
                if (__builtin_expect(entry.check_version(timestamp), 1)) {
                    bool should_continue = callback(entry.get_dest(), 0.0);
                    scan_sum += entry.get_dest();
                    if (!should_continue) 
                        return scan_sum;
                }
            }
        }
        return scan_sum;
    }

    // Insert to last leaf if the neighbor does not exist, otherwise insert to the leaf where the neighbor exists
    template<typename EdgeEntry>
    bool PMAIndex<EdgeEntry>::insert_edge(uint64_t dest, uint64_t timestamp) {
        return m_leaf->insert(dest, timestamp);
    }

    template<typename EdgeEntry>
    uint64_t PMAIndex<EdgeEntry>::insert_edge_batch(const std::vector<uint64_t> &dest_list, uint64_t timestamp) {
        uint64_t sum = 0;
        for (auto & dest : dest_list) {
            sum += this->insert_edge(dest, timestamp);
        }
        return sum;
    }

    // only for edge test use, should be moved
    template<typename EdgeEntry>
    uint64_t PMAIndex<EdgeEntry>::init_graph(std::vector<uint64_t> &dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
        std::sort(dest.begin() + start, dest.begin() + end);
        std::vector<uint64_t> unique;
        uint64_t prev = 0;
        for (uint64_t ptr = start; ptr < end; ptr++) {
            if (ptr == start || dest[ptr] != prev) unique.push_back(dest[ptr]);
            prev = dest[ptr];
        }
        uint64_t tmp_size = unique.size();
        uint64_t capacity = m_leaf->segment_size;
        if (tmp_size > m_leaf->segment_size) {
           capacity = (1 << (log2(tmp_size) + 1));
        }

        m_leaf->init_vars(capacity);
        m_leaf->impl.resize(capacity);
        m_leaf->sizes.resize(m_leaf->segment_num, 0);

        uint64_t tmp_idx = 0;
        uint64_t average_size = (tmp_size + m_leaf->segment_num - 1) / m_leaf->segment_num;
        for (uint64_t i = 0; i < m_leaf->segment_num; i++) {
            uint64_t j = 0;
            uint64_t remain_size = std::min(average_size, tmp_size - tmp_idx);
            for (j = 0; j < remain_size; j++) {
                uint64_t timestamp = exp_cfg.test_version_chain ? 0 : tmp_idx + start;
                m_leaf->impl[i * m_leaf->segment_size + j] = EdgeEntry{unique[tmp_idx], timestamp};
                tmp_idx++;
            }
            m_leaf->sizes[i] = j;
        }
        
        m_leaf->element_num = tmp_size;

        return unique.size();
    }
}