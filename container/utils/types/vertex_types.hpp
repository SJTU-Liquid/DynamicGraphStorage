#pragma once

#include <cstdint>
#include <memory>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <forward_list>
#include <immintrin.h>
#include "../rwlock.hpp"
#include "edge_types.hpp"

namespace container {

template<template<typename> class EdgeIndex, typename EdgeEntry>
struct NeighborEntry;

#ifdef ENABLE_TIMESTAMP
    template<template<typename> class EdgeIndex, typename EdgeEntry>
    struct VertexEntry {
        uint64_t vertex{};
        NeighborEntry<EdgeIndex, EdgeEntry>* neighbor;
        std::forward_list<std::pair<uint64_t, uint64_t>>* degree;
        // void *neighbor_ptr{};

#ifdef ENABLE_LOCK
        std::unique_ptr<RWSpinLock> spinlock{};
#endif

        VertexEntry() : vertex(std::numeric_limits<uint64_t>::max())
        {
                degree = new std::forward_list<std::pair<uint64_t, uint64_t>>();
#ifdef ENABLE_LOCK
                spinlock = std::make_unique<RWSpinLock>();
#endif
        }

        explicit VertexEntry(uint64_t vertex, uint64_t timestamp) 
            : vertex(vertex)
        {
            degree = new std::forward_list<std::pair<uint64_t, uint64_t>>();
            degree->push_front({0, timestamp});
#ifdef ENABLE_LOCK
                spinlock = std::make_unique<RWSpinLock>();
#endif
        }

        explicit VertexEntry(uint64_t vertex, uint64_t timestamp, NeighborEntry<EdgeIndex, EdgeEntry>* neighbor_ptr) 
            : vertex(vertex), neighbor(neighbor_ptr)
        {
            degree = new std::forward_list<std::pair<uint64_t, uint64_t>>();
            degree->push_front({0, timestamp});
#ifdef ENABLE_LOCK
                spinlock = std::make_unique<RWSpinLock>();
#endif
        }

        uint64_t get_degree(uint64_t timestamp) const {   
            for (const auto& val : *degree) {
                if (val.second <= timestamp) return val.first;
            }
            throw std::runtime_error("Invalid timestamp from container::VertexEntry::get_degree(uint64_t timestamp)\n");
        }

        void update_degree(uint64_t new_degree, uint64_t timestamp) {
            if (!degree) degree = new std::forward_list<std::pair<uint64_t, uint64_t>>();
            degree->push_front({new_degree, timestamp});
        }

        void gc(uint64_t timestamp) {
            if (!degree) return;

            auto it = degree->begin();
            auto prev = degree->before_begin();
            uint64_t cnt = 0;

            while (it != degree->end()) {
                if (it->second < timestamp && cnt) {
                    degree->erase_after(prev);
                    break;
                } else {
                    prev = it;
                    ++it;
                    cnt++;
                }
            }
        }

        void clear_degree() {
            if(degree) {
                delete degree;
                degree = nullptr;
            }
        }

        void clear() {
            clear_degree();
            if (neighbor) {
                neighbor->clear();
                delete neighbor;
                neighbor = nullptr;
            }
        }

        bool operator==(const VertexEntry &other) const {
            return vertex == other.vertex;
        }

        bool operator<(const VertexEntry &other) const {
            return vertex < other.vertex;
        }

#ifdef ENABLE_LOCK
        void lock() {
            spinlock->lock();
        }

        void unlock() {
            spinlock->unlock();
        }

        void lock_shared() {
            spinlock->lock_shared();
        }

        void unlock_shared() {
            spinlock->unlock_shared();
        }

        RWSpinLock* get_lock() {
            return spinlock.get();
        }

#else
        void lock() {}

        void unlock() {}

        void lock_shared() {}

        void unlock_shared() {}
#endif
    };
#else
    template<template<typename> class EdgeIndex, typename EdgeEntry>
    struct VertexEntry {
        uint64_t vertex{};
        uint64_t degree;
        // void *neighbor_ptr{};
        NeighborEntry<EdgeIndex, EdgeEntry>* neighbor;

#ifdef ENABLE_LOCK
        std::unique_ptr<RWSpinLock> spinlock{};
#endif

        VertexEntry() : vertex(std::numeric_limits<uint64_t>::max()), degree(0)
        {
#ifdef ENABLE_LOCK
                spinlock = std::make_unique<RWSpinLock>();
#endif
        }

        explicit VertexEntry(uint64_t vertex, uint64_t timestamp, NeighborEntry<EdgeIndex, EdgeEntry>* neighbor_ptr) 
            : vertex(vertex), degree(0), neighbor(neighbor_ptr)
        {
#ifdef ENABLE_LOCK
                spinlock = std::make_unique<RWSpinLock>();
#endif
        }

        uint64_t get_degree(uint64_t timestamp) const {   
            return degree;
        }
        
        void gc(uint64_t timestamp) {

        }

        void update_degree(uint64_t new_degree, uint64_t timestamp) {
            degree = new_degree;
        }

        void clear_degree() {}

        void clear() {
            if (neighbor) {
                neighbor->clear();
                delete neighbor;
                neighbor = nullptr;
            }
        }

        bool operator==(const VertexEntry &other) const {
            return vertex == other.vertex;
        }

        bool operator<(const VertexEntry &other) const {
            return vertex < other.vertex;
        }

#ifdef ENABLE_LOCK
        void lock() {
            spinlock->lock();
        }

        void unlock() {
            spinlock->unlock();
        }

        void lock_shared() {
            spinlock->lock_shared();
        }

        void unlock_shared() {
            spinlock->unlock_shared();
        }
#else
        void lock() {}

        void unlock() {}

        void lock_shared() {}

        void unlock_shared() {}
#endif
    };
#endif

    struct RequiredLock {
        uint64_t idx;
        bool is_exclusive;
    };


    template<template<typename> class EdgeIndex, typename EdgeEntry>
    struct PAMVertexEntry {
        using key_t = uint64_t;
        using val_t = EdgeIndex<EdgeEntry>*;
        static bool comp(key_t a, key_t b) {
            return a < b;}
    };


    template<template<typename> class EdgeIndex, typename EdgeEntry>
    struct COWVertexEntry {
        using key_t = uint64_t;
        using val_t = EdgeIndex<EdgeEntry>;
        static bool comp(key_t a, key_t b) {
            return a < b;}
    };

    uint64_t get_lowest_key(uint64_t key) {
        return std::hash<uint64_t>{}(key) / container::config::BLOCK_SIZE + 1;
    }

    bool check_is_head(uint64_t key) {
        return std::hash<uint64_t>{}(key) % container::config::BLOCK_SIZE == 0;
    }


    enum NeighborType {
        Vector,
        NonVector,
    };

    template <typename EdgeEntry>
    struct IIterator {
        virtual ~IIterator() = default;
        virtual bool valid() = 0;
        virtual IIterator& operator++() = 0;
        virtual uint64_t operator*() = 0;
        virtual EdgeEntry* operator->() = 0;
    };

    template <typename IteratorType, typename EdgeEntry>
    class IteratorImpl : public IIterator<EdgeEntry> {
        IteratorType iterator;

    public:
        explicit IteratorImpl(IteratorType it) : iterator(it) {}

        bool valid() override {
            return iterator.valid(); 
        }

        IIterator<EdgeEntry>& operator++() override {
            ++iterator;
            return *this;
        }

        uint64_t operator*() override {
            return iterator->get_dest(); 
        }

        EdgeEntry* operator->() override {
            return iterator.operator->();
        }
    };

    template<typename EdgeEntry>
    class VectorIteratorImpl {
        using VecIterator = typename std::vector<EdgeEntry>::iterator;
        VecIterator iterator;
        VecIterator end;
        uint64_t timestamp;

    public:
        VectorIteratorImpl(VecIterator begin, VecIterator end, uint64_t ts)
            : iterator(begin), end(end), timestamp(ts) {}

        VectorIteratorImpl(const VectorIteratorImpl &other)
            : iterator(other.iterator), end(other.end), timestamp(other.timestamp) {}

        bool valid() const {
            return iterator != end;
        }

        VectorIteratorImpl& operator++() {
            if (valid()) {
                do {
                    ++iterator;
                } while (valid() && !iterator->check_version(timestamp));
            } 
            return *this;
        }


        EdgeEntry* operator->() {
            return &(*iterator);
        }

        uint64_t operator*() const {
            return iterator->get_dest();
        }

    };

    template <typename EdgeEntry>
    class UnifiedIterator {
        std::unique_ptr<IIterator<EdgeEntry>> iterator_impl;

#ifdef ENABLE_LOCK
        RWSpinLock* lock_ptr;
        bool read_only;
#endif

    public:
#ifdef ENABLE_LOCK
        template <typename IteratorType>
        UnifiedIterator(IteratorType it, RWSpinLock *lock, bool read_only = true) 
            : iterator_impl(std::make_unique<IteratorImpl<IteratorType, EdgeEntry>>(it))
            , lock_ptr(lock), read_only(read_only) {}
        ~UnifiedIterator() {
            if(read_only) lock_ptr->unlock_shared();
            else lock_ptr->unlock();
        }

        UnifiedIterator(UnifiedIterator&& other) noexcept
            : iterator_impl(std::move(other.iterator_impl)), lock_ptr(other.lock_ptr) {
                other.lock_ptr = nullptr;
            }
#else
        template <typename IteratorType>
        explicit UnifiedIterator(IteratorType it) 
            : iterator_impl(std::make_unique<IteratorImpl<IteratorType, EdgeEntry>>(it)) {}
#endif
        bool is_valid() const {
            return iterator_impl->valid();
        }

        UnifiedIterator& operator++() {
            ++(*iterator_impl);
            return *this;
        }

        uint64_t operator*() const {
            return **iterator_impl;
        }

        EdgeEntry* operator->() const {
            return iterator_impl->operator->(); 
        }
    };


#ifdef ENABLE_ADAPTIVE
    template<template<typename> class EdgeIndex, typename EdgeEntry>
    struct NeighborEntry {
        NeighborType type;
        std::vector<EdgeEntry>* vector_ptr;
        EdgeIndex<EdgeEntry>* neighbor_ptr;

        NeighborEntry(): type(Vector) {
            vector_ptr = new std::vector<EdgeEntry>{};
            neighbor_ptr = nullptr;
        }

        NeighborEntry(NeighborEntry &other) = delete;

        ~NeighborEntry() {
            clear();
        }

        bool insert_edge(uint64_t dest, uint64_t timestamp) {
            if (type == Vector) {
                if (vector_ptr->size() < container::config::DEFAULT_VECTOR_SIZE) {
                    auto pos = std::lower_bound(vector_ptr->begin(), vector_ptr->end(), dest, [] (const EdgeEntry &entry, uint64_t dest) {
                        return entry.get_dest() < dest;
                    });
                    if (pos != vector_ptr->end() && pos->get_dest() == dest) {
                        pos->update_version(timestamp);
                        return false;
                    } else {
                        EdgeEntry entry{dest, timestamp};
                        vector_ptr->insert(pos, std::move(entry));
                        return true;
                    }
                } else {
                    neighbor_ptr = new EdgeIndex<EdgeEntry>();
                    for (auto & entry : *vector_ptr) {
                        std::vector<uint64_t> versions;
                        entry.get_versions(&versions);
                        for (int i = versions.size() - 1; i >= 0; i--) {
                            neighbor_ptr->insert_edge(entry.get_dest(), versions[i]);
                        }
                    }
                    delete vector_ptr;
                    vector_ptr = nullptr;
                    type = NonVector;
                    return neighbor_ptr->insert_edge(dest, timestamp);
                }
            } else {
                return neighbor_ptr->insert_edge(dest, timestamp);
            }
        }

        uint64_t insert_edge_batch(const std::vector<uint64_t> &dest_list, uint64_t timestamp) {
            uint64_t sum = 0;
            for (auto & dest : dest_list) {
                sum += this->insert_edge(dest, timestamp);
            }
            return sum;
        }

        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) const {
            if (type == Vector) {
                auto pos = std::lower_bound(vector_ptr->begin(), vector_ptr->end(), dest, [] (const EdgeEntry &entry, uint64_t dest) {
                    return entry.get_dest() < dest;
                });
                if (pos != vector_ptr->end() && pos->get_dest() == dest) {
                    return pos->check_version(timestamp);
                }
                return false;
            } else return neighbor_ptr->has_edge(src, dest, timestamp);
        }

        template<typename F>
        uint64_t edges(F&& callback, uint64_t timestamp) const {
            if (type == Vector) {
                uint64_t sum = 0;
                for (auto &entry : *vector_ptr) {
                    if (entry.check_version(timestamp)) {
                        bool should_continue = callback(entry.get_dest(), 0.0); // NOTE: Do not support weight now
                        sum += entry.dest;
                        if (!should_continue) return sum;
                    }
                }
                return sum;
            } else return neighbor_ptr->edges(callback, timestamp);
        }
        
        uint64_t intersect(const NeighborEntry<EdgeIndex, EdgeEntry> & other, uint64_t timestamp) {
            return 0;
        }

        void clear() {
            if (type == Vector && vector_ptr != nullptr) delete vector_ptr;
            else if (type == NonVector && neighbor_ptr != nullptr) delete neighbor_ptr;
            vector_ptr = nullptr;
            neighbor_ptr = nullptr;
        }

        EdgeIndex<EdgeEntry>* get_neighbor_ptr() {
            return neighbor_ptr;
        }
#ifdef ENABLE_LOCK
        UnifiedIterator<EdgeEntry> get_begin(uint64_t timestamp, RWSpinLock *lock, bool read_only = true) const {
            if (type == Vector) {
                return UnifiedIterator<EdgeEntry>(VectorIteratorImpl<EdgeEntry>{vector_ptr->begin(), vector_ptr->end(), timestamp}, lock, read_only);
            } else {
                return UnifiedIterator<EdgeEntry>(neighbor_ptr->get_begin(timestamp), lock, read_only);
            }
        }
#else
        UnifiedIterator<EdgeEntry> get_begin(uint64_t timestamp) const {
            if (type == Vector) {
                return UnifiedIterator<EdgeEntry>(VectorIteratorImpl<EdgeEntry>{vector_ptr->begin(), vector_ptr->end(), timestamp});
            } else {
                return UnifiedIterator<EdgeEntry>(neighbor_ptr->get_begin(timestamp));
            }
        }
#endif
    };
#else
    template<template<typename> class EdgeIndex, typename EdgeEntry>
    struct NeighborEntry {
        EdgeIndex<EdgeEntry>* neighbor_ptr;

        NeighborEntry() {
            neighbor_ptr = new EdgeIndex<EdgeEntry>();
        }

        NeighborEntry(NeighborEntry &other) = delete;

        ~NeighborEntry() {
            if (neighbor_ptr != nullptr) delete neighbor_ptr;
        }

        bool insert_edge(uint64_t dest, uint64_t timestamp) {
            return neighbor_ptr->insert_edge(dest, timestamp);
        }

        uint64_t insert_edge_batch(const std::vector<uint64_t> &dest_list, uint64_t timestamp) {
            return neighbor_ptr->insert_edge_batch(dest_list, timestamp);
        }

        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) const {
            return neighbor_ptr->has_edge(src, dest, timestamp);
        }

        template<typename F>
        uint64_t edges(F&& callback, uint64_t timestamp) const {
            return neighbor_ptr->edges(callback, timestamp);
        }
        
        uint64_t intersect(const NeighborEntry<EdgeIndex, EdgeEntry> & other, uint64_t timestamp) {
            return 0;
        }

        void clear() {
            if (neighbor_ptr != nullptr) delete neighbor_ptr;
            neighbor_ptr = nullptr;
        }

        EdgeIndex<EdgeEntry>* get_neighbor_ptr() {
            return neighbor_ptr;
        }

#ifdef ENABLE_LOCK
        UnifiedIterator<EdgeEntry> get_begin(uint64_t timestamp, RWSpinLock* lock, bool read_only = true) const {
            return UnifiedIterator<EdgeEntry>(neighbor_ptr->get_begin(timestamp), lock, read_only);
        }
#else
        UnifiedIterator<EdgeEntry> get_begin(uint64_t timestamp) const {
            return UnifiedIterator<EdgeEntry>(neighbor_ptr->get_begin(timestamp));
        }
#endif
    };
#endif
}