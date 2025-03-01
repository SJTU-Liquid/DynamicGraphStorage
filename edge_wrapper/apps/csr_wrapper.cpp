// #include <cstdint>

// #include "edge_wrapper.h"
// #include "types/types.hpp"
// #include "edge_wrapper/edge_driver.h"
// #include "utils/types.hpp"

// class CSR_Wrapper {
// private:
//     std::vector<container::VertexEntry> *m_vertex_table;
//     uint64_t m_num_vertices;
// public:
//     CSR_Wrapper(uint64_t num_vertices) {
//         m_vertex_table = new std::vector<container::VertexEntry>(num_vertices);
//         m_num_vertices = num_vertices;
//         for (uint64_t i = 0; i < num_vertices; i++) {
//             std::vector<uint64_t> *neighbor = new std::vector<uint64_t>();
//             void* neighbor_ptr = static_cast<void*>(neighbor);
//             (*m_vertex_table)[i].vertex = i;
//             (*m_vertex_table)[i].neighbor_ptr = neighbor_ptr;
//             (*m_vertex_table)[i].degree = new container::DegreeEntry(0, 0, nullptr);
//         }
//     }
    
//     ~CSR_Wrapper() = default;

//     void insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
//         std::vector<uint64_t> *neighbor = static_cast<std::vector<uint64_t>*>((*m_vertex_table)[src].neighbor_ptr);
//         auto iter = std::lower_bound(neighbor->begin(), neighbor->end(), dest);
//         neighbor->insert(iter, dest);
//     }

//     bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
//         std::vector<uint64_t> *neighbor = static_cast<std::vector<uint64_t>*>((*m_vertex_table)[src].neighbor_ptr);
        
//         auto it = std::lower_bound(neighbor->begin(), neighbor->end(), dest);
//         if (it != neighbor->end() && *it == dest) return true; 
//         return false;
//     }

//     template<class F>
//     uint64_t edges(uint64_t src, F&& callback, uint64_t timestamp) {
//         uint64_t sum = 0;
//         std::vector<uint64_t> *neighbor = static_cast<std::vector<uint64_t>*>((*m_vertex_table)[src].neighbor_ptr);
//         // for (auto iter = neighbor->begin(); iter != neighbor->end(); iter++) {
//         //     callback(*iter, 0.0);
//         //     sum += *iter;
//         // }
//         uint64_t size = neighbor->size();
//         for (uint64_t i = 0; i < size; i++) {
//             uint64_t dest = (*neighbor)[i];
//             callback(dest, 0.0);
//             sum += dest;
//         }
//         return sum;
//     }

//     void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
//         std::sort(dest.begin() + start, dest.begin() + end);
//         std::vector<uint64_t> *neighbor = static_cast<std::vector<uint64_t>*>((*m_vertex_table)[src].neighbor_ptr);
//         neighbor->resize(end - start);
//         std::copy(dest.begin() + start, dest.begin() + end, neighbor->begin());
//     }

//     void init_real_graph(std::vector<operation> & stream) {
//         for (auto op : stream) {
//             std::vector<uint64_t> *neighbor = static_cast<std::vector<uint64_t>*>((*m_vertex_table)[op.e.source].neighbor_ptr);
//             neighbor->push_back(op.e.destination);
//         }
//         for (uint64_t i = 0; i < m_num_vertices; i++) {
//             std::vector<uint64_t> *neighbor = static_cast<std::vector<uint64_t>*>((*m_vertex_table)[i].neighbor_ptr);
//             std::sort(neighbor->begin(), neighbor->end());
//         }
//     }
// };

// void execute(std::vector<uint64_t> & dest_list, std::vector<int> & element_sizes, EdgeDriverConfig exp_cfg) {
//     for (auto & size : element_sizes) {
//         uint64_t max = container::config::BLOCK_SIZE / sizeof(container::EdgeEntry);
//         max = max >= (1ull << size) ? max : (1ull << size);
//         auto m_num_vertices = (container::config::ELEMENTS_NUM / max) >= (1ull << 20) ? (1ull << 20) : (container::config::ELEMENTS_NUM / max);
//         auto wrapper = CSR_Wrapper(m_num_vertices);
//         exp_cfg.num_search = 10000;
//         EdgeDriver<CSR_Wrapper> d(wrapper, dest_list, exp_cfg.output_dir, m_num_vertices, exp_cfg, (1ull << size));
//         d.execute(exp_cfg.workload_type);
//     }
// }


// void execute_real_graph(std::vector<uint64_t> & dest_list, std::vector<int> & element_sizes, uint64_t num_vertices, EdgeDriverConfig exp_cfg) {
//     for (uint64_t size : element_sizes) {
//         auto wrapper = CSR_Wrapper(num_vertices);
//         EdgeDriver<CSR_Wrapper> d(wrapper, dest_list, exp_cfg.output_dir, num_vertices, exp_cfg, (1ull << size));
//         d.execute_real_graph(exp_cfg.workload_type, exp_cfg.target_stream_type, exp_cfg.workload_dir);
//     }
// }

// #include "edge_driver_main.h"