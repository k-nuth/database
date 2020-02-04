# Knuth Database <a target="_blank" href="https://gitter.im/k-nuth/Lobby">![Gitter Chat][badge.Gitter]</a>

*High Performance Blockchain Database*

| **master(linux/osx)** | **dev(linux/osx)**   | **master(windows)**   | **dev(windows)** |
|:------:|:-:|:-:|:-:|
| [![Build Status](https://travis-ci.org/k-nuth/database.svg)](https://travis-ci.org/k-nuth/database)       | [![Build StatusB](https://travis-ci.org/k-nuth/database.svg?branch=dev)](https://travis-ci.org/k-nuth/database?branch=dev)  | [![Appveyor Status](https://ci.appveyor.com/api/projects/status/github/k-nuth/database?svg=true)](https://ci.appveyor.com/project/k-nuth/database)  | [![Appveyor StatusB](https://ci.appveyor.com/api/projects/status/github/k-nuth/database?branch=dev&svg=true)](https://ci.appveyor.com/project/k-nuth/database?branch=dev)  |

Make sure you have installed [domain](https://github.com/k-nuth/domain) beforehand according to its build instructions.

```
$ git clone https://github.com/k-nuth/database.git
$ cd database
$ mkdir build
$ cd build
$ cmake .. -DWITH_TESTS=OFF -DWITH_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-std=c++11"
$ make -j2
$ sudo make install
$ sudo ldconfig
```

kth-database will now be installed in `/usr/local/`.

**About Knuth Database**

Knuth Database is a custom database build directly on the operating system's [memory-mapped file](https://en.wikipedia.org/wiki/Memory-mapped_file) system. All primary tables and indexes are built on in-memory hash tables, resulting in constant-time lookups. The database uses [sequence locking](https://en.wikipedia.org/wiki/Seqlock) to avoid writer starvation while never blocking the reader. This is ideal for a high performance blockchain server as reads are significantly more frequent than writes and yet writes must proceed wtihout delay. The [blockchain](https://github.com/k-nuth/blockchain) library uses the database as its blockchain store.

[badge.Gitter]: https://img.shields.io/badge/gitter-join%20chat-blue.svg
