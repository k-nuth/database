// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef KTH_DB_UNSPENT_LEGACY

#include <kth/database/unspent_transaction.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <kth/domain.hpp>

namespace kth::database {

using namespace kth::domain::chain;

unspent_transaction::unspent_transaction(unspent_transaction&& other)
  : height_(other.height_),
    median_time_past_(other.median_time_past_),
    is_coinbase_(other.is_coinbase_),
    is_confirmed_(other.is_confirmed_),
    hash_(std::move(other.hash_)),
    outputs_(other.outputs_)
{}

unspent_transaction::unspent_transaction(const unspent_transaction& other)
  : height_(other.height_),
    median_time_past_(other.median_time_past_),
    is_coinbase_(other.is_coinbase_),
    is_confirmed_(other.is_confirmed_),
    hash_(other.hash_),
    outputs_(other.outputs_)
{
}

unspent_transaction::unspent_transaction(hash_digest const& hash)
  : height_(0),
    median_time_past_(0),
    is_coinbase_(false),
    is_confirmed_(false),
    hash_(hash),
    outputs_(std::make_shared<output_map>())
{
}

unspent_transaction::unspent_transaction(const output_point& point)
  : unspent_transaction(point.hash())
{
}

unspent_transaction::unspent_transaction(const domain::chain::transaction& tx,
    size_t height, uint32_t median_time_past, bool confirmed)
  : height_(height),
    median_time_past_(median_time_past),
    is_coinbase_(tx.is_coinbase()),
    is_confirmed_(confirmed),
    hash_(tx.hash()),
    outputs_(std::make_shared<output_map>())
{
    auto const& outputs = tx.outputs();
    auto const size = safe_unsigned<uint32_t>(outputs.size());
    outputs_->reserve(size);

    // TODO: consider eliminating the byte buffer in favor of ops::list only.
    for (uint32_t index = 0; index < size; ++index)
        (*outputs_)[index] = outputs[index];
}

hash_digest const& unspent_transaction::hash() const {
    return hash_;
}

size_t unspent_transaction::height() const {
    return height_;
}

uint32_t unspent_transaction::median_time_past() const {
    return median_time_past_;
}

bool unspent_transaction::is_coinbase() const {
    return is_coinbase_;
}

bool unspent_transaction::is_confirmed() const {
    return is_confirmed_;
}

unspent_transaction::output_map_ptr unspent_transaction::outputs() const {
    return outputs_;
}

// For the purpose of bimap identity only the tx hash matters.
bool unspent_transaction::operator==(const unspent_transaction& other) const {
    return hash_ == other.hash_;
}

unspent_transaction& unspent_transaction::operator=(
    unspent_transaction&& other)
{
    height_ = other.height_;
    median_time_past_ = other.median_time_past_;
    is_coinbase_ = other.is_coinbase_;
    is_confirmed_ = other.is_confirmed_;
    hash_ = std::move(other.hash_);
    outputs_ = other.outputs_;
    return *this;
}

unspent_transaction& unspent_transaction::operator=(
    const unspent_transaction& other)
{
    height_ = other.height_;
    median_time_past_ = other.median_time_past_;
    is_coinbase_ = other.is_coinbase_;
    is_confirmed_ = other.is_confirmed_;
    hash_ = other.hash_;
    outputs_ = other.outputs_;
    return *this;
}

} // namespace kth::database

#endif // KTH_DB_UNSPENT_LEGACY
