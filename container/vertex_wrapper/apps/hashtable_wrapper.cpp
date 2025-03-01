#include "vertex_wrapper.h"
#include "types/types.hpp"
#include "vertex_wrapper/vertex_driver.h"
#include "vertex_index/hashmap.hpp"
#include "utils/types.hpp"

template<typename T>
class DummyEdgeIndex {};

class DummyEdgeEntry {};

using EdgeEntry = DummyEdgeEntry;

template<typename T>
using EdgeIndexTemplate = DummyEdgeIndex<T>;

template<template<typename> class EdgeIndex, typename EdgeEntry>
using VertexEntryTemplate = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

using VertexIndex = container::HashmapVertexIndex<VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>;

void execute(uint64_t num_vertices, std::string output_dir, bool is_insert_task, double initial_graph_rate = 0.8, int seed = 0) {
    VertexIndex m_hash_index;
    VertexDriver d(m_hash_index, num_vertices, output_dir, initial_graph_rate, seed);
    d.execute(is_insert_task);
    m_hash_index.clear();
}

#include "vertex_driver_main.h"