#pragma once

#include <limits>
#include <cstdint>

namespace container::config {
    const uint64_t VERTEX_INDEX_LOCK_IDX = std::numeric_limits<uint64_t>::max();
    
    const double VECTOR_GROWTH_RATE = 1.0;

    static size_t BLOCK_SIZE = BLOCK_SIZE_VALUE;     // 4KB, set this to control block size of index except for PMA

    static size_t DEFAULT_VECTOR_SIZE = DEFAULT_VECTOR_SIZE_VALUE;  // In entry num;

    static uint64_t MEMORY_SIZE = 1ull << 33;
    static uint64_t ELEMENTS_NUM = MEMORY_SIZE / 8;
}