#include "edge_index/skiplist.hpp"
#include "vertex_index/vector.hpp"

#include "apps/base_2pl.h"

using EdgeEntry = container::VersionedEdgeEntry;

template<typename T>
using EdgeIndexTemplate = container::SkipListEdgeIndex<T>;

template<template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexEntryTemplate = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

template<typename VertexEntry, template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexIndexTemplate = container::VectorVertexIndex<VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>;

using vector2skiplist = container::Container<
    VertexIndexTemplate,
    VertexEntryTemplate,
    EdgeIndexTemplate,
    EdgeEntry
>;

class Vector2SkipList : public WrapperBase<vector2skiplist> {
public:
    explicit Vector2SkipList(bool is_directed = false, bool is_weighted = true)
        : WrapperBase<vector2skiplist>(is_directed, is_weighted) {}

    static std::string repl() {
        return std::string{"Vector_SkipList_Wrapper"};
    }
};

namespace wrapper {
    void execute(const DriverConfig & config) {
       auto wrapper = Vector2SkipList(false, true);
       Driver<Vector2SkipList, std::shared_ptr<Vector2SkipList::Snapshot>> d(wrapper, config);
       d.execute(config.workload_type, config.target_stream_type);
   }
}

#include "driver_main.h"