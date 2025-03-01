#include "container/edge_index/pam_tree_cow.hpp"
#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_wrapper/edge_driver.h"
#include "utils/types.hpp"

using EdgeEntry = uint64_t;

template<typename T>
using EdgeIndexTemplate = container::PAMStruct;

using EdgeIndex = EdgeIndexTemplate<EdgeEntry>;

class PAM_Wrapper {
private:
    std::vector<container::PAMStruct>* m_vertex_table;

public:
    PAM_Wrapper(uint64_t m_num_vertices) {
        m_vertex_table = new std::vector<container::PAMStruct>(m_num_vertices);
    }
    
    ~PAM_Wrapper() {
        for (auto &vertex : *m_vertex_table) {
            vertex.clear();
        }
        m_vertex_table->clear();
        delete m_vertex_table;
    }

    void insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        bool flag = false;
        (*m_vertex_table)[src].insert_edge(dest, flag);
    }

    bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        return (*m_vertex_table)[src].has_edge(src, dest);
    }

    template<class F>
    uint64_t edges(uint64_t src, F&& callback, uint64_t timestamp) {
        return (*m_vertex_table)[src].edges(callback);
    }

    void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
        // for (uint64_t i = start; i < end; i++) {
        //     insert_edge(src, dest[i], i);
        // }
        std::sort(dest.begin() + start, dest.begin() + end);
        uint64_t batch_end = start, batch_size = 1000000;
        
        for (uint64_t batch_start = start; batch_start < end; batch_start = batch_end) {
            batch_end = std::min(batch_start + batch_size, end);
            (*m_vertex_table)[src].insert_edge_batch(dest, batch_start, batch_end);
        }
    }

    void init_real_graph(std::vector<operation> & stream) {
        for (uint64_t i = 0; i < stream.size(); i++) {
            auto op = stream[i];
            insert_edge(op.e.source, op.e.destination, i);
        }
    }
};

void execute(EdgeDriverConfig exp_cfg) {
    auto wrapper = PAM_Wrapper(exp_cfg.num_of_vertices);
    EdgeDriver<PAM_Wrapper> d(wrapper, exp_cfg);
    d.execute();
}


void execute_real_graph(uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
    auto wrapper = PAM_Wrapper(num_vertices);
    EdgeDriver<PAM_Wrapper> d(wrapper, exp_cfg);
    d.execute_real_graph();
}

#include "edge_driver_main.h"