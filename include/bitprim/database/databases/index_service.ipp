/**
 * Copyright (c) 2016-2018 Bitprim Inc.
 *
 * This file is part of Bitprim.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef BITPRIM_DATABASE_INDEX_SERVICE_IPP_
#define BITPRIM_DATABASE_INDEX_SERVICE_IPP_

namespace libbitcoin {
namespace database {


#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
result_code internal_database_basis<Clock>::start_indexing() {

    if (is_indexing_end()) {
        LOG_DEBUG(LOG_DATABASE) << "Error starting indexing because the process is already done";
        return result_code::other;
    }

    //get last height indexed
    auto last_indexed = get_last_indexed_height();
    LOG_DEBUG(LOG_DATABASE) << "Last indexed block is " << last_indexed;

    //get block from height
    auto last_height = get_last_height();
    LOG_DEBUG(LOG_DATABASE) << "Last height is " << last_height;

    while (last_indexed < last_height) {
        ++last_indexed;

        //process block
        index_block_height(last_indexed);

        //update last indexed height
        update_last_height_indexed(last_height);

        //delete raw block

        LOG_DEBUG(LOG_DATABASE) << "Indexed height " << last_height;
    }

    //mark end of indexing
    set_end_indexing();

    LOG_DEBUG(LOG_DATABASE) << "Finished indexing at height " << last_height;
}

template <typename Clock>
bool internal_database_basis<Clock>::is_indexing_end() {
    return true;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_end_indexing() {
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::update_last_height_indexed(uint32_t height) {

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::index_block_height(uint32_t height) {
    //get block by height
    auto const block = get_block(height);
    auto const& txs = block.transactions();

    uint32_t pos = 0;
    for (auto const& tx : txs) {

        //insert block_index
        insert_block_index(block);
        //insert transaction
        //TODO (Mario): Remove transaction unconfirmed ?????

        insert_index_transaction(tx);
    
        ++pos;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_block_index(chain::block block) {

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_index_transaction(chain::transaction tx) {

    return result_code::success;
}

template <typename Clock>
uint32_t internal_database_basis<Clock>::get_last_indexed_height() {
    return 0;
} 

#endif

} // namespace database
} // namespace libbitcoin


#endif // BITPRIM_DATABASE_INDEX_SERVICE_IPP_
