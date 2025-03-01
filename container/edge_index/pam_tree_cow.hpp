#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>
#include <atomic>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_sort.h>

#include "utils/PAM/include/pam/pam.h"
#include "utils/PAM/include/pam/map.h"
#include "utils/PAM/include/pam/sequence_ops.h"
#include "utils/types.hpp"
#include "utils/config.hpp"

#define PRE_VEC_KEY 0

using PUU = std::pair<uint64_t, uint64_t>;

using PAMTree = pam_map<container::PAMEntry>;

using Join_Tree = typename weight_balanced_tree::template
       balance<basic_node<typename weight_balanced_tree::data, typename map_full_entry<container::PAMEntry>::entry_t>>;

using Seq_Tree = sequence_ops<Join_Tree>;

// DESCRIPTION:
// if the hyperparameter `b` is 2, the sequence is 1 2 3 5, then the block should be [1] in pre_vec, and 2 - [2, 3, 5], because
// the larger head does not appear. The 2 - [2, 3, 5] will be split into 2 parts if a head that is larger than 2 and smaller than 5 is inserted.
// e.g. If 4 is inserted, the block should be [1] in pre_vec, and 2 - [2, 3] and 4 - [4, 5].

// Changes
// 1. add a pre_vec to store the edges that are smaller than the smallest key in the tree.
// 2. add find_approx() to find_approx the largest key that is smaller than the target key.
// 3. change the implementation of the functions.
namespace container {
    auto binary_search(std::vector<uint64_t>& neighbor, uint64_t dest) {
        auto it = std::lower_bound(neighbor.begin(), neighbor.end(), dest);
        if (it != neighbor.end() && *it == dest) return &(*it);
        return static_cast<uint64_t *>(nullptr);
    }

    void insert_sorted(std::vector<uint64_t>& vec, uint64_t value) {
        auto pos = std::lower_bound(vec.begin(), vec.end(), value);
        if (pos == vec.end() || *pos != value) {
            vec.insert(pos, value);
        }
    }

    struct PAMStruct {
        pam_map<container::PAMEntry> tree;
        uint64_t degree;
        uint64_t vertex_id;
        std::vector<uint64_t> head_list;

        explicit PAMStruct(uint64_t id = 0, bool init_plus = true): degree(0), vertex_id(id) {
            if (init_plus) tree.insert({PRE_VEC_KEY, {}});
        }

        PAMStruct& operator=(const PAMStruct &other) {
            if (this != &other) { 
                tree.clear();
                tree = other.tree;
                degree = other.degree;
                vertex_id = other.vertex_id;
                head_list = other.head_list;
            }
            return *this;
        }

        PAMStruct& operator=(PAMStruct&& other) noexcept {
            if (this != &other) {
                tree.clear();  
                tree = std::move(other.tree); 
                degree = other.degree; 
                vertex_id = other.vertex_id;  
                head_list = std::move(other.head_list);  
            }
            return *this;
        }

        PAMStruct(PAMStruct &&other) 
            : tree(std::move(other.tree)), degree(other.degree), vertex_id(other.vertex_id),
                head_list(std::move(other.head_list)) {}

        PAMStruct(const PAMStruct& other)
            : tree(other.tree), degree(other.degree), vertex_id(other.vertex_id), 
                head_list(other.head_list) {}

        ~PAMStruct() {
            tree.clear();
        }

        // PAMStruct(const PAMStruct &other) = delete;

        struct PAMTreeIterator {
            Seq_Tree::node_iterator iter;
            std::vector<uint64_t>::iterator inner_iter;

            PAMTreeIterator(Seq_Tree::node_iterator pos) : iter(pos) {
                if (iter != PAMTree::node_end()) {
                    inner_iter = ((*iter).second).begin();
                    if (inner_iter == (*iter).second.end()) {
                        ++iter;
                        inner_iter = ((*iter).second).begin();
                    }
                }
            }
            
            bool is_valid() {
                return iter != PAMTree::node_end() && inner_iter != ((*iter).second).end();
            }

            PAMTreeIterator& operator++() {
                if (is_valid()) {
                    ++inner_iter;
                    if (inner_iter == ((*iter).second).end()) {
                        ++iter;
                        if (is_valid()) inner_iter = ((*iter).second).begin();
                    }
                }
                return (*this);
            }

            uint64_t operator*() {
                if (is_valid()) {
                    return *inner_iter;
                } else {
                    throw std::runtime_error("use of invalid iterator, PAMIterator::operator*");
                }
            }
        };

        bool has_edge(uint64_t src, uint64_t dest) {
            auto vec_ptr = tree.find_approx(get_lowest_key(dest));

            if (vec_ptr.has_value()) {
                return (binary_search(vec_ptr.value(), dest) != nullptr);
            }
            return false;
        }
        
        // need to make sure dest list is sorted!!!
        PAMStruct&& insert_edge_batch(const std::vector<uint64_t>& dest_list, uint64_t start, uint64_t end) { // add lvalue for newly added
            for (uint64_t i = start; i < end; i++) {
                auto dest = dest_list[i];
                if (check_is_head(dest)) {
                    insert_sorted(head_list, dest);
                    bool flag = false;
                    insert_edge(dest, flag); // TODO: optimize this non atomic bug
                }
            }
            std::vector<std::pair<uint64_t, std::vector<uint64_t>>> tree_constructor;
            tree_constructor.reserve(head_list.size() + 1); // the number of headers counting in pre_vec

            uint64_t cur_ind = 0;

            size_t estimated_group_size = 2 * (end - start) / (head_list.size() + 1);
            std::vector<uint64_t> first_vec;
            first_vec.reserve(estimated_group_size);
            tree_constructor.emplace_back(PRE_VEC_KEY, std::move(first_vec));

            auto iter = head_list.begin();
            for (uint64_t i = start; i < end; i++) {
                auto dest = dest_list[i];
                if (iter != head_list.end() && dest >= *iter) {
                    while (iter + 1 != head_list.end() && dest >= *(iter + 1)) iter++; // fix this ugly code
                    std::vector<uint64_t> new_vec;
                    new_vec.reserve(estimated_group_size);
                    new_vec.push_back(dest);

                    tree_constructor.emplace_back(get_lowest_key(*iter), std::move(new_vec));
                    iter++;
                    cur_ind++;
                } 
                else tree_constructor[cur_ind].second.push_back(dest);
            }

            pam_map<container::PAMEntry> batch;
            batch = batch.multi_insert_sorted(std::move(batch), std::move(tree_constructor));
            uint64_t sum = 0;

            auto op = [&sum] (const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
                auto iter_a = a.begin();
                auto iter_b = b.begin();
                auto end_a = a.end();
                auto end_b = b.end();
                std::vector<uint64_t> result;
                result.reserve(a.size() + b.size());

                while (iter_a != end_a && iter_b != end_b) {
                    auto val_a = *iter_a;
                    auto val_b = *iter_b;
                    if (val_a < val_b) {
                        result.push_back(val_a);
                        iter_a++;
                    }
                    else if (val_a > *iter_b) {
                        result.push_back(*iter_b);
                        sum++;
                        iter_b++;
                    }
                    else {
                        result.push_back(val_a);
                        iter_a++;
                        iter_b++;
                    }
                }

                while (iter_a != end_a) {
                    result.push_back(*iter_a);
                    iter_a++;
                }
                while (iter_b != end_b) {
                    result.push_back(*iter_b);
                    iter_b++;
                    sum++;
                }
                return result;
            };

            if ((end - start) < degree) {
                auto new_tree = tree.map_union(std::move(batch), std::move(tree), std::move(op));
                tree = new_tree;
            } else {
                auto new_tree = tree.map_union(std::move(tree), std::move(batch), std::move(op));
                tree = new_tree;
            }
            
            degree += sum;
            return std::move(*this);
        }


        // need to make sure dest list is sorted!!!
        // TODO: add is_sorted parameter
        PAMStruct&& insert_edge_batch(const std::vector<PUU>& dest_list, uint64_t start, uint64_t end, uint64_t &edges_added) { 
            uint64_t dest = 0;
            std::atomic<uint64_t> sum = 0;
            uint64_t head_sum = 0;
            for (uint64_t i = start; i < end; i++) {
                dest = dest_list[i].second;
                if (check_is_head(dest)) {
                    insert_sorted(head_list, dest);
                    bool flag = false;
                    insert_edge(dest, flag); // TODO: optimize this non atomic bug
                    if (flag) {
                        sum++;
                        head_sum++;
                    }
                }
            }
            std::vector<std::pair<uint64_t, std::vector<uint64_t>>> tree_constructor;
            tree_constructor.reserve(head_list.size() + 1); // the number of headers counting in pre_vec

            uint64_t cur_ind = 0;

            size_t estimated_group_size = 2 * (end - start) / (head_list.size() + 1);
            tree_constructor.emplace_back(PRE_VEC_KEY, std::vector<uint64_t>());
            tree_constructor.back().second.reserve(estimated_group_size);

            auto iter = head_list.begin();
            for (uint64_t i = start; i < end; i++) {
                dest = dest_list[i].second;
                if (iter != head_list.end() && dest >= *iter) {
                    while (iter + 1 != head_list.end() && dest >= *(iter + 1)) iter++; // fix this ugly code
                    tree_constructor.emplace_back(
                        get_lowest_key(*iter),
                        std::vector<uint64_t>{dest}
                    );
                    tree_constructor.back().second.reserve(estimated_group_size);
                    iter++;
                    cur_ind++;
                } 
                else tree_constructor[cur_ind].second.push_back(dest);
            }

            pam_map<container::PAMEntry> batch;
            batch = batch.multi_insert_sorted(std::move(batch), tree_constructor);

            auto op = [&sum] (const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
                std::vector<uint64_t> result;
                result.reserve(a.size() + b.size());

                if (a.size() < b.size()) {
                    std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
                } else {
                    std::set_union(b.begin(), b.end(), a.begin(), a.end(), std::back_inserter(result));
                }

                sum.fetch_add(result.size() - a.size());
                return std::move(result);
            };

            if ((end - start) < degree) {
                auto new_tree = tree.map_union(std::move(batch), std::move(tree), std::move(op));
                tree = new_tree;
            } else {
                auto new_tree = tree.map_union(std::move(tree), std::move(batch), std::move(op));
                tree = new_tree;
            }
            
            degree = degree + sum - head_sum; // fix this ugly code
            edges_added = sum;
            return std::move(*this);
        }

        uint64_t intersect(PAMStruct &other) {
            // uint64_t sum = 0;

            // auto neighbor_a = pam_tree;
            // auto neighbor_b = other.pam_tree;

            // auto edge_map_func = [&] (PAMTree::E& l) -> void {
            //     sum += l.second->size();
            // }; 

            // {
            //     auto iter_a = neighbor_a->pre_vec->begin();
            //     auto iter_b = neighbor_b->pre_vec->begin();
            //     while (iter_a != neighbor_a->pre_vec->end() && iter_b != neighbor_b->pre_vec->end()) {
            //         if (*iter_a < *iter_b) iter_a++;
            //         else if (*iter_a > *iter_b) iter_b++;
            //         else {
            //             sum++;
            //             iter_a++;
            //             iter_b++;
            //         }
            //     }
            // }

            // // auto res_tree_tree = neighbor_a->tree.map_intersect(*neighbor_a->tree, *neighbor_b->tree);
            // // PAMTree::foreach_seq(res_tree_tree, edge_map_func);
            // auto block_iter_a = PAMTree::node_begin(*neighbor_a->tree);
            // auto block_iter_b = PAMTree::node_begin(*neighbor_b->tree);
            // if (block_iter_a != PAMTree::node_end() && block_iter_b != PAMTree::node_end()) {
            //     auto inner_iter_a = ((*block_iter_a).second)->begin();
            //     auto inner_iter_b = ((*block_iter_b).second)->begin();
            //     while (1) {
            //         auto dest_a = *inner_iter_a;
            //         auto dest_b = *inner_iter_b;
            //         if (dest_a < dest_b) {
            //             inner_iter_a++;
            //             if (inner_iter_a == ((*block_iter_a).second)->end()) {
            //                 ++block_iter_a;
            //                 if (block_iter_a == PAMTree::node_end()) break;
            //                 inner_iter_a = ((*block_iter_a).second)->begin();
            //             }
            //         } else if (dest_a > dest_b) {
            //             inner_iter_b++;
            //             if (inner_iter_b == ((*block_iter_b).second)->end()) {
            //                 ++block_iter_b;
            //                 if (block_iter_b == PAMTree::node_end()) break;
            //                 inner_iter_b = ((*block_iter_b).second)->begin();
            //             }
            //         } else {
            //             sum++;
            //             inner_iter_a++;
            //             if (inner_iter_a == ((*block_iter_a).second)->end()) {
            //                 ++block_iter_a;
            //                 if (block_iter_a == PAMTree::node_end()) break;
            //                 inner_iter_a = ((*block_iter_a).second)->begin();
            //             }
            //             inner_iter_b++;
            //             if (inner_iter_b == ((*block_iter_b).second)->end()) {
            //                 ++block_iter_b;
            //                 if (block_iter_b == PAMTree::node_end()) break;
            //                 inner_iter_b = ((*block_iter_b).second)->begin();
            //             }
            //         }
            //     }
            // }

            // if (neighbor_a->pre_vec->empty() && neighbor_b->pre_vec->empty()) return sum;
            // if (!neighbor_a->pre_vec->empty()) {
            //     auto iter_a = neighbor_a->pre_vec->begin();
            //     auto vec_tree_func = [&] (PAMTree::E& l) -> void {
            //         auto iter_b = l.second->begin();
            //         while (iter_a != neighbor_a->pre_vec->end() && iter_b != l.second->end()) {
            //             if (*iter_a < *iter_b) iter_a++;
            //             else if (*iter_a > *iter_b) iter_b++;
            //             else {
            //                 sum++;
            //                 iter_a++;
            //                 iter_b++;
            //             }
            //         }
            //     }; 
            //     PAMTree::foreach_seq(*neighbor_b->tree, vec_tree_func);
            // }

            // if (!neighbor_b->pre_vec->empty()) {
            //     auto iter_b = neighbor_b->pre_vec->begin();
            //     auto tree_vec_func = [&] (PAMTree::E& l) -> void {
            //         auto iter_a = l.second->begin();
            //         while (iter_b != neighbor_b->pre_vec->end() && iter_a != l.second->end()) {
            //             if (*iter_a < *iter_b) iter_a++;
            //             else if (*iter_a > *iter_b) iter_b++;
            //             else {
            //                 sum++;
            //                 iter_a++;
            //                 iter_b++;
            //             }
            //         }
            //     }; 
            //     PAMTree::foreach_seq(*neighbor_a->tree, tree_vec_func);
            // }

            return 0;
            // TODO: intersect
        }

        PAMStruct&& insert_edge(uint64_t dest, bool & flag) {
            // find_approx the position
            auto vec_res = tree.find_approx(get_lowest_key(dest));
            auto update_func = [&] (std::vector<uint64_t> old_vec, std::vector<uint64_t> new_vec) {
                return new_vec;
            };

            assert(vec_res.has_value());
            auto vec_ptr = vec_res.value();
            auto pos = std::lower_bound(vec_ptr.begin(), vec_ptr.end(), dest);
            if (pos != vec_ptr.end() && *pos == dest) {
                flag = false;
                return std::move(*this);
            }

            if ((vec_ptr.size() != 0) && check_is_head(vec_ptr[0])) {
                if(check_is_head(dest)) {
                    insert_sorted(head_list, dest);

                    // split
                    auto new_size = vec_ptr.end() - pos;
                    auto remain_size = pos - vec_ptr.begin();
                    auto new_vec = std::vector<uint64_t>();
                    auto remain_vec = std::vector<uint64_t>();
                    new_vec.resize(new_size + 1);
                    remain_vec.resize(remain_size);

                    new_vec[0] = dest;
                    std::copy(pos, vec_ptr.end(), new_vec.begin() + 1);
                    std::copy(vec_ptr.begin(), pos, remain_vec.begin());

                    tree.insert({get_lowest_key(dest), {}});

                    std::vector<std::pair<uint64_t, std::vector<uint64_t>>> insert_vec = { {get_lowest_key(dest), new_vec}, {get_lowest_key(remain_vec[0]), remain_vec}};
                    tree = tree.multi_update(tree, insert_vec, update_func);
                } else {
                    auto single_update_func = [&] (std::pair<uint64_t, std::vector<uint64_t>> old_vec) {
                        auto pos = std::lower_bound(old_vec.second.begin(), old_vec.second.end(), dest);
                        old_vec.second.insert(pos, dest);
                        return old_vec.second;
                    };
                   tree.update(get_lowest_key(vec_ptr[0]), single_update_func);
                }
            } else {
                if(check_is_head(dest)) {
                    // check if there exists elements in pre_vec
                    insert_sorted(head_list, dest);
                    if(vec_ptr.size() != 0) {
                        // split
                        auto new_size = vec_ptr.end() - pos;
                        auto remain_size = pos - vec_ptr.begin();
                        auto new_vec = std::vector<uint64_t>();
                        auto remain_vec = std::vector<uint64_t>();
                        new_vec.resize(new_size + 1);
                        remain_vec.resize(remain_size);
                        new_vec[0] = dest;
                        std::copy(pos, vec_ptr.end(), new_vec.begin() + 1);
                        std::copy(vec_ptr.begin(), pos, remain_vec.begin());
                        auto ins_vec = std::vector<uint64_t>();
                        tree.insert({get_lowest_key(dest), ins_vec});

                        std::vector<std::pair<uint64_t, std::vector<uint64_t>>> insert_vec = { {get_lowest_key(dest), new_vec}, {PRE_VEC_KEY, remain_vec}};
                        tree = tree.multi_update(tree, insert_vec, update_func);
                    } else {
                        // insert a new vec
                        auto new_vec = std::vector<uint64_t>();
                        new_vec.push_back(dest);
                        tree.insert({get_lowest_key(dest), new_vec});
                    }
                } else {
                    // add to pre_vec
                    if(vec_ptr.size() == 0) {
                        auto new_vec = std::vector<uint64_t>{dest};
                        tree.insert({PRE_VEC_KEY, new_vec});
                    } else {
                        // grow
                        if (pos != vec_ptr.end() && *pos == dest) {
                            flag = false;
                            return std::move(*this);
                        }

                        auto single_update_func = [&] (std::pair<uint64_t, std::vector<uint64_t>> old_vec) {
                            auto pos = std::lower_bound(old_vec.second.begin(), old_vec.second.end(), dest);
                            old_vec.second.insert(pos, dest);
                            return old_vec.second;
                        };
                        tree.update(PRE_VEC_KEY, single_update_func);
                    }
                }
            }
            degree++;
            flag = true;
            return std::move(*this);
        }

        uint64_t get_degree() {
            return degree;
        }

        template<typename F>
        uint64_t edges(F&& callback) {
            uint64_t sum = 0;
            bool flag = true;
            uint64_t cnt = 0;
            auto edge_map_func = [&] (PAMTree::E& l) -> void {
                for (size_t idx = 0; idx < l.second.size(); idx++) {
                    if (!flag) return;
                    auto dest = l.second[idx];
                    bool should_continue = callback(dest, 0.0);
                    sum += dest;
                    cnt++;
                    if (!should_continue) {
                        flag = false;
                        return;
                    }
                }
            };
            
            PAMTree::foreach_seq(tree, edge_map_func);
            return sum;
        }

        // void get_neighbor(void* vertex_ptr, std::vector<uint64_t>& neighbor) {
        //     edges(vertex_ptr, [&neighbor](uint64_t dest, double weight) {
        //         neighbor.push_back(dest);
        //     }, timestamp);
        // }

        bool clear_neighbor() {
            return true;
        }

        void clear() {
        }

        PAMTreeIterator get_begin() {
            return PAMTreeIterator(PAMTree::node_begin(tree));
        }
    };

}
