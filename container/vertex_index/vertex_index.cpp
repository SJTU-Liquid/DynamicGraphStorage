#include "utils/types.hpp"
#include "utils/config.hpp"
// For language identification.

//namespace container {
//    template<typename Derived>  // CRTP
//    struct VertexIndexWrapper {
//        bool lock(container::RequiredLock& lock) {
//            return static_cast<Derived*>(this)->lock(lock);
//        }
//
//        bool unlock(container::RequiredLock& lock) {
//            return static_cast<Derived*>(this)->unlock(lock);
//        }
//
//        bool has_vertex(uint64_t vertex) const{
//            return static_cast<Derived*>(this)->has_vertex(vertex);
//        }
//
//        void get_vertices(std::vector<uint64_t>& vertices) const{
//            static_cast<Derived*>(this)->get_vertices(vertices);
//        }
//
//        void get_neighbor_ptrs(std::vector<void *> &neighbor_ptrs) const{
//            static_cast<Derived*>(this)->get_neighbor_ptrs(neighbor_ptrs);
//        }
//
//        bool insert_vertex(uint64_t vertex, void* neighbor_ptr, uint64_t timestamp){
//            return static_cast<Derived*>(this)->insert_vertex(vertex, neighbor_ptr, timestamp);
//        }
//
//        void* get_neighbor_ptr(uint64_t vertex) const{
//            return static_cast<Derived*>(this)->get_neighbor_ptr(vertex);
//        }
//
//        void* get_entry(uint64_t vertex) const{
//            return static_cast<Derived*>(this)->get_entry(vertex);
//        }
//
//        void clear(){
//            static_cast<Derived*>(this)->clear();
//        }
//    };
//
//}
