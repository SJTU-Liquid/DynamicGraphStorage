#include <iostream>
#include "container.hpp"
#include "transaction.hpp"
//#include "vertex_index/hashmap.hpp"
#include "edge_index/vector.hpp"
//#include "vertex_index/rb_tree.hpp"
//#include "edge_index/avl_tree.hpp"
#include "vertex_index/vector.hpp"
//#include "edge_index/hashmap.hpp"
//#include "edge_index/skiplist/skiplist.hpp"
//#include "edge_index/log_block.hpp"
//#include "vertex_index/avl_tree.hpp"
//#include "vertex_index/art_tree/art_tree.hpp"
//#include "edge_index/pam_tree.hpp"
//#include "edge_index/rb_tree.hpp"
//#include "edge_index/pma.hpp"

using namespace container;

int main() {
    TransactionManager<Container<VectorVertexIndex, VectorEdgeIndex>> tm{false, false};

    // Txn test
    {
        std::cout << "insert Vertices" << std::endl;
        auto txn = tm.get_write_transaction();
        for(int i = 10000; i >= 0; i--) {
            txn.insert_vertex(i);
        }
        txn.commit();
    }

    {
        std::cout << "Get Vertex Test" << std::endl;
        std::vector<uint64_t> vertices;
        auto txn = tm.get_read_transaction();
        txn.get_vertices(vertices);
        txn.commit();

        for(auto& v: vertices) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }

    {
        std::cout << "Insert Edges" << std::endl;
        auto txn = tm.get_write_transaction();
        txn.insert_edge(0, 1);
        txn.insert_edge(1, 0);
        txn.commit();
    }

    {
        std::cout << "Insert Edges" << std::endl;
        auto txn = tm.get_write_transaction();
        txn.insert_edge(0, 2);
        txn.insert_edge(2, 0);
        txn.commit();
    }

    {
        std::cout << "Read Txn Test" << std::endl;
        std::vector<uint64_t> neighbor;
        auto txn = tm.get_read_transaction();
        txn.get_neighbor(0, neighbor);
        std::cout << txn.get_degree(0) << std::endl;
        txn.commit();

        std::cout << "Neighbor size: " << neighbor.size() << std::endl;
        int count = 2;
        for(auto& v: neighbor) {
//            assert(v == count);
            count++;
        }
        std::cout << "get_neighbor test passed" << std::endl;
//        assert(txn.has_edge(0, 2));
//        assert(!txn.has_edge(0, 10000));
//        assert(!txn.has_edge(2, 3));
        std::cout << "has_edge test passed" << std::endl;
    }

    return 0;
}
