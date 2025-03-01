#include "edge_index/pma.hpp"
#include "vertex_index/vector.hpp"

#include "apps/base_2pl.h"

using EdgeEntry = container::VersionedEdgeEntry;
template<typename T>
using EdgeIndexTemplate = container::PMAIndex<T>;

template<template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexEntryTemplate = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

template<typename VertexEntry, template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexIndexTemplate = container::VectorVertexIndex<VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>;

using vec2pma_container = container::Container<
    VertexIndexTemplate,
    VertexEntryTemplate,
    EdgeIndexTemplate,
    EdgeEntry
>;

class Vector2PMA : public WrapperBase<vec2pma_container> {
public:
    explicit Vector2PMA(bool is_directed = false, bool is_weighted = true)
        : WrapperBase<vec2pma_container>(is_directed, is_weighted) {}

    static std::string repl() {
        return std::string{"Vector_PMA_Wrapper"};
    }
};

namespace wrapper {
    void execute(const DriverConfig & config) {
       auto wrapper = Vector2PMA(false, true);
       Driver<Vector2PMA, std::shared_ptr<Vector2PMA::Snapshot>> d(wrapper, config);
       d.execute(config.workload_type, config.target_stream_type);
   }
}

#include "driver_main.h"