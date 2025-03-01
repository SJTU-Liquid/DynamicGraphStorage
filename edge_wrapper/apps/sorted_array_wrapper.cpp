#include <cstdint>

#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_driver.h"
#include "container/edge_index/sorted_array.hpp"
#include "utils/types.hpp"

using EdgeEntry = container::VersionedEdgeEntry;

template<typename T>
using EdgeIndexTemplate = container::SortedArrayEdgeIndex<T>;

using EdgeIndex = EdgeIndexTemplate<EdgeEntry>;

using VertexEntry = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

class SortedArrayWrapper {
private:
    std::vector<VertexEntry> *m_vertex_table;
    uint64_t m_num_vertices;
public:
    SortedArrayWrapper(uint64_t num_vertices) {
        m_vertex_table = new std::vector<VertexEntry>(num_vertices);
        m_num_vertices = num_vertices;
        for (uint64_t i = 0; i < num_vertices; i++) {
            (*m_vertex_table)[i].vertex = i;
            (*m_vertex_table)[i].neighbor = new container::NeighborEntry<EdgeIndexTemplate, EdgeEntry>();
            (*m_vertex_table)[i].update_degree(0, 0);
        }
    }
    
    ~SortedArrayWrapper() {
        for (auto &vertex : *m_vertex_table) {
            vertex.clear();
        }
        m_vertex_table->clear();
        delete m_vertex_table;
    }
    
    void insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        (*m_vertex_table)[src].neighbor->insert_edge(dest, timestamp);
    }

    bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        return (*m_vertex_table)[src].neighbor->has_edge(src, dest, timestamp);
    }

    template<class F>
    uint64_t edges(uint64_t src, F&& callback, uint64_t timestamp) {
        return (*m_vertex_table)[src].neighbor->edges(callback, timestamp);
    }

    void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
        (*m_vertex_table)[src].neighbor->get_neighbor_ptr()->init_graph(dest, start, end, exp_cfg);

        if (exp_cfg.test_version_chain) {
            auto m_size = end - start;
            std::mt19937 gen(exp_cfg.seed);
            std::vector<int> versioned_dest(m_size);
            std::iota(versioned_dest.begin(), versioned_dest.end(), start); 
            std::shuffle(versioned_dest.begin(), versioned_dest.end(), gen);

            for (uint64_t j = 0; j < m_size * exp_cfg.timestamp_rate; j++) {
                for (int k = 1; k <= exp_cfg.version_chain_length; k++) {
                    insert_edge(src, dest[versioned_dest[j]], k);
                }
            }
        }
    }

    void init_real_graph(std::vector<operation> & stream) {
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> graph(m_num_vertices);
        for (uint64_t i = 0; i < stream.size(); i++) {
            auto edge = stream[i].e;
            graph[edge.source].push_back({edge.destination, i});
        }
        for (uint64_t i = 0; i < m_num_vertices; i++) {
            std::sort(graph[i].begin(), graph[i].end(), [](const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b) {
                return a.first < b.first;
            });
            auto neighbor = (*m_vertex_table)[i].neighbor->get_neighbor_ptr()->m_arr;
            for (uint64_t j = 0; j < graph[i].size(); j++) {
                EdgeEntry entry(graph[i][j].first, graph[i][j].second);
                neighbor->push_back(std::move(entry));
            }
        }
    }
};

void execute(EdgeDriverConfig exp_cfg) {
    auto wrapper = SortedArrayWrapper(exp_cfg.num_of_vertices);
    EdgeDriver<SortedArrayWrapper> d(wrapper, exp_cfg);
    d.execute();
}


void execute_real_graph(uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
    auto wrapper = SortedArrayWrapper(num_vertices);
    EdgeDriver<SortedArrayWrapper> d(wrapper, exp_cfg);
    d.execute_real_graph();
}

#include "edge_driver_main.h"