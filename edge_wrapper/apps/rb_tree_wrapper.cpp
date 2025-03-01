#include "container/edge_index/rb_tree.hpp"
#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_wrapper/edge_driver.h"
#include "utils/types.hpp"

class RBTree_Wrapper {
private:
    void *m_rbtree_index;
    std::vector<container::VertexEntry> *m_vertex_table;
public:
    RBTree_Wrapper(uint64_t num_vertices) {
        m_rbtree_index = new container::RBTreeEdgeIndex();
        m_vertex_table = new std::vector<container::VertexEntry>(num_vertices);
        for (uint64_t i = 0; i < num_vertices; i++) {
            void* neighbor_ptr = static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index)->allocate_neighbor();
            (*m_vertex_table)[i].vertex = i;
            (*m_vertex_table)[i].neighbor_ptr = neighbor_ptr;
            (*m_vertex_table)[i].degree = new container::DegreeEntry(0, 0, nullptr);
        }
    }
    
    ~RBTree_Wrapper() {
        for (auto &vertex : *m_vertex_table) {
            static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index)->clear_neighbor(vertex.neighbor_ptr);
            delete vertex.degree;
        }
        m_vertex_table->clear();
        delete m_vertex_table;
        delete static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index);
    }

    void insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index)->insert_edge(&((*m_vertex_table)[src]), dest, timestamp);
    }

    bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        return static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index)->has_edge((*m_vertex_table)[src].neighbor_ptr, 0, dest, timestamp);
    }

    template<class F>
    uint64_t edges(uint64_t src, F&& callback, uint64_t timestamp) {
        auto ret = static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index)->edges(&((*m_vertex_table)[src]), callback, timestamp);
        return ret;
    }

    void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
        for (auto i = start; i < end; i++) {
            static_cast<container::RBTreeEdgeIndex*>(m_rbtree_index)->insert_edge(&((*m_vertex_table)[src]), dest[i], 0);
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
        auto wrapper = RBTree_Wrapper(m_num_vertices);
        exp_cfg.num_search = 10000;
        EdgeDriver<RBTree_Wrapper> d(wrapper, dest_list, exp_cfg.output_dir, m_num_vertices, exp_cfg, (1ull << size));
        d.execute(exp_cfg.workload_type);
    }
}


void execute_real_graph(std::vector<uint64_t> & dest_list, std::vector<int> & element_sizes, uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
    for (uint64_t size : element_sizes) {
        auto wrapper = RBTree_Wrapper(num_vertices);
        EdgeDriver<RBTree_Wrapper> d(wrapper, dest_list, exp_cfg.output_dir, num_vertices, exp_cfg, (1ull << size));
        d.execute_real_graph(exp_cfg.workload_type, exp_cfg.target_stream_type, exp_cfg.workload_dir);
    }
}

#include "edge_driver_main.h"