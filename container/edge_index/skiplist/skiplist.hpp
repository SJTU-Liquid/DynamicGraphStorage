#pragma once

#include "edge_block.hpp"

#include <cstdint>
#include <cstdlib>
#include <random>

namespace container {
    template<typename EdgeEntry>
    struct SkipListIterator;

    template<typename EdgeEntry>
    struct SkipList {
        EdgeBlock<EdgeEntry> *head;

        size_t block_size;
        static const float p;    
        static thread_local std::mt19937 level_generator;

        EdgeBlock<EdgeEntry>* find_block(EdgeBlock<EdgeEntry>* cur_block, uint64_t dest, EdgeBlock<EdgeEntry> *blocks[SKIP_LIST_LEVELS]);
        uint64_t get_height();

    public:
        explicit SkipList(uint64_t block_size);
        ~SkipList();
        bool insert_edge(uint64_t dest, uint64_t timestamp);
        bool has_edge(uint64_t dest, uint64_t timestamp);
        SkipListIterator<EdgeEntry> begin(uint64_t);
    };


    template<typename EdgeEntry>
    thread_local std::mt19937 SkipList<EdgeEntry>::level_generator;
    template<typename EdgeEntry>
    const float SkipList<EdgeEntry>::p = 0.5;

    template<typename EdgeEntry>
    SkipList<EdgeEntry>::SkipList(uint64_t sz) : block_size(sz){
        if ((block_size & (block_size - 1)) != 0) {
            throw std::runtime_error("Block size must be a power of 2");
        }
        head = new EdgeBlock<EdgeEntry>(block_size);
    }

    template<typename EdgeEntry>
    SkipList<EdgeEntry>::~SkipList() {
        auto cur = head;
        while (cur != nullptr) {
            auto next = cur->next_levels[0];
            delete cur;
            cur = next;
        }
        head = nullptr;
    }


    template<typename EdgeEntry>
    EdgeBlock<EdgeEntry>* SkipList<EdgeEntry>::find_block(EdgeBlock<EdgeEntry>* cur_block, uint64_t dest, EdgeBlock<EdgeEntry> *blocks[SKIP_LIST_LEVELS]) {
        for (int l = SKIP_LIST_LEVELS - 1; l >= 0; l--) {
            while (cur_block->next_levels[l] != nullptr && cur_block->next_levels[l]->max < dest
                    && cur_block->next_levels[l]->next_levels[0] != nullptr) {
                cur_block = cur_block->next_levels[l];
            }
            blocks[l] = cur_block;
        }

        return blocks[0]->next_levels[0] != nullptr && blocks[0]->max < dest ? blocks[0]->next_levels[0] : blocks[0];
    }


    template<typename EdgeEntry>
    uint64_t SkipList<EdgeEntry>::get_height() {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        auto level = 1;
        while (d(level_generator) < p && level < SKIP_LIST_LEVELS) {
            level += 1;
        }
        return level;
    }

    template<typename EdgeEntry>
    bool SkipList<EdgeEntry>::insert_edge(uint64_t dest, uint64_t timestamp) {
        EdgeBlock<EdgeEntry>* blocks_per_level[SKIP_LIST_LEVELS]; 
        auto block = find_block(head, dest, blocks_per_level);
    
        if (block->size == block_size) {
            EdgeBlock<EdgeEntry>* new_block = new EdgeBlock<EdgeEntry>(block_size);
            block->split(*new_block);

            new_block->next_levels[0] = block->next_levels[0];
            new_block->before = block;
            if (new_block->next_levels[0] != nullptr) {
                new_block->next_levels[0]->before = new_block;
            }
            block->next_levels[0] = new_block;

            auto height = get_height();
            for (uint l = 1; l < SKIP_LIST_LEVELS; l++) {
                if (l < height) {
                    if (blocks_per_level[l]->next_levels[l] != block) {
                        new_block->next_levels[l] = blocks_per_level[l]->next_levels[l];
                        blocks_per_level[l]->next_levels[l] = new_block;
                    } else {
                        new_block->next_levels[l] = block->next_levels[l];
                        block->next_levels[l] = new_block;
                    }
                } else {
                    new_block->next_levels[l] = nullptr;
                }
            }
            return insert_edge(dest, timestamp);
        } else {
            return block->insert_edge(dest, timestamp);
        }
    }


    template<typename EdgeEntry>
    bool SkipList<EdgeEntry>::has_edge(uint64_t dest, uint64_t timestamp) {
        if (head != nullptr) {
            EdgeBlock<EdgeEntry> *blocks[SKIP_LIST_LEVELS];
            auto block = find_block(head, dest, blocks);

            auto begin = block->impl.begin();
            auto end = block->impl.begin() + block->size;
            EdgeEntry value(dest);
            auto pos = std::lower_bound(begin, end, value);
            if (pos != end && pos->get_dest() == dest && pos->check_version(timestamp)) return true;

        } 
        return false;
    }

    template<typename EdgeEntry>
    SkipListIterator<EdgeEntry> SkipList<EdgeEntry>::begin(uint64_t timestamp) {
        SkipListIterator<EdgeEntry> it(head, timestamp);
        while(it.valid() && !it->check_version(timestamp)) it++;
        return it;
    }

    template<typename EdgeEntry>
    struct SkipListIterator {
        EdgeBlock<EdgeEntry>* block = nullptr;
        uint64_t index = 0;
        uint64_t timestamp;
    public:
        SkipListIterator(EdgeBlock<EdgeEntry> *b, uint64_t ts) : block(b), timestamp(ts), index(0) {}
        SkipListIterator(const SkipListIterator& other) {
            block = other.block;
            index = other.index;
            timestamp = other.timestamp;
        };

        SkipListIterator& operator=(const SkipListIterator& rhs) {
            if (this != &rhs) {
                block = rhs.block;
                index = rhs.index;
            }
            return *this;
        }

        SkipListIterator& operator++() {
            if (block != nullptr) {
                do {
                    if (index == block->size - 1) {
                        block = block->next_levels[0];
                        index = 0;
                    } else {
                        index++;
                    }
                } while (__builtin_expect(valid() && !block->impl[index].check_version(timestamp), 1));
            }
            return *this;
        }

        SkipListIterator operator++(int) {
            SkipListIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const SkipListIterator &rhs) {
            return this->block == rhs.block && this->index == rhs.index;
        }

        bool operator!=(const SkipListIterator &rhs) {
            return this->block != rhs.block || this->index != rhs.index;
        }

        EdgeEntry& operator*() {
            return block->impl[index];
        }

        EdgeEntry* operator->() {
            return &(block->impl[index]);
        }

        bool valid() {
            return (block != nullptr) && (index < block->size);
        }
    };
}
