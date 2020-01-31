# Bitprim Database <a target="_blank" href="https://gitter.im/bitprim/Lobby">![Gitter Chat][badge.Gitter]</a>

*Bitcoin High Performance Blockchain Database*

| **master(linux/osx)** | **dev(linux/osx)**   | **master(windows)**   | **dev(windows)** |
|:------:|:-:|:-:|:-:|
| [![Build Status](https://travis-ci.org/k-nuth/kth-database.svg)](https://travis-ci.org/k-nuth/kth-database)       | [![Build StatusB](https://travis-ci.org/k-nuth/kth-database.svg?branch=dev)](https://travis-ci.org/k-nuth/kth-database?branch=dev)  | [![Appveyor Status](https://ci.appveyor.com/api/projects/status/github/k-nuth/kth-database?svg=true)](https://ci.appveyor.com/project/k-nuth/kth-database)  | [![Appveyor StatusB](https://ci.appveyor.com/api/projects/status/github/k-nuth/kth-database?branch=dev&svg=true)](https://ci.appveyor.com/project/k-nuth/kth-database?branch=dev)  |

Make sure you have installed [bitprim-core](https://github.com/k-nuth/kth-core) beforehand according to its build instructions.

```
$ git clone https://github.com/k-nuth/kth-database.git
$ cd bitprim-database
$ mkdir build
$ cd build
$ cmake .. -DWITH_TESTS=OFF -DWITH_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-std=c++11"
$ make -j2
$ sudo make install
$ sudo ldconfig
```

bitprim-database will now be installed in `/usr/local/`.

**About Bitprim Database**

Bitprim Database is a custom database build directly on the operating system's [memory-mapped file](https://en.wikipedia.org/wiki/Memory-mapped_file) system. All primary tables and indexes are built on in-memory hash tables, resulting in constant-time lookups. The database uses [sequence locking](https://en.wikipedia.org/wiki/Seqlock) to avoid writer starvation while never blocking the reader. This is ideal for a high performance blockchain server as reads are significantly more frequent than writes and yet writes must proceed wtihout delay. The [bitprim-blockchain](https://github.com/k-nuth/kth-blockchain) library uses the database as its blockchain store.

[badge.Gitter]: https://img.shields.io/badge/gitter-join%20chat-blue.svg
