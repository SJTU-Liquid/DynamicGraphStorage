#include "edge_index/log_block.hpp"
#include "vertex_index/vector.hpp"

#include "apps/base_2pl.h"

using EdgeEntry = container::LogEdgeEntry;

template<typename T>
using EdgeIndexTemplate = container::LogBlockEdgeIndex<T>;

template<template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexEntryTemplate = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

template<typename VertexEntry, template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexIndexTemplate = container::VectorVertexIndex<VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>;

using vector2logblock = container::Container<
    VertexIndexTemplate,
    VertexEntryTemplate,
    EdgeIndexTemplate,
    EdgeEntry
>;

class Vector2Logblock : public WrapperBase<vector2logblock> {
public:
    explicit Vector2Logblock(bool is_directed = false, bool is_weighted = true)
        : WrapperBase<vector2logblock>(is_directed, is_weighted) {}

    static std::string repl() {
        return std::string{"Vector_LogBlock_Wrapper"};
    }
};

namespace wrapper {
    void execute(const DriverConfig & config) {
       auto wrapper = Vector2Logblock(false, true);
       Driver<Vector2Logblock, std::shared_ptr<Vector2Logblock::Snapshot>> d(wrapper, config);
       d.execute(config.workload_type, config.target_stream_type);
   }
}

#include "driver_main.h"