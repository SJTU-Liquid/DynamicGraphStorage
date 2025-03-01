#include "utils/types.hpp"

#include "skiplist.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <random>

using namespace container;

void large_scale_test() {
    const uint64_t block_size = 1024;  
    const uint64_t num_entries = 1000000; 
    SkipList<container::VersionEdgeEntry> skiplist(block_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dest_dist(1, num_entries);
    std::uniform_int_distribution<uint64_t> time_dist(1, num_entries);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < num_entries; ++i) {
        assert(skiplist.insert_edge(i, 0));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time taken to insert " << num_entries << " entries: " << duration << " ms" << std::endl;

    start_time = std::chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < num_entries / 10; ++i) {  
        assert(skiplist.has_edge(i, 0));
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time taken to query " << num_entries / 10 << " entries: " << duration << " ms" << std::endl;


    start_time = std::chrono::high_resolution_clock::now();

    auto it = skiplist.begin(0);
    uint64_t i = 0;
    while(it.valid()) {
        assert((it++)->dest == (i++));
    }
    assert(i == num_entries);

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time taken to query scan: " << duration << " ms" << std::endl;

    
}

int main() {
    try {
        large_scale_test();
        std::cout << "Large scale test passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
