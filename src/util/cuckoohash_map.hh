#ifndef _CUCKOOHASH_MAP_HH
#define _CUCKOOHASH_MAP_HH

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>

#include "cuckoohash_config.hh"
#include "cuckoohash_util.hh"

/**
 * \ingroup util
 * This is a copy of libcuckoo from https://github.com/efficient/libcuckoo/
 * Apache Licensed.
 * \verbatim
 * Copyright (C) 2013, Carnegie Mellon University and Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  ---------------------------
 *
 *  The third-party libraries have their own licenses, as detailed in their source
 *  files.
 * \endverbatim
 */
template < class Key,
           class T,
           class Hash = std::hash<Key>,
           class Pred = std::equal_to<Key>,
           class Alloc = std::allocator<std::pair<const Key, T>>,
           size_t SLOT_PER_BUCKET = DEFAULT_SLOT_PER_BUCKET
           >
class cuckoohash_map {
public:
    //! key_type is the type of keys.
    typedef Key                     key_type;
    //! value_type is the type of key-value pairs.
    typedef std::pair<const Key, T> value_type;
    //! mapped_type is the type of values.
    typedef T                       mapped_type;
    //! hasher is the type of the hash function.
    typedef Hash                    hasher;
    //! key_equal is the type of the equality predicate.
    typedef Pred                    key_equal;
    //! allocator_type is the type of the allocator
    typedef Alloc                   allocator_type;

    //! Class returned by operator[] which wraps an entry in the hash table.
    //! Note that this reference type behave somewhat differently from an STL
    //! map reference. Most importantly, running this operator will not insert a
    //! default key-value pair into the map if the given key is not already in
    //! the map.
    class reference {
        // Note that this implementation here is not exactly STL compliant. To
        // maintain performance and avoid hitting the hash table too many times,
        // The reference object is *lazy*. In other words,
        //
        //  - operator[] does not actually perform an insert. It returns a
        //    reference object pointing to the requested key.
        //  - On table[i] = val // reference::operator=(mapped_type)
        //    an update / insert is called
        //  - On table[i] = table[j] // reference::operator=(const reference&)
        //    an update / insert is called with the value of table[j]
        //  - On val = table[i] // operator mapped_type()
        //    a find is called
        //  - On table[i] (i.e. no operation performed)
        //    the destructor is called immediately (reference::~reference())
        //    and nothing happens.
    public:
        //! Delete the default constructor, which should never be used
        reference() = delete;

        //! Casting to \p mapped_type runs a find for the stored key. If the
        //! find fails, it will thrown an exception.
        operator mapped_type() const {
            return owner_.find(key_);
        }

        //! The assignment operator will first try to update the value at the
        //! reference's key. If the key isn't in the table, it will insert the
        //! key with \p val.
        reference& operator=(const mapped_type& val) {
            owner_.upsert(
                key_, [&val](mapped_type& v) { v = val; }, val);
            return *this;
        }

        //! The copy assignment operator doesn't actually copy the passed-in
        //! reference. Instead, it has the same behavior as operator=(const
        //! mapped_type& val).
        reference& operator=(const reference& ref) {
            *this = (mapped_type) ref;
            return *this;
        }

    private:
        // private constructor which initializes the owner and key
        reference(
            cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>& owner,
            const key_type& key) : owner_(owner), key_(key) {}

        // reference to the hash map instance
        cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>& owner_;
        // the referenced key
        const key_type& key_;

        // cuckoohash_map needs to call the private constructor
        friend class cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>;
    };

    typedef const mapped_type const_reference;

private:
    // Constants used internally

    // true if the key is small and simple, which means using partial keys would
    // probably slow us down
    static const bool is_simple =
        std::is_pod<key_type>::value && sizeof(key_type) <= 8;

    static const bool value_copy_assignable = std::is_copy_assignable<
        mapped_type>::value;

    // number of locks in the locks_ array
    static const size_t kNumLocks = 1 << 16;

    // number of cores on the machine
    static size_t kNumCores() {
        static size_t cores = std::thread::hardware_concurrency() == 0 ?
            sysconf(_SC_NPROCESSORS_ONLN) : std::thread::hardware_concurrency();
        return cores;
    }

    // A fast, lightweight spinlock
    class spinlock {
        std::atomic_flag lock_;
    public:
        spinlock() {
            lock_.clear();
        }

        inline void lock() {
            while (lock_.test_and_set(std::memory_order_acquire));
        }

        inline void unlock() {
            lock_.clear(std::memory_order_release);
        }

        inline bool try_lock() {
            return !lock_.test_and_set(std::memory_order_acquire);
        }

    } __attribute__((aligned(64)));

    typedef enum {
        ok = 0,
        failure = 1,
        failure_key_not_found = 2,
        failure_key_duplicated = 3,
        failure_space_not_enough = 4,
        failure_function_not_supported = 5,
        failure_table_full = 6,
        failure_under_expansion = 7,
    } cuckoo_status;

    typedef char partial_t;
    // Two partial key containers. One for when we're actually using partial
    // keys and another that mocks partial keys for when the type is simple. The
    // bucket will derive the correct class depending on whether the type is
    // simple or not.
    class RealPartialContainer {
        std::array<partial_t, SLOT_PER_BUCKET> partials_;
    public:
        const partial_t& partial(int ind) const {
            return partials_[ind];
        }
        partial_t& partial(int ind) {
            return partials_[ind];
        }
    };

    class FakePartialContainer {
    public:
        // These methods should never be called, so we raise an exception if
        // they are.
        const partial_t& partial(int) const {
            throw std::logic_error(
                "FakePartialContainer::partial should never be called");
        }
        partial_t& partial(int) {
            throw std::logic_error(
                "FakePartialContainer::partial should never be called");
        }
    };

    // The Bucket type holds SLOT_PER_BUCKET keys and values, and a occupied
    // bitset, which indicates whether the slot at the given bit index is in
    // the table or not. It uses aligned_storage arrays to store the keys and
    // values to allow constructing and destroying key-value pairs in place.
    class Bucket : public std::conditional<is_simple, FakePartialContainer,
                                           RealPartialContainer>::type {
    private:
        std::array<typename std::aligned_storage<
                       sizeof(value_type), alignof(value_type)>::type,
                   SLOT_PER_BUCKET> kvpairs_;
        std::bitset<SLOT_PER_BUCKET> occupied_;

        const value_type& kvpair(int ind) const {
            return *static_cast<const value_type*>(
                static_cast<const void*>(&kvpairs_[ind]));
        }

        value_type& kvpair_noconst(int ind) {
            return *static_cast<value_type*>(
                static_cast<void*>(&kvpairs_[ind]));
        }

    public:
        bool occupied(int ind) const {
            return occupied_.test(ind);
        }

        const key_type& key(int ind) const {
            return kvpair(ind).first;
        }

        const mapped_type& val(int ind) const {
            return kvpair(ind).second;
        }

        mapped_type& val(int ind) {
            return kvpair_noconst(ind).second;
        }

        template <class... Args>
        void setKV(size_t ind, Args&&... args) {
            occupied_.set(ind);
            new ((void*)&kvpair_noconst(ind)) value_type(
                std::forward<Args>(args)...);
        }

        void eraseKV(size_t ind) {
            occupied_.reset(ind);
            (&kvpair_noconst(ind))->~value_type();
        }

        Bucket() {
            occupied_.reset();
        }

        ~Bucket() {
            for (size_t i = 0; i < SLOT_PER_BUCKET; ++i) {
                if (occupied(i)) {
                    eraseKV(i);
                }
            }
        }
    };

    // cacheint is a cache-aligned atomic integer type.
    struct cacheint {
        std::atomic<size_t> num;
        cacheint(): num(0) {}
        cacheint(size_t x): num(x) {}
        cacheint(const cacheint& x): num(x.num.load()) {}
        cacheint(cacheint&& x): num(x.num.load()) {}
    } __attribute__((aligned(64)));

    // An alias for the type of lock we are using
    typedef spinlock locktype;

    typedef typename allocator_type::template rebind<
        Bucket>::other bucket_allocator;

    typedef typename allocator_type::template rebind<
        cacheint>::other cacheint_allocator;

    // TableInfo contains the entire state of the hashtable. We allocate one
    // TableInfo pointer per hash table and store all of the table memory in it,
    // so that all the data can be atomically swapped during expansion.
    struct TableInfo {
        // 2**hashpower is the number of buckets
        const size_t hashpower_;

        // vector of buckets
        std::vector<Bucket, bucket_allocator> buckets_;

        // array of locks
        std::array<locktype, kNumLocks> locks_;

        // per-core counters for the number of inserts and deletes
        std::vector<cacheint, cacheint_allocator> num_inserts, num_deletes;

        // The constructor allocates the memory for the table. It allocates one
        // cacheint for each core in num_inserts and num_deletes.
        TableInfo(const size_t hashpower)
            : hashpower_(hashpower), buckets_(hashsize(hashpower_)),
              num_inserts(kNumCores(), 0), num_deletes(kNumCores(), 0) {}

        ~TableInfo() {}
    };

    typedef typename allocator_type::template rebind<
        TableInfo>::other tableinfo_allocator;

    static tableinfo_allocator get_tableinfo_allocator() {
        static tableinfo_allocator alloc;
        return alloc;
    }

    // This is a hazard pointer, used to indicate which version of the TableInfo
    // is currently being used in the thread. Since cuckoohash_map operations
    // can run simultaneously in different threads, this variable is thread
    // local. Note that this variable can be safely shared between different
    // cuckoohash_map instances, since multiple operations cannot occur
    // simultaneously in one thread. The hazard pointer variable points to a
    // pointer inside a global list of pointers, that each map checks before
    // deleting any old TableInfo pointers.
    static __thread TableInfo** hazard_pointer;

    // A GlobalHazardPointerList stores a list of pointers that cannot be
    // deleted by an expansion thread. Each thread gets its own node in the
    // list, whose data pointer it can modify without contention.
    class GlobalHazardPointerList {
        std::list<TableInfo*> hp_;
        std::mutex lock_;
    public:
        // new_hazard_pointer creates and returns a new hazard pointer for a
        // thread.
        TableInfo** new_hazard_pointer() {
            std::unique_lock<std::mutex> ul(lock_);
            hp_.emplace_back(nullptr);
            return &hp_.back();
        }

        // delete_unused scans the list of hazard pointers, deleting any
        // pointers in old_pointers that aren't in this list. If it does delete
        // a pointer in old_pointers, it deletes that node from the list.
        void delete_unused(std::list<std::unique_ptr<TableInfo>>&
                           old_pointers) {
            std::unique_lock<std::mutex> ul(lock_);
            old_pointers.remove_if(
                [this](const std::unique_ptr<TableInfo>& ptr) {
                    return std::find(hp_.begin(), hp_.end(), ptr.get()) ==
                        hp_.end();
                });
        }
    };

    // As long as the thread_local hazard_pointer is static, which means each
    // template instantiation of a cuckoohash_map class gets its own per-thread
    // hazard pointer, then each template instantiation of a cuckoohash_map
    // class can get its own global_hazard_pointers list, since different
    // template instantiations won't interfere with each other.
    static GlobalHazardPointerList global_hazard_pointers;

    // check_hazard_pointer should be called before any public method that loads
    // a table snapshot. It checks that the thread local hazard pointer pointer
    // is not null, and gets a new pointer if it is null.
    static inline void check_hazard_pointer() {
        if (hazard_pointer == nullptr) {
            hazard_pointer = global_hazard_pointers.new_hazard_pointer();
        }
    }

    // Once a function is finished with a version of the table, it will want to
    // unset the hazard pointer it set so that it can be freed if it needs to.
    // This is an object which, upon destruction, will unset the hazard pointer.
    class HazardPointerUnsetter {
    public:
        ~HazardPointerUnsetter() {
            *hazard_pointer = nullptr;
        }
    };

    // counterid stores the per-thread counter index of each thread.
    static __thread int counterid;

    // check_counterid checks if the counterid has already been determined. If
    // not, it assigns a counterid to the current thread by picking a random
    // core. This should be called at the beginning of any function that changes
    // the number of elements in the table.
    static inline void check_counterid() {
        if (counterid < 0) {
            counterid = rand() % kNumCores();
        }
    }

    // reserve_calc takes in a parameter specifying a certain number of slots
    // for a table and returns the smallest hashpower that will hold n elements.
    static size_t reserve_calc(size_t n) {
        double nhd = ceil(log2((double)n / (double)SLOT_PER_BUCKET));
        size_t new_hashpower = (size_t) (nhd <= 0 ? 1.0 : nhd);
        assert(n <= hashsize(new_hashpower) * SLOT_PER_BUCKET);
        return new_hashpower;
    }

    // hashfn returns an instance of the hash function
    static hasher hashfn() {
        static hasher hash;
        return hash;
    }

    // eqfn returns an instance of the equality predicate
    static key_equal eqfn() {
        static key_equal eq;
        return eq;
    }

public:
    //! The constructor creates a new hash table with enough space for \p n
    //! elements. If the constructor fails, it will throw an exception.
    explicit cuckoohash_map(size_t n = DEFAULT_SIZE) {
        const size_t hashpower = reserve_calc(n);
        TableInfo* ptr = get_tableinfo_allocator().allocate(1);
        try {
            get_tableinfo_allocator().construct(ptr, hashpower);
            table_info.store(ptr);
        } catch (...) {
            get_tableinfo_allocator().deallocate(ptr, 1);
            throw;
        }
    }

    //! The destructor explicitly deletes the current table info.
    ~cuckoohash_map() {
        TableInfo* ti = table_info.load();
        if (ti != nullptr) {
            get_tableinfo_allocator().destroy(ti);
            get_tableinfo_allocator().deallocate(ti, 1);
        }
    }

    //! clear removes all the elements in the hash table, calling their
    //! destructors.
    void clear() {
        check_hazard_pointer();
        TableInfo* ti = snapshot_and_lock_all();
        assert(ti == table_info.load());
        AllUnlocker au(ti);
        HazardPointerUnsetter hpu;
        cuckoo_clear(ti);
    }

    //! size returns the number of items currently in the hash table. Since it
    //! doesn't lock the table, elements can be inserted during the computation,
    //! so the result may not necessarily be exact.
    size_t size() const {
        check_hazard_pointer();
        const TableInfo* ti = snapshot_table_nolock();
        HazardPointerUnsetter hpu;
        const size_t s = cuckoo_size(ti);
        return s;
    }

    //! empty returns true if the table is empty.
    bool empty() const {
        return size() == 0;
    }

    //! hashpower returns the hashpower of the table, which is
    //! log<SUB>2</SUB>(the number of buckets).
    size_t hashpower() const {
        check_hazard_pointer();
        TableInfo* ti = snapshot_table_nolock();
        HazardPointerUnsetter hpu;
        const size_t hashpower = ti->hashpower_;
        return hashpower;
    }

    //! bucket_count returns the number of buckets in the table.
    size_t bucket_count() const {
        check_hazard_pointer();
        TableInfo* ti = snapshot_table_nolock();
        HazardPointerUnsetter hpu;
        size_t buckets = hashsize(ti->hashpower_);
        return buckets;
    }

    //! load_factor returns the ratio of the number of items in the table to the
    //! total number of available slots in the table.
    double load_factor() const {
        check_hazard_pointer();
        const TableInfo* ti = snapshot_table_nolock();
        HazardPointerUnsetter hpu;
        return cuckoo_loadfactor(ti);
    }

    //! find searches through the table for \p key, and stores the associated
    //! value it finds in \p val.
    ENABLE_IF(, value_copy_assignable, bool)
    find(const key_type& key, mapped_type& val) const {
        check_hazard_pointer();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;
        std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
        HazardPointerUnsetter hpu;

        const cuckoo_status st = cuckoo_find(key, val, hv, ti, i1, i2);
        unlock_two(ti, i1, i2);
        return (st == ok);
    }

    //! This version of find does the same thing as the two-argument version,
    //! except it returns the value it finds, throwing an \p std::out_of_range
    //! exception if the key isn't in the table.
    ENABLE_IF(, value_copy_assignable, mapped_type)
    find(const key_type& key) const {
        mapped_type val;
        bool done = find(key, val);
        if (done) {
            return val;
        } else {
            throw std::out_of_range("key not found in table");
        }
    }

    //! contains searches through the table for \p key, and returns true if it
    //! finds it in the table, and false otherwise.
    bool contains(const key_type& key) const {
        check_hazard_pointer();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;
        std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
        HazardPointerUnsetter hpu;

        const bool result = cuckoo_contains(key, hv, ti, i1, i2);
        unlock_two(ti, i1, i2);
        return result;
    }

    //! insert puts the given key-value pair into the table. It first checks
    //! that \p key isn't already in the table, since the table doesn't support
    //! duplicate keys. If the table is out of space, insert will automatically
    //! expand until it can succeed. Note that expansion can throw an exception,
    //! which insert will propagate. If \p key is already in the table, it
    //! returns false, otherwise it returns true.
    template <class V>
    typename std::enable_if<std::is_convertible<V, const mapped_type&>::value,
                            bool>::type
    insert(const key_type& key, V val) {
        check_hazard_pointer();
        check_counterid();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;
        std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
        HazardPointerUnsetter hpu;
        return cuckoo_insert_loop(key, std::forward<V>(val),
                                  hv, ti, i1, i2);
    }

    //! erase removes \p key and it's associated value from the table, calling
    //! their destructors. If \p key is not there, it returns false, otherwise
    //! it returns true.
    bool erase(const key_type& key) {
        check_hazard_pointer();
        check_counterid();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;
        std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
        HazardPointerUnsetter hpu;

        const cuckoo_status st = cuckoo_delete(key, hv, ti, i1, i2);
        unlock_two(ti, i1, i2);
        return (st == ok);
    }

    //! update changes the value associated with \p key to \p val. If \p key is
    //! not there, it returns false, otherwise it returns true.
    ENABLE_IF(, value_copy_assignable, bool)
    update(const key_type& key, const mapped_type& val) {
        check_hazard_pointer();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;
        std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
        HazardPointerUnsetter hpu;

        const cuckoo_status st = cuckoo_update(key, val, hv, ti, i1, i2);
        unlock_two(ti, i1, i2);
        return (st == ok);
    }

    //! update_fn changes the value associated with \p key with the function \p
    //! fn. \p fn will be passed one argument of type \p mapped_type& and can
    //! modify the argument as desired, returning nothing. If \p key is not
    //! there, it returns false, otherwise it returns true.
    template <typename Updater>
    bool update_fn(const key_type& key, Updater fn) {
        check_hazard_pointer();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;
        std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
        HazardPointerUnsetter hpu;

        const cuckoo_status st = cuckoo_update_fn(key, fn, hv, ti, i1, i2);
        unlock_two(ti, i1, i2);
        return (st == ok);
    }

    //! upsert is a combination of update_fn and insert. It first tries updating
    //! the value associated with \p key using \p fn. If \p key is not in the
    //! table, then it runs an insert with \p key and \p val. It will always
    //! succeed, since if the update fails and the insert finds the key already
    //! inserted, it can retry the update.
    template <typename Updater>
    void upsert(const key_type& key, Updater fn, const mapped_type& val) {
        check_hazard_pointer();
        check_counterid();
        size_t hv = hashed_key(key);
        TableInfo* ti;
        size_t i1, i2;

        bool res;
        do {
            std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
            HazardPointerUnsetter hpu;
            const cuckoo_status st = cuckoo_update_fn(key, fn, hv, ti, i1, i2);
            if (st == ok) {
                unlock_two(ti, i1, i2);
                return;
            }

            // We run an insert, since the update failed
            res = cuckoo_insert_loop(key, val, hv, ti, i1, i2);

            // The only valid reason for res being false is if insert
            // encountered a duplicate key after releasing the locks and
            // performing cuckoo hashing. In this case, we retry the entire
            // upsert operation.
        } while (!res);
        return;
    }

    //! rehash will size the table using a hashpower of \p n. Note that the
    //! number of buckets in the table will be 2<SUP>\p n</SUP> after expansion,
    //! so the table will have 2<SUP>\p n</SUP> &times; \ref SLOT_PER_BUCKET
    //! slots to store items in. If \p n is not larger than the current
    //! hashpower, then it decreases the hashpower to either \p n or the
    //! smallest power that can hold all the elements currently in the table. It
    //! returns true if the table expansion succeeded, and false otherwise.
    //! rehash can throw an exception if the expansion fails to allocate enough
    //! memory for the larger table.
    bool rehash(size_t n) {
        check_hazard_pointer();
        TableInfo* ti = snapshot_table_nolock();
        HazardPointerUnsetter hpu;
        if (n == ti->hashpower_) {
            return false;
        }
        const cuckoo_status st = cuckoo_expand_simple(
            n, n > ti->hashpower_);
        return (st == ok);
    }

    //! reserve will size the table to have enough slots for at least \p n
    //! elements. If the table can already hold that many elements, the function
    //! will shrink the table to the smallest hashpower that can hold the
    //! maximum of \p n and the current table size. Otherwise, the function will
    //! expand the table to a hashpower sufficient to hold \p n elements. It
    //! will return true if there was an expansion, and false otherwise. reserve
    //! can throw an exception if the expansion fails to allocate enough memory
    //! for the larger table.
    bool reserve(size_t n) {
        check_hazard_pointer();
        TableInfo* ti = snapshot_table_nolock();
        HazardPointerUnsetter hpu;
        size_t new_hashpower = reserve_calc(n);
        if (new_hashpower == ti->hashpower_) {
            return false;
        }
        const cuckoo_status st = cuckoo_expand_simple(
            new_hashpower, new_hashpower > ti->hashpower_);
        return (st == ok);
    }

    //! hash_function returns the hash function object used by the table.
    hasher hash_function() const {
        return hashfn();
    }

    //! key_eq returns the equality predicate object used by the table.
    key_equal key_eq() const {
        return eqfn();
    }

    //! Returns a \ref reference to the mapped value stored at the given key.
    //! Note that the reference behaves somewhat differently from an STL map
    //! reference (see the \ref reference documentation for details).
    reference operator[](const key_type& key) {
        return (reference(*this, key));
    }

    //! Returns a \ref const_reference to the mapped value stored at the given
    //! key. This is equivalent to running the overloaded \ref find function
    //! with no value parameter.
    const_reference operator[](const key_type& key) const {
        return find(key);
    }

private:
    std::atomic<TableInfo*> table_info;

    // old_table_infos holds pointers to old TableInfos that were replaced
    // during expansion. This keeps the memory alive for any leftover
    // operations, until they are deleted by the global hazard pointer manager.
    std::list<std::unique_ptr<TableInfo>> old_table_infos;

    // lock locks the given bucket index.
    static inline void lock(TableInfo* ti, const size_t i) {
        ti->locks_[lock_ind(i)].lock();
    }

    // unlock unlocks the given bucket index.
    static inline void unlock(TableInfo* ti, const size_t i) {
        ti->locks_[lock_ind(i)].unlock();
    }

    // lock_two locks the two bucket indexes, always locking the earlier index
    // first to avoid deadlock. If the two indexes are the same, it just locks
    // one.
    static void lock_two(TableInfo* ti, size_t i1, size_t i2) {
        i1 = lock_ind(i1);
        i2 = lock_ind(i2);
        if (i1 < i2) {
            ti->locks_[i1].lock();
            ti->locks_[i2].lock();
        } else if (i2 < i1) {
            ti->locks_[i2].lock();
            ti->locks_[i1].lock();
        } else {
            ti->locks_[i1].lock();
        }
    }

    // unlock_two unlocks both of the given bucket indexes, or only one if they
    // are equal. Order doesn't matter here.
    static void unlock_two(TableInfo* ti, size_t i1, size_t i2) {
        i1 = lock_ind(i1);
        i2 = lock_ind(i2);
        ti->locks_[i1].unlock();
        if (i1 != i2) {
            ti->locks_[i2].unlock();
        }
    }

    // lock_three locks the three bucket indexes in numerical order.
    static void lock_three(TableInfo* ti, size_t i1,
                           size_t i2, size_t i3) {
        i1 = lock_ind(i1);
        i2 = lock_ind(i2);
        i3 = lock_ind(i3);
        // If any are the same, we just run lock_two
        if (i1 == i2) {
            lock_two(ti, i1, i3);
        } else if (i2 == i3) {
            lock_two(ti, i1, i3);
        } else if (i1 == i3) {
            lock_two(ti, i1, i2);
        } else {
            if (i1 < i2) {
                if (i2 < i3) {
                    ti->locks_[i1].lock();
                    ti->locks_[i2].lock();
                    ti->locks_[i3].lock();
                } else if (i1 < i3) {
                    ti->locks_[i1].lock();
                    ti->locks_[i3].lock();
                    ti->locks_[i2].lock();
                } else {
                    ti->locks_[i3].lock();
                    ti->locks_[i1].lock();
                    ti->locks_[i2].lock();
                }
            } else if (i2 < i3) {
                if (i1 < i3) {
                    ti->locks_[i2].lock();
                    ti->locks_[i1].lock();
                    ti->locks_[i3].lock();
                } else {
                    ti->locks_[i2].lock();
                    ti->locks_[i3].lock();
                    ti->locks_[i1].lock();
                }
            } else {
                ti->locks_[i3].lock();
                ti->locks_[i2].lock();
                ti->locks_[i1].lock();
            }
        }
    }

    // unlock_three unlocks the three given buckets
    static void unlock_three(TableInfo* ti, size_t i1,
                             size_t i2, size_t i3) {
        i1 = lock_ind(i1);
        i2 = lock_ind(i2);
        i3 = lock_ind(i3);
        ti->locks_[i1].unlock();
        if (i2 != i1) {
            ti->locks_[i2].unlock();
        }
        if (i3 != i1 && i3 != i2) {
            ti->locks_[i3].unlock();
        }
    }

    // snapshot_table_nolock loads the table info pointer and sets the hazard
    // pointer, whithout locking anything. There is a possibility that after
    // loading a snapshot and setting the hazard pointer, an expansion runs and
    // create a new version of the table, leaving the old one for deletion. To
    // deal with that, we check that the table_info we loaded is the same as the
    // current one, and if it isn't, we try again. Whenever we check if (ti !=
    // table_info.load()) after setting the hazard pointer, there is an ABA
    // issue, where the address of the new table_info equals the address of a
    // previously deleted one, however it doesn't matter, since we would still
    // be looking at the most recent table_info in that case.
    TableInfo* snapshot_table_nolock() const {
        while (true) {
            TableInfo* ti = table_info.load();
            *hazard_pointer = ti;
            // If the table info has changed in the time we set the hazard
            // pointer, ti could have been deleted, so try again.
            if (ti != table_info.load()) {
                continue;
            }
            return ti;
        }
    }

    // snapshot_and_lock_two loads the table_info pointer and locks the buckets
    // associated with the given hash value. It returns the table_info and the
    // two locked buckets as a tuple. Since the positions of the bucket locks
    // depends on the number of buckets in the table, the table_info pointer
    // needs to be grabbed first.
    std::tuple<TableInfo*, size_t, size_t>
    snapshot_and_lock_two(const size_t hv) const {
        TableInfo* ti;
        size_t i1, i2;
        while (true) {
            ti = table_info.load();
            *hazard_pointer = ti;
            // If the table info has changed in the time we set the hazard
            // pointer, ti could have been deleted, so try again.
            if (ti != table_info.load()) {
                continue;
            }
            i1 = index_hash(ti, hv);
            i2 = alt_index(ti, hv, i1);
            lock_two(ti, i1, i2);
            // Check the table info again
            if (ti != table_info.load()) {
                unlock_two(ti, i1, i2);
                continue;
            }
            return std::make_tuple(ti, i1, i2);
        }
    }

    // AllUnlocker is an object which releases all the locks on the given table
    // info when it's destructor is called.
    class AllUnlocker {
        TableInfo* ti_;
    public:
        AllUnlocker(TableInfo* ti): ti_(ti) {}
        ~AllUnlocker() {
            if (ti_ != nullptr) {
                for (size_t i = 0; i < kNumLocks; ++i) {
                    ti_->locks_[i].unlock();
                }
            }
        }
    };

    // snapshot_and_lock_all is similar to snapshot_and_lock_two, except that it
    // takes all the locks in the table.
    TableInfo* snapshot_and_lock_all() const {
        while (true) {
            TableInfo* ti = table_info.load();
            *hazard_pointer = ti;
            // If the table info has changed, ti could have been deleted, so try
            // again
            if (ti != table_info.load()) {
                continue;
            }
            for (size_t i = 0; i < kNumLocks; ++i) {
                ti->locks_[i].lock();
            }
            // If the table info has changed, unlock the locks and try again.
            if (ti != table_info.load()) {
                AllUnlocker au(ti);
                continue;
            }
            return ti;
        }
    }

    // lock_ind converts an index into buckets_ to an index into locks_.
    static inline size_t lock_ind(const size_t bucket_ind) {
        return bucket_ind & (kNumLocks - 1);
    }

    // hashsize returns the number of buckets corresponding to a given
    // hashpower.
    static inline size_t hashsize(const size_t hashpower) {
        return 1U << hashpower;
    }

    // hashmask returns the bitmask for the buckets array corresponding to a
    // given hashpower.
    static inline size_t hashmask(const size_t hashpower) {
        return hashsize(hashpower) - 1;
    }

    // hashed_key hashes the given key.
    static inline size_t hashed_key(const key_type &key) {
        return hashfn()(key);
    }

    // index_hash returns the first possible bucket that the given hashed key
    // could be.
    static inline size_t index_hash(const TableInfo* ti, const size_t hv) {
        return hv & hashmask(ti->hashpower_);
    }

    // alt_index returns the other possible bucket that the given hashed key
    // could be. It takes the first possible bucket as a parameter. Note that
    // this function will return the first possible bucket if index is the
    // second possible bucket, so alt_index(ti, hv, alt_index(ti, hv,
    // index_hash(ti, hv))) == index_hash(ti, hv).
    static inline size_t alt_index(
        const TableInfo* ti, const size_t hv, const size_t index) {
        // ensure tag is nonzero for the multiply
        const size_t tag = (hv >> ti->hashpower_) + 1;
        // 0x5bd1e995 is the hash constant from MurmurHash2
        return (index ^ (tag * 0x5bd1e995)) & hashmask(ti->hashpower_);
    }

    // partial_key returns a partial_t representing the upper sizeof(partial_t)
    // bytes of the hashed key. This is used for partial-key cuckoohashing. If
    // the key type is POD and small, we don't use partial keys, so we just
    // return 0.
    ENABLE_IF(static inline, is_simple, partial_t)
    partial_key(const size_t hv) {
        return (partial_t)(hv >> ((sizeof(size_t)-sizeof(partial_t)) * 8));
    }

    ENABLE_IF(static inline, !is_simple, partial_t) partial_key(const size_t&) {
        return 0;
    }

    // A constexpr version of pow that we can use for static_asserts
    static constexpr size_t const_pow(size_t a, size_t b) {
        return (b == 0) ? 1 : a * const_pow(a, b - 1);
    }

    // The maximum number of items in a BFS path.
    static const uint8_t MAX_BFS_PATH_LEN = 5;

    // CuckooRecord holds one position in a cuckoo path.
    typedef struct  {
        size_t bucket;
        size_t slot;
        key_type key;
    }  CuckooRecord;

    // b_slot holds the information for a BFS path through the table
    struct b_slot {
        // The bucket of the last item in the path
        size_t bucket;
        // a compressed representation of the slots for each of the buckets in
        // the path. pathcode is sort of like a base-SLOT_PER_BUCKET number, and
        // we need to hold at most MAX_BFS_PATH_LEN slots. Thus we need the
        // maximum pathcode to be at least SLOT_PER_BUCKET^(MAX_BFS_PATH_LEN)
        size_t pathcode;
        static_assert(const_pow(SLOT_PER_BUCKET, MAX_BFS_PATH_LEN) <
                      std::numeric_limits<decltype(pathcode)>::max(),
                      "pathcode may not be large enough to encode a cuckoo"
                      " path");
        // The 0-indexed position in the cuckoo path this slot occupies. It must
        // be less than MAX_BFS_PATH_LEN, and also able to hold negative values.
        int_fast8_t depth;
        static_assert(MAX_BFS_PATH_LEN - 1 <=
                      std::numeric_limits<decltype(depth)>::max(),
                      "The depth type must able to hold a value of"
                      " MAX_BFS_PATH_LEN - 1");
        static_assert(-1 >= std::numeric_limits<decltype(depth)>::min(),
                      "The depth type must be able to hold a value of -1");
        b_slot() {}
        b_slot(const size_t b, const size_t p, const decltype(depth) d)
            : bucket(b), pathcode(p), depth(d) {
            assert(d < MAX_BFS_PATH_LEN);
        }
    } __attribute__((__packed__));

    // b_queue is the queue used to store b_slots for BFS cuckoo hashing.
    class b_queue {
        // The maximum size of the BFS queue. Note that unless it's less than
        // SLOT_PER_BUCKET^MAX_BFS_PATH_LEN, it won't really mean anything.
        static const size_t MAX_CUCKOO_COUNT = 512;
        static_assert((MAX_CUCKOO_COUNT & (MAX_CUCKOO_COUNT - 1)) == 0,
                      "MAX_CUCKOO_COUNT should be a power of 2");
        // A circular array of b_slots
        b_slot slots[MAX_CUCKOO_COUNT];
        // The index of the head of the queue in the array
        size_t first;
        // One past the index of the last item of the queue in the array.
        size_t last;

        // returns the index in the queue after ind, wrapping around if
        // necessary.
        size_t increment(size_t ind) {
            return (ind + 1) & (MAX_CUCKOO_COUNT - 1);
        }

    public:
        b_queue() : first(0), last(0) {}

        void enqueue(b_slot x) {
            assert(!full());
            slots[last] = x;
            last = increment(last);
        }

        b_slot dequeue() {
            assert(!empty());
            b_slot& x = slots[first];
            first = increment(first);
            return x;
        }

        bool empty() {
            return first == last;
        }

        bool full() {
            return increment(last) == first;
        }
    } __attribute__((__packed__));

    // slot_search searches for a cuckoo path using breadth-first search. It
    // starts with the i1 and i2 buckets, and, until it finds a bucket with an
    // empty slot, adds each slot of the bucket in the b_slot. If the queue runs
    // out of space, it fails.
    static b_slot slot_search(TableInfo* ti, const size_t i1, const size_t i2) {
        b_queue q;
        // The initial pathcode informs cuckoopath_search which bucket the path
        // starts on
        q.enqueue(b_slot(i1, 0, 0));
        q.enqueue(b_slot(i2, 1, 0));
        while (!q.full() && !q.empty()) {
            b_slot x = q.dequeue();
            // Picks a (sort-of) random slot to start from
            size_t starting_slot = x.pathcode % SLOT_PER_BUCKET;
            for (size_t i = 0; i < SLOT_PER_BUCKET && !q.full();
                 ++i) {
                size_t slot = (starting_slot + i) % SLOT_PER_BUCKET;
                lock(ti, x.bucket);
                if (!ti->buckets_[x.bucket].occupied(slot)) {
                    // We can terminate the search here
                    x.pathcode = x.pathcode * SLOT_PER_BUCKET + slot;
                    unlock(ti, x.bucket);
                    return x;
                }

                // If x has less than the maximum number of path components,
                // create a new b_slot item, that represents the bucket we would
                // have come from if we kicked out the item at this slot.
                if (x.depth < MAX_BFS_PATH_LEN - 1) {
                    const size_t hv = hashed_key(
                        ti->buckets_[x.bucket].key(slot));
                    unlock(ti, x.bucket);
                    b_slot y(alt_index(ti, hv, x.bucket),
                             x.pathcode * SLOT_PER_BUCKET + slot, x.depth+1);
                    q.enqueue(y);
                }
            }
        }
        // We didn't find a short-enough cuckoo path, so the queue ran out of
        // space. Return a failure value.
        return b_slot(0, 0, -1);
    }

    // cuckoopath_search finds a cuckoo path from one of the starting buckets to
    // an empty slot in another bucket. It returns the depth of the discovered
    // cuckoo path on success, and -1 on failure. Since it doesn't take locks on
    // the buckets it searches, the data can change between this function and
    // cuckoopath_move. Thus cuckoopath_move checks that the data matches the
    // cuckoo path before changing it.
    static int cuckoopath_search(TableInfo* ti, CuckooRecord* cuckoo_path,
                                 const size_t i1, const size_t i2) {
        b_slot x = slot_search(ti, i1, i2);
        if (x.depth == -1) {
            return -1;
        }
        // Fill in the cuckoo path slots from the end to the beginning
        for (int i = x.depth; i >= 0; i--) {
            cuckoo_path[i].slot = x.pathcode % SLOT_PER_BUCKET;
            x.pathcode /= SLOT_PER_BUCKET;
        }
        // Fill in the cuckoo_path buckets and keys from the beginning to the
        // end, using the final pathcode to figure out which bucket the path
        // starts on. Since data could have been modified between slot_search
        // and the computation of the cuckoo path, this could be an invalid
        // cuckoo_path.
        CuckooRecord* curr = cuckoo_path;
        if (x.pathcode == 0) {
            curr->bucket = i1;
            lock(ti, curr->bucket);
            if (!ti->buckets_[curr->bucket].occupied(curr->slot)) {
                // We can terminate here
                unlock(ti, curr->bucket);
                return 0;
            }
            curr->key = ti->buckets_[curr->bucket].key(curr->slot);
            unlock(ti, curr->bucket);
        } else {
            assert(x.pathcode == 1);
            curr->bucket = i2;
            lock(ti, curr->bucket);
            if (!ti->buckets_[curr->bucket].occupied(curr->slot)) {
                // We can terminate here
                unlock(ti, curr->bucket);
                return 0;
            }
            curr->key = ti->buckets_[curr->bucket].key(curr->slot);
            unlock(ti, curr->bucket);
        }
        for (int i = 1; i <= x.depth; ++i) {
            CuckooRecord* prev = curr++;
            const size_t prevhv = hashed_key(prev->key);
            assert(prev->bucket == index_hash(ti, prevhv) ||
                   prev->bucket == alt_index(ti, prevhv, index_hash(ti,
                                                                    prevhv)));
            // We get the bucket that this slot is on by computing the alternate
            // index of the previous bucket
            curr->bucket = alt_index(ti, prevhv, prev->bucket);
            lock(ti, curr->bucket);
            if (!ti->buckets_[curr->bucket].occupied(curr->slot)) {
                // We can terminate here
                unlock(ti, curr->bucket);
                return i;
            }
            curr->key = ti->buckets_[curr->bucket].key(curr->slot);
            unlock(ti, curr->bucket);
        }
        return x.depth;
    }


    // cuckoopath_move moves keys along the given cuckoo path in order to make
    // an empty slot in one of the buckets in cuckoo_insert. Before the start of
    // this function, the two insert-locked buckets were unlocked in run_cuckoo.
    // At the end of the function, if the function returns true (success), then
    // the last bucket it looks at (which is either i1 or i2 in run_cuckoo)
    // remains locked. If the function is unsuccessful, then both insert-locked
    // buckets will be unlocked.
    static bool cuckoopath_move(
        TableInfo* ti, CuckooRecord* cuckoo_path, size_t depth,
        const size_t i1, const size_t i2) {
        if (depth == 0) {
            // There is a chance that depth == 0, when try_add_to_bucket sees i1
            // and i2 as full and cuckoopath_search finds one empty. In this
            // case, we lock both buckets. If the bucket that cuckoopath_search
            // found empty isn't empty anymore, we unlock them and return false.
            // Otherwise, the bucket is empty and insertable, so we hold the
            // locks and return true.
            const size_t bucket = cuckoo_path[0].bucket;
            assert(bucket == i1 || bucket == i2);
            lock_two(ti, i1, i2);
            if (!ti->buckets_[bucket].occupied(cuckoo_path[0].slot)) {
                return true;
            } else {
                unlock_two(ti, i1, i2);
                return false;
            }
        }

        while (depth > 0) {
            CuckooRecord* from = cuckoo_path + depth - 1;
            CuckooRecord* to   = cuckoo_path + depth;
            size_t fb = from->bucket;
            size_t fs = from->slot;
            size_t tb = to->bucket;
            size_t ts = to->slot;

            size_t ob = 0;
            if (depth == 1) {
                // Even though we are only swapping out of i1 or i2, we have to
                // lock both of them along with the slot we are swapping to,
                // since at the end of this function, i1 and i2 must be locked.
                ob = (fb == i1) ? i2 : i1;
                lock_three(ti, fb, tb, ob);
            } else {
                lock_two(ti, fb, tb);
            }

            // We plan to kick out fs, but let's check if it is still there;
            // there's a small chance we've gotten scooped by a later cuckoo. If
            // that happened, just... try again. Also the slot we are filling in
            // may have already been filled in by another thread, or the slot we
            // are moving from may be empty, both of which invalidate the swap.
            if (!eqfn()(ti->buckets_[fb].key(fs), from->key) ||
                ti->buckets_[tb].occupied(ts) ||
                !ti->buckets_[fb].occupied(fs)) {
                if (depth == 1) {
                    unlock_three(ti, fb, tb, ob);
                } else {
                    unlock_two(ti, fb, tb);
                }
                return false;
            }

            if (!is_simple) {
                ti->buckets_[tb].partial(ts) = ti->buckets_[fb].partial(fs);
            }
            ti->buckets_[tb].setKV(ts, ti->buckets_[fb].key(fs),
                                   std::move(ti->buckets_[fb].val(fs)));
            ti->buckets_[fb].eraseKV(fs);
            if (depth == 1) {
                // Don't unlock fb or ob, since they are needed in
                // cuckoo_insert. Only unlock tb if it doesn't unlock the same
                // bucket as fb or ob.
                if (lock_ind(tb) != lock_ind(fb) &&
                    lock_ind(tb) != lock_ind(ob)) {
                    unlock(ti, tb);
                }
            } else {
                unlock_two(ti, fb, tb);
            }
            depth--;
        }
        return true;
    }

    // run_cuckoo performs cuckoo hashing on the table in an attempt to free up
    // a slot on either i1 or i2. On success, the bucket and slot that was freed
    // up is stored in insert_bucket and insert_slot. In order to perform the
    // search and the swaps, it has to unlock both i1 and i2, which can lead to
    // certain concurrency issues, the details of which are explained in the
    // function. If run_cuckoo returns ok (success), then the slot it freed up
    // is still locked. Otherwise it is unlocked.
    cuckoo_status run_cuckoo(TableInfo* ti, const size_t i1, const size_t i2,
                             size_t &insert_bucket, size_t &insert_slot) {

        CuckooRecord cuckoo_path[MAX_BFS_PATH_LEN];

        // We must unlock i1 and i2 here, so that cuckoopath_search and
        // cuckoopath_move can lock buckets as desired without deadlock.
        // cuckoopath_move has to look at either i1 or i2 as its last slot, and
        // it will lock both buckets and leave them locked after finishing. This
        // way, we know that if cuckoopath_move succeeds, then the buckets
        // needed for insertion are still locked. If cuckoopath_move fails, the
        // buckets are unlocked and we try again. This unlocking does present
        // two problems. The first is that another insert on the same key runs
        // and, finding that the key isn't in the table, inserts the key into
        // the table. Then we insert the key into the table, causing a
        // duplication. To check for this, we search i1 and i2 for the key we
        // are trying to insert before doing so (this is done in cuckoo_insert,
        // and requires that both i1 and i2 are locked). Another problem is that
        // an expansion runs and changes table_info, meaning the cuckoopath_move
        // and cuckoo_insert would have operated on an old version of the table,
        // so the insert would be invalid. For this, we check that ti ==
        // table_info.load() after cuckoopath_move, signaling to the outer
        // insert to try again if the comparison fails.
        unlock_two(ti, i1, i2);

        bool done = false;
        while (!done) {
            int depth = cuckoopath_search(ti, cuckoo_path, i1, i2);
            if (depth < 0) {
                break;
            }

            if (cuckoopath_move(ti, cuckoo_path, depth, i1, i2)) {
                insert_bucket = cuckoo_path[0].bucket;
                insert_slot = cuckoo_path[0].slot;
                assert(insert_bucket == i1 || insert_bucket == i2);
                assert(!ti->locks_[lock_ind(i1)].try_lock());
                assert(!ti->locks_[lock_ind(i2)].try_lock());
                assert(!ti->buckets_[insert_bucket].occupied(insert_slot));
                done = true;
                break;
            }
        }

        if (!done) {
            return failure;
        } else if (ti != table_info.load()) {
            // Unlock i1 and i2 and signal to cuckoo_insert to try again. Since
            // we set the hazard pointer to be ti, this check isn't susceptible
            // to an ABA issue, since a new pointer can't have the same address
            // as ti.
            unlock_two(ti, i1, i2);
            return failure_under_expansion;
        }
        return ok;
    }

    // try_read_from_bucket will search the bucket for the given key and store
    // the associated value if it finds it.
    ENABLE_IF(static, value_copy_assignable, bool) try_read_from_bucket(
        const TableInfo* ti, const partial_t partial,
        const key_type &key, mapped_type &val, const size_t i) {
        for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
            if (!ti->buckets_[i].occupied(j)) {
                continue;
            }
            if (!is_simple && partial != ti->buckets_[i].partial(j)) {
                continue;
            }
            if (eqfn()(key, ti->buckets_[i].key(j))) {
                val = ti->buckets_[i].val(j);
                return true;
            }
        }
        return false;
    }

    // check_in_bucket will search the bucket for the given key and return true
    // if the key is in the bucket, and false if it isn't.
    static bool check_in_bucket(
        const TableInfo* ti, const partial_t partial,
        const key_type &key, const size_t i) {
        for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
            if (!ti->buckets_[i].occupied(j)) {
                continue;
            }
            if (!is_simple && partial != ti->buckets_[i].partial(j)) {
                continue;
            }
            if (eqfn()(key, ti->buckets_[i].key(j))) {
                return true;
            }
        }
        return false;
    }

    // add_to_bucket will insert the given key-value pair into the slot.
    template <class V>
    static void add_to_bucket(TableInfo* ti, const partial_t partial,
                              const key_type &key, V val,
                              const size_t i, const size_t j) {
        assert(!ti->buckets_[i].occupied(j));
        if (!is_simple) {
            ti->buckets_[i].partial(j) = partial;
        }
        ti->buckets_[i].setKV(j, key, std::forward<V>(val));
        ti->num_inserts[counterid].num.fetch_add(1, std::memory_order_relaxed);
    }

    // try_find_insert_bucket will search the bucket and store the index of an
    // empty slot if it finds one, or -1 if it doesn't. Regardless, it will
    // search the entire bucket and return false if it finds the key already in
    // the table (duplicate key error) and true otherwise.
    static bool try_find_insert_bucket(
        TableInfo* ti, const partial_t partial,
        const key_type &key, const size_t i, int& j) {
        j = -1;
        bool found_empty = false;
        for (size_t k = 0; k < SLOT_PER_BUCKET; ++k) {
            if (ti->buckets_[i].occupied(k)) {
                if (!is_simple && partial != ti->buckets_[i].partial(k)) {
                    continue;
                }
                if (eqfn()(key, ti->buckets_[i].key(k))) {
                    return false;
                }
            } else {
                if (!found_empty) {
                    found_empty = true;
                    j = k;
                }
            }
        }
        return true;
    }

    // try_del_from_bucket will search the bucket for the given key, and set the
    // slot of the key to empty if it finds it.
    static bool try_del_from_bucket(TableInfo* ti, const partial_t partial,
                                    const key_type &key, const size_t i) {
        for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
            if (!ti->buckets_[i].occupied(j)) {
                continue;
            }
            if (!is_simple && ti->buckets_[i].partial(j) != partial) {
                continue;
            }
            if (eqfn()(ti->buckets_[i].key(j), key)) {
                ti->buckets_[i].eraseKV(j);
                ti->num_deletes[counterid].num.fetch_add(
                    1, std::memory_order_relaxed);
                return true;
            }
        }
        return false;
    }

    // try_update_bucket will search the bucket for the given key and change its
    // associated value if it finds it.
    ENABLE_IF(static, value_copy_assignable, bool) try_update_bucket(
        TableInfo* ti, const partial_t partial,
        const key_type &key, const mapped_type &value, const size_t i) {
        for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
            if (!ti->buckets_[i].occupied(j)) {
                continue;
            }
            if (!is_simple && ti->buckets_[i].partial(j) != partial) {
                continue;
            }
            if (eqfn()(ti->buckets_[i].key(j), key)) {
                ti->buckets_[i].val(j) = value;
                return true;
            }
        }
        return false;
    }

    // try_update_bucket_fn will search the bucket for the given key and change
    // its associated value with the given function if it finds it.
    template <typename Updater>
    static bool try_update_bucket_fn(
        TableInfo* ti, const partial_t partial,
        const key_type &key, Updater fn, const size_t i) {
        for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
            if (!ti->buckets_[i].occupied(j)) {
                continue;
            }
            if (!is_simple && ti->buckets_[i].partial(j) != partial) {
                continue;
            }
            if (eqfn()(ti->buckets_[i].key(j), key)) {
                fn(ti->buckets_[i].val(j));
                return true;
            }
        }
        return false;
    }

    // cuckoo_find searches the table for the given key and value, storing the
    // value in the val if it finds the key. It expects the locks to be taken
    // and released outside the function.
    ENABLE_IF(static, value_copy_assignable, cuckoo_status)
    cuckoo_find(const key_type& key, mapped_type& val,
                const size_t hv, const TableInfo* ti,
                const size_t i1, const size_t i2) {
        const partial_t partial = partial_key(hv);
        if (try_read_from_bucket(ti, partial, key, val, i1)) {
            return ok;
        }
        if (try_read_from_bucket(ti, partial, key, val, i2)) {
            return ok;
        }
        return failure_key_not_found;
    }

    // cuckoo_contains searches the table for the given key, returning true if
    // it's in the table and false otherwise. It expects the locks to be taken
    // and released outside the function.
    static bool cuckoo_contains(const key_type& key,
                                const size_t hv, const TableInfo* ti,
                                const size_t i1, const size_t i2) {
        const partial_t partial = partial_key(hv);
        if (check_in_bucket(ti, partial, key, i1)) {
            return true;
        }
        if (check_in_bucket(ti, partial, key, i2)) {
            return true;
        }
        return false;
    }

    // cuckoo_insert tries to insert the given key-value pair into an empty slot
    // in i1 or i2, performing cuckoo hashing if necessary. It expects the locks
    // to be taken outside the function, but they are released here, since
    // different scenarios require different handling of the locks. Before
    // inserting, it checks that the key isn't already in the table. cuckoo
    // hashing presents multiple concurrency issues, which are explained in the
    // function.
    template <class V>
    cuckoo_status cuckoo_insert(const key_type &key, V val,
                                const size_t hv, TableInfo* ti,
                                const size_t i1, const size_t i2) {
        int res1, res2;
        const partial_t partial = partial_key(hv);
        if (!try_find_insert_bucket(ti, partial, key, i1, res1)) {
            unlock_two(ti, i1, i2);
            return failure_key_duplicated;
        }
        if (!try_find_insert_bucket(ti, partial, key, i2, res2)) {
            unlock_two(ti, i1, i2);
            return failure_key_duplicated;
        }
        if (res1 != -1) {
            add_to_bucket(ti, partial, key, std::forward<V>(val), i1, res1);
            unlock_two(ti, i1, i2);
            return ok;
        }
        if (res2 != -1) {
            add_to_bucket(ti, partial, key, std::forward<V>(val), i2, res2);
            unlock_two(ti, i1, i2);
            return ok;
        }

        // we are unlucky, so let's perform cuckoo hashing
        size_t insert_bucket = 0;
        size_t insert_slot = 0;
        cuckoo_status st = run_cuckoo(ti, i1, i2, insert_bucket, insert_slot);
        if (st == failure_under_expansion) {
            // The run_cuckoo operation operated on an old version of the table,
            // so we have to try again. We signal to the calling insert method
            // to try again by returning failure_under_expansion.
            return failure_under_expansion;
        } else if (st == ok) {
            assert(!ti->locks_[lock_ind(i1)].try_lock());
            assert(!ti->locks_[lock_ind(i2)].try_lock());
            assert(!ti->buckets_[insert_bucket].occupied(insert_slot));
            assert(insert_bucket == index_hash(ti, hv) ||
                   insert_bucket == alt_index(ti, hv, index_hash(ti, hv)));
            // Since we unlocked the buckets during run_cuckoo, another insert
            // could have inserted the same key into either i1 or i2, so we
            // check for that before doing the insert.
            if (cuckoo_contains(key, hv, ti, i1, i2)) {
                unlock_two(ti, i1, i2);
                return failure_key_duplicated;
            }
            add_to_bucket(ti, partial, key, std::forward<V>(val),
                          insert_bucket, insert_slot);
            unlock_two(ti, i1, i2);
            return ok;
        }
        assert(st == failure);
        LIBCUCKOO_DBG("hash table is full (hashpower = %zu, hash_items = %zu,"
                      "load factor = %.2f), need to increase hashpower\n",
                      ti->hashpower_, cuckoo_size(ti), cuckoo_loadfactor(ti));
        return failure_table_full;
    }

    // We run cuckoo_insert in a loop until it succeeds in insert and upsert, so
    // we pulled out the loop to avoid duplicating it. This should be called
    // directly after snapshot_and_lock_two, and by the end of the function, the
    // hazard pointer will have been unset.
    template <class V>
    bool cuckoo_insert_loop(const key_type& key, V val,
                            size_t hv, TableInfo* ti, size_t i1, size_t i2) {
        cuckoo_status st = cuckoo_insert(key, std::forward<V>(val),
                                         hv, ti, i1, i2);
        while (st != ok) {
            // If the insert failed with failure_key_duplicated, it returns here
            if (st == failure_key_duplicated) {
                return false;
            }
            // If it failed with failure_under_expansion, the insert operated on
            // an old version of the table, so we just try again. If it's
            // failure_table_full, we have to expand the table before trying
            // again.
            if (st == failure_table_full) {
                if (cuckoo_expand_simple(ti->hashpower_ + 1, true) ==
                    failure_under_expansion) {
                    LIBCUCKOO_DBG("expansion is on-going\n");
                }
            }
            std::tie(ti, i1, i2) = snapshot_and_lock_two(hv);
            st = cuckoo_insert(key, std::forward<V>(val), hv, ti, i1, i2);
        }
        return true;
    }

    // cuckoo_delete searches the table for the given key and sets the slot with
    // that key to empty if it finds it. It expects the locks to be taken and
    // released outside the function.
    cuckoo_status cuckoo_delete(const key_type &key, const size_t hv,
                                TableInfo* ti, const size_t i1,
                                const size_t i2) {
        const partial_t partial = partial_key(hv);
        if (try_del_from_bucket(ti, partial, key, i1)) {
            return ok;
        }
        if (try_del_from_bucket(ti, partial, key, i2)) {
            return ok;
        }
        return failure_key_not_found;
    }

    // cuckoo_update searches the table for the given key and updates its value
    // if it finds it. It expects the locks to be taken and released outside the
    // function.
    ENABLE_IF(, value_copy_assignable, cuckoo_status)
    cuckoo_update(const key_type &key, const mapped_type &val,
                  const size_t hv, TableInfo* ti,
                  const size_t i1, const size_t i2) {
        const partial_t partial = partial_key(hv);
        if (try_update_bucket(ti, partial, key, val, i1)) {
            return ok;
        }
        if (try_update_bucket(ti, partial, key, val, i2)) {
            return ok;
        }
        return failure_key_not_found;
    }

    // cuckoo_update_fn searches the table for the given key and runs the given
    // function on its value if it finds it, assigning the result of the
    // function to the value. It expects the locks to be taken and released
    // outside the function.
    template <typename Updater>
    cuckoo_status cuckoo_update_fn(const key_type &key, Updater fn,
                                   const size_t hv, TableInfo* ti,
                                   const size_t i1, const size_t i2) {
        const partial_t partial = partial_key(hv);
        if (try_update_bucket_fn(ti, partial, key, fn, i1)) {
            return ok;
        }
        if (try_update_bucket_fn(ti, partial, key, fn, i2)) {
            return ok;
        }
        return failure_key_not_found;
    }

    // cuckoo_clear empties the table, calling the destructors of all the
    // elements it removes from the table. It assumes the locks are taken as
    // necessary.
    cuckoo_status cuckoo_clear(TableInfo* ti) {
        const size_t num_buckets = ti->buckets_.size();
        ti->buckets_.clear();
        ti->buckets_.resize(num_buckets);
        for (size_t i = 0; i < ti->num_inserts.size(); ++i) {
            ti->num_inserts[i].num.store(0);
            ti->num_deletes[i].num.store(0);
        }
        return ok;
    }

    // cuckoo_size returns the number of elements in the given table.
    size_t cuckoo_size(const TableInfo* ti) const {
        size_t inserts = 0;
        size_t deletes = 0;
        for (size_t i = 0; i < ti->num_inserts.size(); ++i) {
            inserts += ti->num_inserts[i].num.load();
            deletes += ti->num_deletes[i].num.load();
        }
        return inserts-deletes;
    }

    // cuckoo_loadfactor returns the load factor of the given table.
    double cuckoo_loadfactor(const TableInfo* ti) const {
        return static_cast<double>(cuckoo_size(ti)) / SLOT_PER_BUCKET /
            hashsize(ti->hashpower_);
    }

    // insert_into_table is a helper function used by cuckoo_expand_simple to
    // fill up the new table.
    static void insert_into_table(
        cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>& new_map,
        const TableInfo* old_ti, size_t i, size_t end) {
        for (;i < end; ++i) {
            for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
                if (old_ti->buckets_[i].occupied(j)) {
                    new_map.insert(
                        old_ti->buckets_[i].key(j),
                        std::move((mapped_type&)old_ti->buckets_[i].val(j)));
                }
            }
        }
    }

    // cuckoo_expand_simple will resize the table to at least the given
    // new_hashpower. If is_expansion is true, new_hashpower must be greater
    // than the current size of the table. If it's false, then new_hashpower
    // must be less. When we're shrinking the table, if the current table
    // contains more elements than can be held by new_hashpower, the resulting
    // hashpower will be greater than new_hashpower. It needs to take all the
    // bucket locks, since no other operations can change the table during
    // expansion.
    cuckoo_status cuckoo_expand_simple(size_t new_hashpower,
                                       bool is_expansion) {
        TableInfo* ti = snapshot_and_lock_all();
        assert(ti == table_info.load());
        AllUnlocker au(ti);
        HazardPointerUnsetter hpu;
        if ((is_expansion && new_hashpower <= ti->hashpower_) ||
            (!is_expansion && new_hashpower >= ti->hashpower_)) {
            // Most likely another expansion ran before this one could grab the
            // locks
            return failure_under_expansion;
        }

        // Creates a new hash table with hashpower new_hashpower and adds all
        // the elements from the old buckets
        cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET> new_map(
            hashsize(new_hashpower) * SLOT_PER_BUCKET);
        const size_t threadnum = kNumCores();
        const size_t buckets_per_thread =
            hashsize(ti->hashpower_) / threadnum;
        std::vector<std::thread> insertion_threads(threadnum);
        for (size_t i = 0; i < threadnum-1; ++i) {
            insertion_threads[i] = std::thread(
                insert_into_table, std::ref(new_map),
                ti, i*buckets_per_thread, (i+1)*buckets_per_thread);
        }
        insertion_threads[threadnum-1] = std::thread(
            insert_into_table, std::ref(new_map), ti,
            (threadnum-1)*buckets_per_thread, hashsize(ti->hashpower_));
        for (size_t i = 0; i < threadnum; ++i) {
            insertion_threads[i].join();
        }
        // Sets this table_info to new_map's. It then sets new_map's
        // table_info to nullptr, so that it doesn't get deleted when
        // new_map goes out of scope
        table_info.store(new_map.table_info.load());
        new_map.table_info.store(nullptr);

        // Rather than deleting ti now, we store it in old_table_infos. We then
        // run a delete_unused routine to delete all the old table pointers.
        old_table_infos.push_back(std::move(std::unique_ptr<TableInfo>(ti)));
        global_hazard_pointers.delete_unused(old_table_infos);
        return ok;
    }

    // Iterator definitions
    friend class const_iterator;
    friend class iterator;

public:
    //! A const_iterator is an iterator through the table that is thread safe.
    //! For the duration of its existence, it takes all the locks on the table
    //! it is given, thereby ensuring that no other threads can modify the table
    //! while the iterator is in use. Note that this also means that only one
    //! iterator can be active on a table at one time and furthermore that all
    //! operations on the table, except the \ref size, \ref empty, \ref
    //! hashpower, \ref bucket_count, and \ref load_factor methods, will stall
    //! until the iterator loses its lock. For this reason, we suggest using the
    //! \ref snapshot_table method if possible, since it is less error-prone.
    //! The iterator allows movement forward and backward through the table as
    //! well as dereferencing items in the table. It maintains the invariant
    //! that the iterator is either an end iterator (which points past the end
    //! of the table), or points to a filled slot. As soon as the iterator
    //! looses its lock on the table, all dereference and movement operations
    //! will throw an exception.
    class const_iterator {
        // The constructor locks the entire table, retrying until
        // snapshot_and_lock_all succeeds. Then it calculates end_pos and
        // begin_pos and sets index and slot to the beginning or end of the
        // table, based on the boolean argument. We keep this constructor
        // private (but expose it to the cuckoohash_map class), since we don't
        // want users calling it.
        const_iterator(
            const cuckoohash_map<Key, T, Hash, Pred, Alloc,
            SLOT_PER_BUCKET>& hm, bool is_end) : hm_(hm) {
            cuckoohash_map<Key, T, Hash, Pred, Alloc,
                           SLOT_PER_BUCKET>::check_hazard_pointer();
            ti_ = hm_.snapshot_and_lock_all();
            assert(ti_ == hm_.table_info.load());

            has_table_lock = true;

            index_ = slot_ = 0;

            set_end(end_pos.first, end_pos.second);
            set_begin(begin_pos.first, begin_pos.second);
            if (is_end) {
                index_ = end_pos.first;
                slot_ = end_pos.second;
            } else {
                index_ = begin_pos.first;
                slot_ = begin_pos.second;
            }
        }

        friend class cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>;

    public:
        //! This is an rvalue-reference constructor that takes the lock from \p
        //! it and copies its state. To create an iterator from scratch, call
        //! the \ref cbegin or \ref cend methods of cuckoohash_map.
        const_iterator(const_iterator&& it)
            : hm_(it.hm_) {
            if (this == &it) {
                return;
            }
            memcpy(this, &it, sizeof(const_iterator));
            it.has_table_lock = false;
        }

        //! The assignment operator behaves identically to the rvalue-reference
        //! constructor.
        const_iterator* operator=(const_iterator&& it) {
            if (this == &it) {
                return this;
            }
            memcpy(this, &it, sizeof(const_iterator));
            it.has_table_lock = false;
            return this;
        }

        //! release unlocks the table, thereby freeing it up for other
        //! operations, but also invalidating all future operations with this
        //! iterator.
        void release() {
            if (has_table_lock) {
                AllUnlocker au(ti_);
                cuckoohash_map<Key, T, Hash, Pred, Alloc,
                               SLOT_PER_BUCKET>::HazardPointerUnsetter hpu;
                has_table_lock = false;
            }
        }

        //! The destructor simply calls \ref release.
        ~const_iterator() {
            release();
        }

        //! is_end returns true if the iterator is at end_pos, which means it is
        //! past the end of the table.
        bool is_end() const {
            return (index_ == end_pos.first && slot_ == end_pos.second);
        }

        //! is_begin returns true if the iterator is at begin_pos, which means
        //! it is at the first item in the table.
        bool is_begin() const {
            return (index_ == begin_pos.first && slot_ == begin_pos.second);
        }

    protected:
        // For the arrow dereference operator, we return a pointer to a
        // lightweight pair consisting of const references to the key and value
        // under the iterator.
        typedef std::pair<const Key&, const T&> ref_pair;
        // Since we can't initialize a ref_pair before knowing what it points
        // to, we use std::aligned_storage to reserve unititialized space for
        // the object, which we then construct with placement new. Since this
        // isn't really part of the logical iterator state, we make it mutable.
        mutable typename std::aligned_storage<sizeof(ref_pair),
                                              alignof(ref_pair)>::type data;

    public:
        //! The dereference operator returns a value_type copied from the
        //! key-value pair under the iterator.
        value_type operator*() const {
            check_lock();
            if (is_end()) {
                throw end_dereference;
            }
            assert(ti_->buckets_[index_].occupied(slot_));
            return {ti_->buckets_[index_].key(slot_),
                    ti_->buckets_[index_].val(slot_)};
        }

        //! The arrow dereference operator returns a pointer to an internal
        //! std::pair which contains const references to the key and value
        //! under the iterator.
        ref_pair* operator->() const {
            check_lock();
            if (is_end()) {
                throw end_dereference;
            }
            assert(ti_->buckets_[index_].occupied(slot_));
            ref_pair* data_ptr =
                static_cast<ref_pair*>(static_cast<void*>(&data));
            new (data_ptr) ref_pair(ti_->buckets_[index_].key(slot_),
                                    ti_->buckets_[index_].val(slot_));
            return data_ptr;
        }

        //! The prefix increment operator moves the iterator forwards to the
        //! next nonempty slot. If it reaches the end of the table, it becomes
        //! an end iterator. It throws an exception if the iterator is already
        //! at the end of the table.
        const_iterator* operator++() {
            check_lock();
            if (is_end()) {
                throw end_increment;
            }
            forward_filled_slot(index_, slot_);
            return this;
        }

        //! The postfix increment operator behaves identically to the prefix
        //! increment operator.
        const_iterator* operator++(int) {
            check_lock();
            if (is_end()) {
                throw end_increment;
            }
            forward_filled_slot(index_, slot_);
            return this;
        }

        //! The prefix decrement operator moves the iterator backwards to the
        //! previous nonempty slot. If we aren't at the beginning, then the
        //! backward_filled_slot operation should not fail. If we are, it throws
        //! an exception.
        const_iterator* operator--() {
            check_lock();
            if (is_begin()) {
                throw begin_decrement;
            }
            backward_filled_slot(index_, slot_);
            return this;
        }

        //! The postfix decrement operator behaves identically to the prefix
        //! decrement operator.
        const_iterator* operator--(int) {
            check_lock();
            if (is_begin()) {
                throw begin_decrement;
            }
            backward_filled_slot(index_, slot_);
            return this;
        }

    protected:
        // A pointer to the associated hashmap
        const cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>& hm_;

        // The hashmap's table info
        typename cuckoohash_map<Key, T, Hash, Pred, Alloc,
                                SLOT_PER_BUCKET>::TableInfo* ti_;

        // Indicates whether the iterator has the table lock
        bool has_table_lock;

        // Stores the bucket and slot of the end iterator, which is one past the
        // end of the table. It is initialized during the iterator's
        // constructor.
        std::pair<size_t, size_t> end_pos;

        // Stotres the bucket and slot of the begin iterator, which is the first
        // filled position in the table. It is initialized during the iterator's
        // constructor. If the table is empty, it points past the end of the
        // table, to the same position as end_pos.
        std::pair<size_t, size_t> begin_pos;

        // The bucket index of the item being pointed to
        size_t index_;

        // The slot in the bucket of the item being pointed to
        size_t slot_;

        // set_end sets the given index and slot to one past the last position
        // in the table.
        void set_end(size_t& index, size_t& slot) {
            index = hm_.bucket_count();
            slot = 0;
        }

        // set_begin sets the given pair to the position of the first element in
        // the table.
        void set_begin(size_t& index, size_t& slot) {
            if (hm_.empty()) {
                set_end(index, slot);
            } else {
                index = slot = 0;
                // There must be a filled slot somewhere in the table
                if (!ti_->buckets_[index].occupied(slot)) {
                    forward_filled_slot(index, slot);
                    assert(!is_end());
                }
            }
        }

        // forward_slot moves the given index and slot to the next available
        // slot in the forwards direction. It returns true if it successfully
        // advances, and false if it has reached the end of the table, in which
        // case it sets index and slot to end_pos.
        bool forward_slot(size_t& index, size_t& slot) {
            if (slot < SLOT_PER_BUCKET-1) {
                ++slot;
                return true;
            } else if (index < hm_.bucket_count()-1) {
                ++index;
                slot = 0;
                return true;
            } else {
                set_end(index, slot);
                return false;
            }
        }

        // backward_slot moves index and slot to the next available slot in the
        // backwards direction. It returns true if it successfully advances, and
        // false if it has reached the beginning of the table, setting the index
        // and slot back to begin_pos.
        bool backward_slot(size_t& index, size_t& slot) {
            if (slot > 0) {
                --slot;
                return true;
            } else if (index > 0) {
                --index;
                slot = SLOT_PER_BUCKET-1;
                return true;
            } else {
                set_begin(index, slot);
                return false;
            }
        }

        // forward_filled_slot moves index and slot to the next filled slot.
        bool forward_filled_slot(size_t& index, size_t& slot) {
            bool res = forward_slot(index, slot);
            if (!res) {
                return false;
            }
            while (!ti_->buckets_[index].occupied(slot)) {
                res = forward_slot(index, slot);
                if (!res) {
                    return false;
                }
            }
            return true;
        }

        // backward_filled_slot moves index and slot to the previous filled
        // slot.
        bool backward_filled_slot(size_t& index, size_t& slot) {
            bool res = backward_slot(index, slot);
            if (!res) {
                return false;
            }
            while (!ti_->buckets_[index].occupied(slot)) {
                res = backward_slot(index, slot);
                if (!res) {
                    return false;
                }
            }
            return true;
        }


        // check_lock throws an exception if the iterator doesn't have a
        // lock.
        void check_lock() const {
            if (!has_table_lock) {
                throw std::runtime_error(
                    "Iterator does not have a lock on the table");
            }
        }

        // Other error messages
        static const std::out_of_range end_dereference;
        static const std::out_of_range end_increment;
        static const std::out_of_range begin_decrement;
    };


    //! An iterator supports the same operations as the const_iterator and
    //! provides an additional \ref set_value method to allow changing values in
    //! the table.
    class iterator : public const_iterator {
        // This constructor does the same thing as the private const_iterator
        // one.
        iterator(cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>& hm,
                 bool is_end)
            : const_iterator(hm, is_end) {}

        friend class cuckoohash_map<Key, T, Hash, Pred, Alloc, SLOT_PER_BUCKET>;

    public:
        //! This constructor is identical to the rvalue-reference constructor of
        //! const_iterator.
        iterator(iterator&& it)
            : const_iterator(std::move(it)) {}

        //! This constructor allows converting from a const_iterator to an
        //! iterator.
        iterator(const_iterator&& it)
            : const_iterator(std::move(it)) {}

        // The assignment operator behaves identically to the rvalue-reference
        // constructor.
        iterator* operator=(iterator&& it) {
            if (this == &it) {
                return this;
            }
            memcpy(this, &it, sizeof(iterator));
            it.has_table_lock = false;
            return this;
        }

        //! set_value sets the value pointed to by the iterator to \p val. This
        //! involves modifying the hash table itself, but since we have a lock
        //! on the table, we are okay. We are only changing the value in the
        //! bucket, so the element will retain it's position in the table.
        void set_value(const mapped_type val) {
            this->check_lock();
            if (this->is_end()) {
                throw this->end_dereference;
            }
            assert(this->ti_->buckets_[this->index_].occupied(this->slot_));
            this->ti_->buckets_[this->index_].val(this->slot_) = val;
        }
    };

// Public iterator functions
public:
    //! cbegin returns a const_iterator to the first filled slot in the
    //! table.
    const_iterator cbegin() const {
        return const_iterator(*this, false);
    }

    //! cend returns a const_iterator set past the end of the table.
    const_iterator cend() const {
        return const_iterator(*this, true);
    }

    //! begin returns an iterator to the first filled slot in the table.
    iterator begin() {
        return iterator(*this, false);
    }

    //! end returns an iterator set past the end of the table.
    iterator end() {
        return iterator(*this, true);
    }

    //! snapshot_table allocates a vector and, using a const_iterator stores all
    //! the elements currently in the table.
    std::vector<value_type> snapshot_table() const {
        std::vector<value_type> items;
        items.reserve(size());
        for (auto it = cbegin(); !it.is_end(); ++it) {
            items.push_back(*it);
        }
        return items;
    }

    // This class is a friend for unit testing
    friend class UnitTestInternalAccess;
};

// Initializing the static members
template <class Key, class T, class Hash, class Pred, class Alloc, size_t SPB>
    __thread typename cuckoohash_map<Key, T, Hash, Pred, Alloc,
                                     SPB>::TableInfo**
    cuckoohash_map<Key, T, Hash, Pred, Alloc, SPB>::hazard_pointer = nullptr;

template <class Key, class T, class Hash, class Pred, class Alloc, size_t SPB>
    __thread int cuckoohash_map<Key, T, Hash, Pred, Alloc, SPB>::counterid = -1;

template <class Key, class T, class Hash, class Pred, class Alloc, size_t SPB>
    typename cuckoohash_map<Key, T, Hash, Pred, Alloc,
                            SPB>::GlobalHazardPointerList
    cuckoohash_map<Key, T, Hash, Pred, Alloc, SPB>::global_hazard_pointers;

template <class Key, class T, class Hash, class Pred, class Alloc, size_t SPB>
const std::out_of_range cuckoohash_map<
    Key, T, Hash, Pred, Alloc, SPB>::const_iterator::end_dereference(
        "Cannot dereference: iterator points past the end of the table");

template <class Key, class T, class Hash, class Pred, class Alloc, size_t SPB>
    const std::out_of_range cuckoohash_map<
    Key, T, Hash, Pred, Alloc, SPB>::const_iterator::end_increment(
        "Cannot increment: iterator points past the end of the table");

template <class Key, class T, class Hash, class Pred, class Alloc, size_t SPB>
    const std::out_of_range cuckoohash_map<
    Key, T, Hash, Pred, Alloc, SPB>::const_iterator::begin_decrement(
        "Cannot decrement: iterator points to the beginning of the table");

#endif // _CUCKOOHASH_MAP_HH
