[![Build Status](https://travis-ci.org/bitprim/bitprim-database.svg?branch=c-api)](https://travis-ci.org/bitprim/bitprim-database)

# Bitprim Database

*Bitcoin High Performance Blockchain Database*

Make sure you have installed [bitprim-core](https://github.com/bitprim/bitprim-core) beforehand according to its build instructions.

```
$ git clone https://github.com/bitprim/bitprim-database.git
$ cd bitprim-database
$ mkdir build
$ cd build
$ cmake .. -DWITH_TESTS=OFF -DWITH_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-std=c++11"
$ make -j2
$ sudo make install
$ sudo ldconfig
```

bitprim-database is now installed in `/usr/local/`.

**About Bitprim Database**

Bitprim Database is a custom database build directly on the operating system's [memory-mapped file](https://en.wikipedia.org/wiki/Memory-mapped_file) system. All primary tables and indexes are built on in-memory hash tables, resulting in constant-time lookups. The database uses [sequence locking](https://en.wikipedia.org/wiki/Seqlock) to avoid writer starvation while never blocking the reader. This is ideal for a high performance blockchain server as reads are significantly more frequent than writes and yet writes must proceed wtihout delay. The [bitprim-blockchain](https://github.com/bitprim/bitprim-blockchain) library uses the database as its blockchain store.
