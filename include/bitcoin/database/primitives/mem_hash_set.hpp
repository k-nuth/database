/**
 * Copyright (c) 2016 Bitprim developers (see AUTHORS)
 *
 * This file is part of Bitprim.
 *
 * Bitprim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBBITCOIN_DATABASE_MEM_HASH_SET_HPP
#define LIBBITCOIN_DATABASE_MEM_HASH_SET_HPP

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
// #include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>

#include <functional>
#include <memory>
// #include <fstream>


namespace libbitcoin {
namespace database {

//TODO Fer: Use Boost FileSystem

// size_t file_size(std::string const& filename) {
//    std::ifstream ifile(filename);
//    ifile.seekg(0, std::ios_base::end);
//    return ifile.tellg();
// }

// namespace mem_hash {
//    struct construct_tag {};
//    struct find_tag {};
// }


template <typename Key, typename Hash = boost::hash<Key>,
         typename KeyEqual = std::equal_to<Key>>
         //,typename Allocator = std::allocator<std::pair<const Key, T>
class mem_hash_set {
public:

   using key_type = Key;
   using value_type = Key;
   using hasher = Hash;
   using key_equal = KeyEqual;
   using allocator_type = boost::interprocess::allocator<value_type, boost::interprocess::managed_mapped_file::segment_manager>;

   using cont_type = boost::unordered_set<key_type, hasher, key_equal, allocator_type>;

   using size_type = typename cont_type::size_type;
   using difference_type = typename cont_type::difference_type;
   using reference = typename cont_type::reference;
   using const_reference = typename cont_type::const_reference;
   using pointer = typename cont_type::pointer;
   using const_pointer = typename cont_type::const_pointer;
   using iterator = typename cont_type::iterator;
   using const_iterator = typename cont_type::const_iterator;
   using local_iterator = typename cont_type::local_iterator;
   using const_local_iterator = typename cont_type::const_local_iterator;

   mem_hash_set(std::string const& filename, std::string const& mapname, std::size_t bucket_count, std::size_t initial_file_size) 
      : filename_(filename)
      , mapname_(mapname)
   {
      if (boost::filesystem::exists(filename_)) {
         file_size_ = boost::filesystem::file_size(filename_);
         mfile_ptr_.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, filename_.c_str(), file_size_));
         cont_ = mfile_ptr_->find<cont_type>(mapname_.c_str()).first;
      } else {
         mfile_ptr_.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, filename_.c_str(), initial_file_size));
         file_size_ = boost::filesystem::file_size(filename_);
         cont_ = mfile_ptr_->construct<cont_type>(mapname_.c_str())(bucket_count, hasher(), key_equal(), mfile_ptr_->get_allocator<value_type>());
      }
   }

   size_type size() const {
      return cont_->size();
   }

   iterator begin() {
      return cont_->begin();
   }

   const_iterator begin() const {
      return cont_->begin();
   }
   
   const_iterator cbegin() const {
      return cont_->cbegin();
   }

   iterator end() {
      return cont_->end();
   }

   const_iterator end() const {
      return cont_->end();
   }
   
   const_iterator cend() const {
      return cont_->cend();
   }

   size_type bucket_count() const {
      return cont_->bucket_count();
   }

   std::pair<iterator, bool> insert(value_type const& x) {
      //TODO: Fer: Hardcoded constants

      auto _25p = file_size_ * 0.1; //0.25;
      
      if (mfile_ptr_->get_free_memory() < _25p) {
         
         // std::cout << "get_free_memory(): " << mfile_ptr_->get_free_memory(); // << std::endl;
         // std::cout << "_25p: " << _25p << std::endl;

         mfile_ptr_.reset(nullptr);

         auto requested_size = file_size_ * 0.5;
         // std::cout << "requested_size: " << requested_size << std::endl;

         boost::interprocess::managed_mapped_file::grow(filename_.c_str(), requested_size);
         file_size_ = boost::filesystem::file_size(filename_);
         mfile_ptr_.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, filename_.c_str(), file_size_));
         cont_ = mfile_ptr_->find<cont_type>(mapname_.c_str()).first;

         // std::cout << "file_size_: " << file_size_ << std::endl;

         // std::cout << " - " << mfile_ptr_->get_free_memory() 
         //           << " - file_size: " << file_size_
         //           << '\n';
      }

      assert(mfile_ptr_->get_free_memory() >= _25p);

      return cont_->insert(x);
   }

   size_type erase(key_type const& key) {
      return cont_->erase(key);  
   }

   size_type count(key_type const& key) const {
      return cont_->count(key);     
   }

   //TODO Fer
   size_t get_free_memory() const {
      return mfile_ptr_->get_free_memory();
   }

   bool flush() {  
      return mfile_ptr_->flush();  
   }
   
private:
   std::string filename_;
   std::string mapname_;
   std::size_t file_size_;
   std::unique_ptr<boost::interprocess::managed_mapped_file> mfile_ptr_;
   cont_type* cont_;
};

} // namespace database
} // namespace libbitcoin

#endif /*LIBBITCOIN_DATABASE_MEM_HASH_SET_HPP*/




// // --------------------------------------------------------------------------------


// #include <array>

// template <size_t Size>
// using byte_array = std::array<uint8_t, Size>;

// struct point {
//    byte_array<32> hash;
//    uint32_t index;
// };

// bool operator==(point const& a, point const& b) {
//     return a.hash == b.hash && a.index == b.index;
// }

// bool operator!=(point const& a, point const& b) {
//     return !(a == b);
// }


// namespace std {

// template <size_t Size>
// struct hash<byte_array<Size>> {
//     size_t operator()(byte_array<Size> const& hash) const {
//         return boost::hash_range(hash.begin(), hash.end());
//     }
// };
// } // namespace std

// namespace boost {

// template <size_t Size>
// struct hash<byte_array<Size>> {
//     size_t operator()(byte_array<Size> const& hash) const {
//         return boost::hash_range(hash.begin(), hash.end());
//     }
// };
// } // namespace boost

// namespace std {

// template <>
// struct hash<point> {
//     size_t operator()(point const& point) const {
//         size_t seed = 0;
//         boost::hash_combine(seed, point.hash);
//         boost::hash_combine(seed, point.index);
//         return seed;
//     }
// };

// // // Extend std namespace with the size of our point, used as database key size.
// // template <>
// // struct tuple_size<chain::point>
// // {
// //     static const size_t value = std::tuple_size<hash_digest>::value + sizeof(uint32_t);

// //     operator std::size_t() const {
// //         return value;
// //     }
// // };

// } // namespace std

// namespace boost {

// template <>
// struct hash<point> {
//     // Changes to this function invalidate existing database files.
//     size_t operator()(point const& point) const {
//         size_t seed = 0;
//         boost::hash_combine(seed, point.hash);
//         boost::hash_combine(seed, point.index);
//         return seed;
//     }
// };
// } // namespace boost


// int main () {


//    std::string filename = "MyMappedFile";
//    std::string mapname = "MyHashMap";

//    // using map_type = mem_hash_map<int, float>;
//    using map_type = mem_hash_set<point>;

// #ifdef CONSTRUCT

//    constexpr int buckets = 45000000; //3;
//    constexpr int filesize = buckets * 12; //65536;

//    map_type xxx(filename, mapname, buckets, filesize, mem_hash::construct_tag());
//    // point x{{0}, 1};
//    // xxx.insert(x);

//    // using cont_type = boost::unordered_set<point>;

//    // std::cout << "press any key...\n";
//    // std::cin.get();

//    // cont_type xxx(45000000);
//    // std::cout << "xxx.bucket_count():     " << xxx.bucket_count() << std::endl;

//    // point x{{0}, 1};
//    // xxx.insert(x);


//    // std::cout << "press any key...\n";
//    // std::cin.get();



// #elif defined(CHECK)
//    map_type xxx(filename, mapname, mem_hash::find_tag());

//    std::cout << "xxx.size():     " << xxx.size() << std::endl;

//    int elements_to_insert = 46000000;

//    for (uint32_t i = 0; i < elements_to_insert; ++i){
//       point x{{0}, i};

//       auto res = xxx.count(x);

//       if (i % 100000 == 0) {
//          std::cout << "i: " << i << " - res: " << res << std::endl;
//       }
//    }

// #elif defined(REMOVE)

//    map_type xxx(filename, mapname, mem_hash::find_tag());

//    std::cout << "xxx.size():     " << xxx.size() << std::endl;

   
//    int max = 3000;
//    int blocks = 430000;

//    for (uint32_t j = 0; j < blocks; ++j){

//       uint32_t first = j * max;
//       uint32_t last = first + max;

//       for (uint32_t i = first; i < last; ++i){
//          point x{{0}, i};
//          xxx.insert(x);
//       }

//       for (uint32_t i = first; i < (last - 100); ++i){
//          point x{{0}, i};
//          xxx.erase(x);
//       }

//       if (j % 10000 == 0) {
//          std::cout << "xxx.size(): " << xxx.size() 
//                    << " - xxx.get_free_memory(): " << xxx.get_free_memory() 
//                    << std::endl;
//       }
//    }




// #else
//    map_type xxx(filename, mapname, mem_hash::find_tag());
//    std::cout << "xxx.bucket_count():     " << xxx.bucket_count() << std::endl;
//    std::cout << "xxx.size():     " << xxx.size() << std::endl;

//    auto next_elem = xxx.size();
//    int elements_to_insert = 45000000; //80000000;

//    for (uint32_t i = next_elem; i < next_elem + elements_to_insert; ++i){
//       xxx.insert(point{{0}, i});

//       if (i % 1000000 == 0) {
//          std::cout << "xxx.size():     " << xxx.size() << std::endl;
//       }
//    }
// #endif

//    return 0;
// }


// template <typename Key, typename T, typename Hash = boost::hash<Key>,
//          typename KeyEqual = std::equal_to<Key>>
//          //,typename Allocator = std::allocator<std::pair<const Key, T>
// class mem_hash_map {
// public:

//    using key_type = Key;
//    using mapped_type = T;
//    using value_type = std::pair<const key_type, mapped_type>;
//    using hasher = Hash;
//    using key_equal = KeyEqual;
//    using allocator_type = boost::interprocess::allocator<value_type, boost::interprocess::managed_mapped_file::segment_manager>;

//    using cont_type = boost::unordered_map<key_type, mapped_type, hasher, key_equal, allocator_type>;

//    using size_type = typename cont_type::size_type;
//    using difference_type = typename cont_type::difference_type;
//    using reference = typename cont_type::reference;
//    using const_reference = typename cont_type::const_reference;
//    using pointer = typename cont_type::pointer;
//    using const_pointer = typename cont_type::const_pointer;
//    using iterator = typename cont_type::iterator;
//    using const_iterator = typename cont_type::const_iterator;
//    using local_iterator = typename cont_type::local_iterator;
//    using const_local_iterator = typename cont_type::const_local_iterator;

//    mem_hash_map(std::string const& filename, std::string const& mapname, std::size_t bucket_count, std::size_t initial_file_size, mem_hash::construct_tag) 
//       : filename_(filename)
//       , mapname_(mapname)
//       , mfile_ptr_(new boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, filename_.c_str(), initial_file_size))
//    {
//       file_size_ = file_size(filename_);
//       cont_ = mfile_ptr_->construct<cont_type>(mapname_.c_str())(bucket_count, hasher(), key_equal(), mfile_ptr_->get_allocator<value_type>());
//    }


//    mem_hash_map(std::string const& filename, std::string const& mapname, mem_hash::find_tag) 
//       : filename_(filename)
//       , mapname_(mapname)
//       , file_size_(file_size(filename_))
//       , mfile_ptr_(new boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, filename_.c_str(), file_size_))
//    {
//       cont_ = mfile_ptr_->find<cont_type>(mapname_.c_str()).first;
//    }


//    size_type size() const {
//       return cont_->size();
//    }

//    iterator begin() {
//       return cont_->begin();
//    }

//    const_iterator begin() const {
//       return cont_->begin();
//    }
   
//    const_iterator cbegin() const {
//       return cont_->cbegin();
//    }

//    iterator end() {
//       return cont_->end();
//    }

//    const_iterator end() const {
//       return cont_->end();
//    }
   
//    const_iterator cend() const {
//       return cont_->cend();
//    }

//    size_type bucket_count() const {
//       return cont_->bucket_count();
//    }

//    std::pair<iterator, bool> insert(value_type const& x) {
//       //TODO: Fer: Hardcoded constants

//       auto _25p = file_size_ * 0.25;
      
//       if (mfile_ptr_->get_free_memory() < _25p) {
         
//          std::cout << "get_free_memory(): " << mfile_ptr_->get_free_memory() << std::endl;
//          // std::cout << "_25p: " << _25p << std::endl;

//          mfile_ptr_.reset(nullptr);

//          auto requested_size = file_size_ * 0.5;
//          //std::cout << "requested_size: " << requested_size << std::endl;

//          boost::interprocess::managed_mapped_file::grow(filename_.c_str(), requested_size);
//          file_size_ = file_size(filename_);
//          mfile_ptr_.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, filename_.c_str(), file_size_));
//          cont_ = mfile_ptr_->find<cont_type>(mapname_.c_str()).first;

//          // std::cout << "file_size_: " << file_size_ << std::endl;

//          std::cout << "mfile_ptr_->get_free_memory(): " << mfile_ptr_->get_free_memory() << std::endl;
//       }

//       // assert(mfile_ptr_->get_free_memory() >= _25p);

//       return cont_->insert(x);
//    }

// private:
//    std::string filename_;
//    std::string mapname_;
//    std::size_t file_size_;
//    std::unique_ptr<boost::interprocess::managed_mapped_file> mfile_ptr_;
//    cont_type* cont_;
// };

