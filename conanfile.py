# Copyright (c) 2016-2022 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
from conans import CMake
from kthbuild import option_on_off, march_conan_manip, pass_march_to_compiler
from kthbuild import KnuthConanFile

class KnuthDatabaseConan(KnuthConanFile):
    def recipe_dir(self):
        return os.path.dirname(os.path.abspath(__file__))

    name = "database"
    # version = get_version()
    license = "http://www.boost.org/users/license.html"
    url = "https://github.com/k-nuth/database/tree/conan-build/conanfile.py"
    description = "High Performance Blockchain Database"
    settings = "os", "compiler", "build_type", "arch"

    # if Version(conan_version) < Version(get_conan_req_version()):
    #     raise Exception ("Conan version should be greater or equal than %s. Detected: %s." % (get_conan_req_version(), conan_version))

    options = {"shared": [True, False],
               "fPIC": [True, False],
               "tests": [True, False],
               "tools": [True, False],
               "currency": ['BCH', 'BTC', 'LTC'],
               "microarchitecture": "ANY",
               "fix_march": [True, False],
               "march_id": "ANY",
               "verbose": [True, False],
               "measurements": [True, False],
               "db": ['legacy', 'legacy_full', 'pruned', 'default', 'full'],
               "db_readonly": [True, False],
               "cached_rpc_data": [True, False],
               "cxxflags": "ANY",
               "cflags": "ANY",
               "glibcxx_supports_cxx11_abi": "ANY",
               "cmake_export_compile_commands": [True, False],
               "log": ["boost", "spdlog", "binlog"],
               "use_libmdbx": [True, False],
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "tests": False,
        "tools": False,
        "currency": "BCH",
        "microarchitecture": "_DUMMY_",
        "fix_march": False,
        "march_id": "_DUMMY_",
        "verbose": False,
        "measurements": False,
        "db": "default",
        "db_readonly": False,
        "cached_rpc_data": False,
        "cxxflags": "_DUMMY_",
        "cflags": "_DUMMY_",
        "glibcxx_supports_cxx11_abi": "_DUMMY_",
        "cmake_export_compile_commands": False,
        "log": "spdlog",
        "use_libmdbx": False,
    }

    generators = "cmake"
    exports = "conan_*", "ci_utils/*"
    exports_sources = "src/*", "CMakeLists.txt", "cmake/*", "kth-databaseConfig.cmake.in", "knuthbuildinfo.cmake", "include/*", "test/*", "tools/*"

    package_files = "build/lkth-database.a"
    build_policy = "missing"

    def _is_legacy_db(self):
        return self.options.db == "legacy" or self.options.db == "legacy_full"

    def requirements(self):
        if not self._is_legacy_db():
            if self.options.use_libmdbx:
                self.requires("libmdbx/0.7.0@kth/stable")
                self.output.info("Using libmdbx for DB management")
            else:
                self.requires("lmdb/0.9.24@kth/stable")
                self.output.info("Using lmdb for DB management")
        else:
            self.output.info("Using legacy DB")

        if self.options.tests:
            self.requires("catch2/2.13.7")

        self.requires("domain/0.X@%s/%s" % (self.user, self.channel))

    def config_options(self):
        KnuthConanFile.config_options(self)

    def configure(self):
        KnuthConanFile.configure(self)

        self.options["*"].cached_rpc_data = self.options.cached_rpc_data
        self.options["*"].measurements = self.options.measurements

        self.options["*"].db_readonly = self.options.db_readonly
        self.output.info("Compiling with read-only DB: %s" % (self.options.db_readonly,))

        # self.options["*"].currency = self.options.currency
        # self.output.info("Compiling for currency: %s" % (self.options.currency,))
        self.output.info("Compiling with measurements: %s" % (self.options.measurements,))
        self.output.info("Compiling for DB: %s" % (self.options.db,))

        self.options["*"].log = self.options.log
        self.output.info("Compiling with log: %s" % (self.options.log,))

    def package_id(self):
        KnuthConanFile.package_id(self)

    def build(self):
        cmake = self.cmake_basis()
        cmake.definitions["WITH_MEASUREMENTS"] = option_on_off(self.options.measurements)
        cmake.definitions["DB_READONLY_MODE"] = option_on_off(self.options.db_readonly)
        cmake.definitions["WITH_CACHED_RPC_DATA"] = option_on_off(self.options.cached_rpc_data)
        cmake.definitions["LOG_LIBRARY"] = self.options.log
        cmake.definitions["USE_LIBMDBX"] = option_on_off(self.options.use_libmdbx)

        if self.options.cmake_export_compile_commands:
            cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = option_on_off(self.options.cmake_export_compile_commands)

        cmake.configure(source_dir=self.source_folder)

        if not self.options.cmake_export_compile_commands:
            cmake.build()
            #Note: Cmake Tests and Visual Studio doesn't work
            if self.options.tests:
                cmake.test()
                # cmake.test(target="tests")

    def imports(self):
        self.copy("*.h", "", "include")

    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.ipp", dst="include", src="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libs = ["kth-database"]

