#include "container/edge_index/log_block.hpp"
#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_driver.h"
#include "utils/types.hpp"

using EdgeEntry = container::LogEdgeEntry;

template<typename T>
using EdgeIndexTemplate = container::LogBlockEdgeIndex<T>;

using EdgeIndex = EdgeIndexTemplate<EdgeEntry>;

using VertexEntry = container::VertexEntry<EdgeIndexTemplate, EdgeEntry>;

class LogBlock_Wrapper {
private:
    uint64_t m_num_vertices;
    std::vector<VertexEntry> *m_vertex_table;

public:
    LogBlock_Wrapper(uint64_t num_vertices) {
        m_vertex_table = new std::vector<VertexEntry>(num_vertices);
        m_num_vertices = num_vertices;
        for (uint64_t i = 0; i < m_num_vertices; i++) {
            (*m_vertex_table)[i].vertex = i;
            (*m_vertex_table)[i].neighbor = new container::NeighborEntry<EdgeIndexTemplate, EdgeEntry>();
            (*m_vertex_table)[i].update_degree(0, 0);
        }
    }
    
    ~LogBlock_Wrapper() {
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
        auto neighbor_ptr = (*m_vertex_table)[src].neighbor->get_neighbor_ptr();
        neighbor_ptr->m_block->resize(0);
        int log_size = log2(end - start);
        delete neighbor_ptr->m_filter;
        neighbor_ptr->m_filter = new SimdBlockFilterFixed((1 << log_size));

        for (uint64_t i = start; i < end; i++) {
            uint64_t timestamp = exp_cfg.test_version_chain ? 0 : i;
            auto entry = EdgeEntry(dest[i], timestamp);

            neighbor_ptr->m_block->push_back(entry);
            neighbor_ptr->m_log_num++;
            neighbor_ptr->m_filter->Add(dest[i]);
        }

        if (exp_cfg.test_version_chain) {
            uint64_t m_size = end - start;
            std::vector<uint64_t> versioned_dest(m_size);
            std::vector<std::pair<uint64_t, uint64_t>> versioned_entry;
            std::mt19937 gen(exp_cfg.seed);
            std::iota(versioned_dest.begin(), versioned_dest.end(), start); 
            std::shuffle(versioned_dest.begin(), versioned_dest.end(), gen);

            for (uint64_t j = 0; j < m_size * exp_cfg.timestamp_rate; j++) {
                for (int k = 1; k <= exp_cfg.version_chain_length; k++) {
                    versioned_entry.push_back({dest[versioned_dest[j]], k});
                    (*neighbor_ptr->m_block)[versioned_dest[j] - start].update_version(1);
                }
            }
            std::shuffle(versioned_entry.begin(), versioned_entry.end(), gen);
            for (auto &pair : versioned_entry) {
                neighbor_ptr->m_block->push_back(EdgeEntry{pair.first, pair.second});
                neighbor_ptr->m_log_num += 1;
            }
        }
    }

    void init_real_graph(std::vector<operation> & stream) {
        std::vector<std::vector<uint64_t>> graph(m_num_vertices);
        std::vector<std::vector<uint64_t>> timestamp(m_num_vertices);
        for (uint64_t i = 0; i < stream.size(); i++) {
            auto edge = stream[i].e;
            graph[edge.source].push_back(edge.destination);
            timestamp[edge.source].push_back(i);
        }
        
        for (uint64_t i = 0; i < m_num_vertices; i++) {
            auto neighbor = (*m_vertex_table)[i].neighbor;
            int log_size = log2(graph[i].size());
            delete neighbor->get_neighbor_ptr()->m_filter;
            neighbor->get_neighbor_ptr()->m_filter = new SimdBlockFilterFixed<>((1 << (log_size - 1)));
            for (uint64_t j = 0; j < graph[i].size(); j++) {
                neighbor->get_neighbor_ptr()->m_block->push_back({graph[i][j], timestamp[i][j]});
                neighbor->get_neighbor_ptr()->m_filter->Add(graph[i][j]);
            }
            neighbor->get_neighbor_ptr()->m_log_num = graph[i].size();
        }
    }
    
};

void execute(EdgeDriverConfig exp_cfg) {
    auto wrapper = LogBlock_Wrapper(exp_cfg.num_of_vertices);
    EdgeDriver<LogBlock_Wrapper> d(wrapper, exp_cfg);
    d.execute();
}


void execute_real_graph(uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
    auto wrapper = LogBlock_Wrapper(num_vertices);
    EdgeDriver<LogBlock_Wrapper> d(wrapper, exp_cfg);
    d.execute_real_graph();
}

#include "edge_driver_main.h"