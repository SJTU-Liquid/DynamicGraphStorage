#include "edge_index/pam_tree.hpp"
#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_wrapper/edge_driver.h"
#include "utils/types.hpp"

class VersionedPAM_Wrapper {
private:
    void *m_pam_index;
    std::vector<container::VertexEntry> *m_vertex_table;

public:
    VersionedPAM_Wrapper(uint64_t m_num_vertices) {
        m_pam_index = new container::VersionedPAMIndex();
        m_vertex_table = new std::vector<container::VertexEntry>(m_num_vertices);
        for (uint64_t i = 0; i < m_num_vertices; i++) {
            void* neighbor_ptr = static_cast<container::VersionedPAMIndex*>(m_pam_index)->allocate_neighbor();
            (*m_vertex_table)[i].vertex = i;
            (*m_vertex_table)[i].neighbor_ptr = neighbor_ptr;
            (*m_vertex_table)[i].degree = new container::DegreeEntry(0, 0, nullptr);
        }
    }
    
    ~VersionedPAM_Wrapper() {
        for (auto &vertex : *m_vertex_table) {
            static_cast<container::VersionedPAMIndex*>(m_pam_index)->clear_neighbor(vertex.neighbor_ptr);
            delete vertex.degree;
        }
        m_vertex_table->clear();
        delete m_vertex_table;
        delete static_cast<container::VersionedPAMIndex*>(m_pam_index);
    }

    void insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        static_cast<container::VersionedPAMIndex*>(m_pam_index)->insert_edge(&((*m_vertex_table)[src]), dest, timestamp);
    }

    bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        return static_cast<container::VersionedPAMIndex*>(m_pam_index)->has_edge((*m_vertex_table)[src].neighbor_ptr, 0, dest, timestamp);
    }

    template<class F>
    uint64_t edges(uint64_t src, F&& callback, uint64_t timestamp) {
        auto ret = static_cast<container::VersionedPAMIndex*>(m_pam_index)->edges((*m_vertex_table)[src].neighbor_ptr, callback, timestamp);
        return ret;
    }

    void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
        for (auto i = start; i < end; i++) {
            static_cast<container::VersionedPAMIndex*>(m_pam_index)->insert_edge(&((*m_vertex_table)[src]), dest[i], 0);
        }

        auto m_size = end - start;
        std::mt19937 gen(exp_cfg.seed);
        std::vector<int> versioned_dest(m_size);
        std::iota(versioned_dest.begin(), versioned_dest.end(), start); 
        std::shuffle(versioned_dest.begin(), versioned_dest.end(), gen);

        for (uint64_t j = 0; j < m_size * exp_cfg.timestamp_rate; j++) {
            insert_edge(src, dest[versioned_dest[j]], 1);
            insert_edge(src, dest[versioned_dest[j]], 2);
        }
    }

    void init_real_graph(std::vector<operation> & stream) {
        for (auto op : stream) {
            insert_edge(op.e.source, op.e.destination, 0);
        }
    }
};

void execute(std::vector<uint64_t> & dest_list, std::vector<int> & element_sizes, EdgeDriverConfig exp_cfg) {
    for (auto & size : element_sizes) {
        uint64_t max = container::config::BLOCK_SIZE;
        max = max >= (1ull << size) ? max : (1ull << size);
        auto m_num_vertices = (container::config::ELEMENTS_NUM / max) >= (1ull << 20) ? (1ull << 20) : (container::config::ELEMENTS_NUM / max);
        auto wrapper = VersionedPAM_Wrapper(m_num_vertices);
        exp_cfg.num_search = 10000;
        EdgeDriver<VersionedPAM_Wrapper> d(wrapper, dest_list, exp_cfg.output_dir, m_num_vertices, exp_cfg, (1ull << size));
        d.execute(exp_cfg.workload_type);
    }
}


void execute_real_graph(std::vector<uint64_t> & dest_list, std::vector<int> & element_sizes, uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
    for (uint64_t size : element_sizes) {
        auto wrapper = VersionedPAM_Wrapper(num_vertices);
        EdgeDriver<VersionedPAM_Wrapper> d(wrapper, dest_list, exp_cfg.output_dir, num_vertices, exp_cfg, (1ull << size));
        d.execute_real_graph(exp_cfg.workload_type, exp_cfg.target_stream_type, exp_cfg.workload_dir);
    }
}
#include "edge_driver_main.h"