#include "edge_index/sorted_array.hpp"
#include "vertex_index/vector.hpp"

#include "apps/base_2pl.h"

using EdgeEntry = container::VersionedEdgeEntry;
template<typename T>
using EdgeIndexTemplate = container::SortedArrayEdgeIndex<T>;

template<template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexEntryTemplate = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

template<typename VertexEntry, template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexIndexTemplate = container::VectorVertexIndex<VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>;

using vec2sorted_array_container = container::Container<
    VertexIndexTemplate,
    VertexEntryTemplate,
    EdgeIndexTemplate,
    EdgeEntry
>;

class Vector2Sorted_array : public WrapperBase<vec2sorted_array_container> {
public:
    explicit Vector2Sorted_array(bool is_directed = false, bool is_weighted = true)
        : WrapperBase<vec2sorted_array_container>(is_directed, is_weighted) {}

    static std::string repl() {
        return std::string{"Vector_Sorted_Array_Wrapper"};
    }
};

namespace wrapper {
    void execute(const DriverConfig & config) {
       auto wrapper = Vector2Sorted_array(false, true);
       Driver<Vector2Sorted_array, std::shared_ptr<Vector2Sorted_array::Snapshot>> d(wrapper, config);
       d.execute(config.workload_type, config.target_stream_type);
   }
}

#include "driver_main.h"