/**
 * From Jan Bötcher's hyParallel repository.
 */
#pragma once

#include <cstdint>
#include <atomic>
#include <cassert>
#include <sched.h>
#include <xmmintrin.h>
#include <ctime>

class RWSpinLock {
    using DATATYPE = uint64_t;
    static constexpr uint64_t excl = 1ull << 63;
    static const DATATYPE LOCK = 0x8000000000000000;

public:
    static const char* name;
    std::atomic<DATATYPE> data;

    /// Constructor
    RWSpinLock() : data{0} {}
    RWSpinLock(const RWSpinLock&) = delete;
    RWSpinLock operator=(const RWSpinLock) = delete;

    bool try_lock_shared() {
        uint64_t before = data.load();
        before = before & ~excl; // before must not be exclusive to succeed
        assert(before < 100);
        return data.compare_exchange_weak(before, before + 1);
    }

    bool tryUpgrade() {
        uint64_t lockedByUs = 1;
        return data.compare_exchange_weak(lockedByUs, excl);
    }

    bool tryLock() {
        uint64_t notLocked = 0;
        return data.compare_exchange_weak(notLocked, excl);
    }

    void unlockExcl() {
        assert(data.load() == excl);
        data.store(0);
    }
    void unlock_shared() {
        assert(data.load());
        --data;
    }

    void lock_shared() {
        unsigned restartCount = 0;
        while (!try_lock_shared()) {
            yield(++restartCount);
        }
    }

    /// Force unlock (independent of current lock state)
    inline void forceUnlock() {
        data.store(0);
    }
    /// Unlock
    inline void unlock() {
        unlockExcl();
    }

    /// Init the lock
    inline void init() {
        data = 0;
    }

    /// Yield (for spin locks)
    static void yield(unsigned k) {
        if (k < 4) {
        } else if (k < 16) {
            _mm_pause();
        } else if (k < 32 || k & 1) {
            sched_yield();
        } else {
            struct timespec rqtp = {0, 0};
            rqtp.tv_sec = 0;
            rqtp.tv_nsec = 1000;
            nanosleep(&rqtp, 0);
        }
    }


public:
    bool isLocked() {
        return data.load() == excl;
    }

    /// Lock
    inline void lock() {
        for (unsigned k = 0; !tryLock(); ++k) {
            yield(k);
        }
    }
};