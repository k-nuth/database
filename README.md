# Bitprim Database <a target="_blank" href="https://gitter.im/bitprim/Lobby">![Gitter Chat][badge.Gitter]</a>

*Bitcoin High Performance Blockchain Database*

| **master(linux/osx)** | **conan-build-win(linux/osx)**   | **master(windows)**   | **conan-build-win(windows)** |
|:------:|:-:|:-:|:-:|
| [![Build Status](https://travis-ci.org/bitprim/bitprim-database.svg)](https://travis-ci.org/bitprim/bitprim-database)       | [![Build StatusB](https://travis-ci.org/bitprim/bitprim-database.svg?branch=conan-build-win)](https://travis-ci.org/bitprim/bitprim-database?branch=conan-build-win)  | [![Appveyor Status](https://ci.appveyor.com/api/projects/status/github/bitprim/bitprim-database?svg=true)](https://ci.appveyor.com/project/bitprim/bitprim-database)  | [![Appveyor StatusB](https://ci.appveyor.com/api/projects/status/github/bitprim/bitprim-database?branch=conan-build-win&svg=true)](https://ci.appveyor.com/project/bitprim/bitprim-database?branch=conan-build-win)  |

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

bitprim-database will now be installed in `/usr/local/`.

**About Bitprim Database**

Bitprim Database is a custom database build directly on the operating system's [memory-mapped file](https://en.wikipedia.org/wiki/Memory-mapped_file) system. All primary tables and indexes are built on in-memory hash tables, resulting in constant-time lookups. The database uses [sequence locking](https://en.wikipedia.org/wiki/Seqlock) to avoid writer starvation while never blocking the reader. This is ideal for a high performance blockchain server as reads are significantly more frequent than writes and yet writes must proceed wtihout delay. The [bitprim-blockchain](https://github.com/bitprim/bitprim-blockchain) library uses the database as its blockchain store.

[badge.Gitter]: https://img.shields.io/badge/gitter-join%20chat-blue.svg
