#pragma once

#include <cstdint>
#include <memory>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <forward_list>
#include <immintrin.h>
#include "../rwlock.hpp"

namespace container {
#ifdef ENABLE_TIMESTAMP
    struct VersionEdgeEntry {
        uint64_t dest;
        std::forward_list<uint64_t>* version_chain;

        VersionEdgeEntry() : dest(std::numeric_limits<uint64_t>::max()), version_chain(nullptr) {}

        explicit VersionEdgeEntry(uint64_t dest) : dest(dest), version_chain(nullptr)  {}

        VersionEdgeEntry(uint64_t dest, uint64_t timestamp) : dest(dest) {
            version_chain = new std::forward_list<uint64_t>();
            version_chain->push_front(timestamp);
        }

        ~VersionEdgeEntry() {
            if(version_chain) delete version_chain;
            version_chain = nullptr;
        }

        // delete copy Ctor
        VersionEdgeEntry(const VersionEdgeEntry &) = delete;
        VersionEdgeEntry &operator=(const VersionEdgeEntry &) = delete;

        // enable move Ctor
        VersionEdgeEntry(VersionEdgeEntry &&) = default;
        VersionEdgeEntry &operator=(VersionEdgeEntry &&) = default; 

        bool operator<(const VersionEdgeEntry &rhs) const {
            return dest < rhs.dest;
        }

        bool operator<(uint64_t cmp_dest) const {
            return this->dest < cmp_dest;
        }

        void update_version(uint64_t timestamp) {
            if (!version_chain) version_chain = new std::forward_list<uint64_t>();
            version_chain->push_front(timestamp);
        }

        void clear_version() {
            if (version_chain) {
                delete version_chain;
                version_chain = nullptr;
            }
        }

        inline bool check_version(uint64_t timestamp) {
            auto it = version_chain->begin();

            if (__builtin_expect(*it <= timestamp, 1)) return true;

            for (++it; it != version_chain->end(); ++it) {
                if (*it <= timestamp) return true;
            }

            return false;
        }
        
        void get_versions(std::vector<uint64_t>* ret_ptr) {
            std::copy(version_chain->begin(), version_chain->end(), std::back_inserter(*ret_ptr));
        }

        uint64_t get_dest() const {
            return dest;
        }


        void gc(uint64_t timestamp) {
            if (!version_chain) return;

            auto it = version_chain->begin();
            auto prev = version_chain->before_begin();
            uint64_t cnt = 0;

            while (it != version_chain->end()) {
                if (*it < timestamp && cnt) {
                    version_chain->erase_after(prev);
                    break;
                } else {
                    prev = it;
                    ++it;
                    cnt++;
                }
            }
        }
    };

#else
    struct VersionEdgeEntry {
        uint64_t dest;

        VersionEdgeEntry() : dest(std::numeric_limits<uint64_t>::max()) {}

        explicit VersionEdgeEntry(uint64_t dest) : dest(dest) {}

        VersionEdgeEntry(uint64_t dest, uint64_t timestamp) : dest(dest) {}

        // delete copy Ctor
        VersionEdgeEntry(const VersionEdgeEntry &) = delete;
        VersionEdgeEntry &operator=(const VersionEdgeEntry &) = delete;

        // enable move Ctor
        VersionEdgeEntry(VersionEdgeEntry &&) = default;
        VersionEdgeEntry &operator=(VersionEdgeEntry &&) = default;

        bool operator<(const VersionEdgeEntry &rhs) const {
            return dest < rhs.dest;
        }

        bool operator<(uint64_t cmp_dest) const {
            return this->dest < cmp_dest;
        }

        void update_version(uint64_t timestamp) {}

        void clear_version() {}

        bool check_version(uint64_t timestamp) {
            return true;
        }

        void get_versions(std::vector<uint64_t>* ret_ptr) {
            ret_ptr->push_back(0);
        }

        uint64_t get_dest() const {
            return dest;
        }

        void gc(uint64_t timestamp) {}
    };
#endif


#ifdef ENABLE_TIMESTAMP
    struct LogEdgeEntry {
        uint64_t dest;
        uint64_t b_timestamp;   // visible from
        uint64_t e_timestamp;   // invisible from

        LogEdgeEntry() : dest(std::numeric_limits<uint64_t>::max()), b_timestamp(std::numeric_limits<uint64_t>::max()),
                         e_timestamp(std::numeric_limits<uint64_t>::max()) {} // invalid
        LogEdgeEntry(uint64_t dest, uint64_t b_timestamp) : dest(dest), b_timestamp(b_timestamp),
                                                            e_timestamp(std::numeric_limits<uint64_t>::max()) {}
        LogEdgeEntry(uint64_t dest, uint64_t b_timestamp, uint64_t e_timestamp) : dest(dest), b_timestamp(b_timestamp), e_timestamp(e_timestamp) {}

        bool check_version(uint64_t timestamp) {
            return (b_timestamp <= timestamp) && (e_timestamp > timestamp);
        }

        bool check_is_newest(uint64_t timestamp) {
            return (b_timestamp <= timestamp) && (e_timestamp == std::numeric_limits<uint64_t>::max());
        }

        void update_version(uint64_t timestamp) {
            e_timestamp = timestamp;
        }

        uint64_t get_dest() const {
            return dest;
        }

        void gc(uint64_t timestamp) {}

    };
#else
    struct LogEdgeEntry {
        uint64_t dest;
        
        LogEdgeEntry() : dest(std::numeric_limits<uint64_t>::max()) {}
        explicit LogEdgeEntry(uint64_t dest) : dest(dest) {}
        LogEdgeEntry(uint64_t dest, uint64_t timestamp) : dest(dest) {}


        bool check_version(uint64_t timestamp) {
            return true;
        }

        bool check_is_newest(uint64_t timestamp) {
            return true;
        }

        void update_version(uint64_t timestamp) {}

        uint64_t get_dest() const {
            return dest;
        }

        void gc(uint64_t timestamp) {}
    };
#endif

#ifdef ENABLE_TIMESTAMP
    struct VersionedEdgeEntry {
        uint64_t dest;
        uint64_t timestamp;
        std::unique_ptr<VersionedEdgeEntry> next;

        VersionedEdgeEntry() : dest(std::numeric_limits<uint64_t>::max()), timestamp(0), next(nullptr) {}

        explicit VersionedEdgeEntry(uint64_t dest) : dest(dest), timestamp(0), next(nullptr)  {}

        VersionedEdgeEntry(uint64_t dest, uint64_t ts) : dest(dest), timestamp(ts), next(nullptr) {}

        // delete copy Ctor
        VersionedEdgeEntry(const VersionedEdgeEntry &) = delete;
        VersionedEdgeEntry &operator=(const VersionedEdgeEntry &) = delete;

        // enable move Ctor
        VersionedEdgeEntry(VersionedEdgeEntry &&) = default;
        VersionedEdgeEntry &operator=(VersionedEdgeEntry &&) = default;

        bool operator<(const VersionedEdgeEntry &rhs) const {
            return dest < rhs.dest;
        }

        bool operator<(uint64_t cmp_dest) const {
            return this->dest < cmp_dest;
        }


        void update_version(uint64_t ts) {
            auto newVersion = std::make_unique<VersionedEdgeEntry>(this->dest, this->timestamp);
            newVersion->next = std::move(this->next);
            this->timestamp = ts;
            this->next = std::move(newVersion);
        }

        void clear_version() {
            if (next) {
                next->clear_version();  
            }
            next.reset();
        }

        inline bool check_version(uint64_t ts) {
            if (__builtin_expect(this->timestamp <= ts, 1)) return true;

            auto entry = this;
            entry = entry->next.get();

            while (entry != nullptr) {
                if (__builtin_expect(entry->timestamp <= ts, 1)) return true;
                entry = entry->next.get();
            }
            return false;
        }
        
        void get_versions(std::vector<uint64_t>* ret_ptr) {
            auto entry = this;

            while (entry != nullptr) {
                ret_ptr->push_back(entry->timestamp);
                entry = entry->next.get();
            }
        }

        uint64_t get_dest() const {
            return dest;
        }

        void gc(uint64_t timestamp) {
            VersionedEdgeEntry* current = this;
            while (current && current->next != nullptr) {
                if (current->next->timestamp < timestamp) {
                    auto to_delete = std::move(current->next);
                    current->next = std::move(to_delete->next);
                } else {
                    current = current->next.get();
                }
            }
        }
    };

#else
    struct VersionedEdgeEntry {
        uint64_t dest;

        VersionedEdgeEntry() : dest(std::numeric_limits<uint64_t>::max()) {}

        explicit VersionedEdgeEntry(uint64_t dest) : dest(dest) {}

        VersionedEdgeEntry(uint64_t dest, uint64_t timestamp) : dest(dest) {}

        // delete copy Ctor
        VersionedEdgeEntry(const VersionedEdgeEntry &) = delete;
        VersionedEdgeEntry &operator=(const VersionedEdgeEntry &) = delete;

        // enable move Ctor
        VersionedEdgeEntry(VersionedEdgeEntry &&) = default;
        VersionedEdgeEntry &operator=(VersionedEdgeEntry &&) = default;

        bool operator<(const VersionedEdgeEntry &rhs) const {
            return dest < rhs.dest;
        }

        bool operator<(uint64_t cmp_dest) const {
            return this->dest < cmp_dest;
        }

        void update_version(uint64_t timestamp) {}

        void clear_version() {}

        bool check_version(uint64_t timestamp) {
            return true;
        }

        void get_versions(std::vector<uint64_t>* ret_ptr) {
            ret_ptr->push_back(0);
        }

        uint64_t get_dest() const {
            return dest;
        }

        void gc(uint64_t timestamp) {}
    };

#endif

    struct PAMEntry {
        using key_t = uint64_t;
        using val_t = std::vector<uint64_t>;
        static bool comp(key_t a, key_t b) {
            return a < b;
        }
    };
} // namespace container