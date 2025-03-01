#include "container/edge_index/pam_tree_cow.hpp"
#include "container/utils/PAM/include/pam/pam.h"
#include "container/utils/unordered_dense/include/ankerl/unordered_dense.h"

#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_driver.h"
#include "utils/types.hpp"

using EdgeEntry = uint64_t;

template<typename T>
using EdgeIndexTemplate = container::PAMStruct;

using EdgeIndex = EdgeIndexTemplate<EdgeEntry>;

class NeoPam_Wrapper {
private:
    pam_map<container::COWVertexEntry<EdgeIndexTemplate, EdgeEntry>, avl_tree> m_vertex_table;

public:
    NeoPam_Wrapper(uint64_t m_num_vertices) {
        for (uint64_t i = 0; i < m_num_vertices; i++) {
            m_vertex_table.insert({i, std::move(EdgeIndex())});
        }
    }
    

    void insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        bool flag = false;
        auto update_func = [&] (std::pair<uint64_t, container::PAMStruct>& entry) {
            return std::move(entry.second.insert_edge(dest, flag));
        };
        m_vertex_table.update(src, update_func);
    }

    bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
        auto res = m_vertex_table.find(src);
        if (!res.has_value()) false;
        return res.value().has_edge(src, dest);
    }

    template<class F>
    uint64_t edges(uint64_t src, F&& callback, uint64_t timestamp) {
        auto res = m_vertex_table.find(src);
        if (!res.has_value()) return 0;
        auto ret = res.value().edges(callback);
        return ret;
    }

    void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
        for (auto i = start; i < end; i++) {
            insert_edge(src, dest[i], 0);
        }
    }

    void init_real_graph(std::vector<operation> & stream) {
        for (auto op : stream) {
            insert_edge(op.e.source, op.e.destination, 0);
        }
    }
};

void execute(EdgeDriverConfig exp_cfg) {
    auto wrapper = NeoPam_Wrapper(exp_cfg.num_of_vertices);
    EdgeDriver<NeoPam_Wrapper> d(wrapper, exp_cfg);
    d.execute();
}


void execute_real_graph(uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
    auto wrapper = NeoPam_Wrapper(num_vertices);
    EdgeDriver<NeoPam_Wrapper> d(wrapper, exp_cfg);
    d.execute_real_graph();
}

#include "edge_driver_main.h"