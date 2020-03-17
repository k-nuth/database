// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_
#define KTH_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>
#include <kth/database/legacy/result/transaction_unconfirmed_result.hpp>
#include <kth/database/legacy/primitives/slab_hash_table.hpp>
#include <kth/database/legacy/primitives/slab_manager.hpp>

// #ifdef KTH_DB_UNSPENT_LEGACY
// #include <kth/database/unspent_outputs.hpp>
// #endif // KTH_DB_UNSPENT_LEGACY


namespace kth {
namespace database {

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API transaction_unconfirmed_database
{
public:
    typedef boost::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Sentinel for use in tx position to indicate unconfirmed.
    static const size_t unconfirmed;

    /// Construct the database.
    transaction_unconfirmed_database(const path& map_filename, size_t buckets,
        size_t expansion, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~transaction_unconfirmed_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Fetch transaction by its hash, at or below the specified block height.
    transaction_unconfirmed_result get(hash_digest const& hash) const;

    /// Store a transaction in the database.
    void store(const chain::transaction& tx);


    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush() const;

    bool unlink(hash_digest const& hash);
    bool unlink_if_exists(hash_digest const& hash);

//    template <typename UnaryFunction>
        //requires Domain of UnaryFunction is chain::transaction
//    void for_each(UnaryFunction f) const;
    template <typename UnaryFunction>
    void for_each(UnaryFunction f) const {
        lookup_map_.for_each([&f](memory_ptr slab){
            if (slab != nullptr) {
                transaction_unconfirmed_result res(slab);
                auto tx = res.transaction();
                tx.recompute_hash();
                return f(tx);
            } else {
                std::cout << "transaction_unconfirmed_database::for_each nullptr slab\n";
            }
            return true;
        });
    }

    template <typename UnaryFunction>
    void for_each_result(UnaryFunction f) const {
        lookup_map_.for_each([&f](memory_ptr slab){
            if (slab != nullptr) {
                transaction_unconfirmed_result res(slab);
                return f(res);
            } else {
                std::cout << "transaction_unconfirmed_database::for_each nullptr slab\n";
            }
            return true;
        });
    }

private:
    typedef slab_hash_table<hash_digest> slab_map;

    memory_ptr find(hash_digest const& hash) const;

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    // Hash table used for looking up txs by hash.
    memory_map lookup_file_;
    slab_hash_table_header lookup_header_;
    slab_manager lookup_manager_;
    slab_map lookup_map_;

    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace kth

#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#endif //KTH_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_
