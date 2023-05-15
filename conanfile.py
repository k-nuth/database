# Copyright (c) 2016-2023 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
from conan import ConanFile
from conan.tools.build.cppstd import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy #, apply_conandata_patches, export_conandata_patches, get, rm, rmdir
from kthbuild import KnuthConanFileV2, option_on_off

required_conan_version = ">=2.0"

class KnuthDatabaseConan(KnuthConanFileV2):
    name = "database"
    license = "http://www.boost.org/users/license.html"
    url = "https://github.com/k-nuth/database/tree/conan-build/conanfile.py"
    description = "High Performance Blockchain Database"
    settings = "os", "compiler", "build_type", "arch"

    options = {"shared": [True, False],
               "fPIC": [True, False],
               "tests": [True, False],
               "tools": [True, False],
               "currency": ['BCH', 'BTC', 'LTC'],
               "march_id": ["ANY"],
               "march_strategy": ["download_if_possible", "optimized", "download_or_fail"],
               "verbose": [True, False],
               "measurements": [True, False],
               "db": ['legacy', 'legacy_full', 'pruned', 'default', 'full'],
               "db_readonly": [True, False],
               "cached_rpc_data": [True, False],
               "cxxflags": ["ANY"],
               "cflags": ["ANY"],
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
        "march_strategy": "download_if_possible",
        "verbose": False,
        "measurements": False,
        "db": "default",
        "db_readonly": False,
        "cached_rpc_data": False,
        "cmake_export_compile_commands": False,
        "log": "spdlog",
        "use_libmdbx": False,
    }

    exports_sources = "src/*", "CMakeLists.txt", "ci_utils/cmake/*", "cmake/*", "knuthbuildinfo.cmake", "include/*", "test/*", "tools/*"

    def _is_legacy_db(self):
        return self.options.db == "legacy" or self.options.db == "legacy_full"

    def build_requirements(self):
        if self.options.tests:
            self.test_requires("catch2/3.3.1")

    def requirements(self):
        self.requires("domain/0.27.0", transitive_headers=True, transitive_libs=True)

        if not self._is_legacy_db():
            if self.options.use_libmdbx:
                self.requires("libmdbx/0.7.0@kth/stable", transitive_headers=True, transitive_libs=True)
                self.output.info("Using libmdbx for DB management")
            else:
                self.requires("lmdb/0.9.29", transitive_headers=True, transitive_libs=True)
                self.output.info("Using lmdb for DB management")
        else:
            self.output.info("Using legacy DB")

    def validate(self):
        KnuthConanFileV2.validate(self)
        if self.info.settings.compiler.cppstd:
            check_min_cppstd(self, "20")

    def config_options(self):
        KnuthConanFileV2.config_options(self)

    def configure(self):
        KnuthConanFileV2.configure(self)

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
        KnuthConanFileV2.package_id(self)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = self.cmake_toolchain_basis()
        # tc.variables["CMAKE_VERBOSE_MAKEFILE"] = True
        tc.variables["WITH_MEASUREMENTS"] = option_on_off(self.options.measurements)
        tc.variables["DB_READONLY_MODE"] = option_on_off(self.options.db_readonly)
        tc.variables["WITH_CACHED_RPC_DATA"] = option_on_off(self.options.cached_rpc_data)
        tc.variables["LOG_LIBRARY"] = self.options.log
        tc.variables["USE_LIBMDBX"] = option_on_off(self.options.use_libmdbx)
        tc.variables["CONAN_DISABLE_CHECK_COMPILER"] = option_on_off(True)

        if self.options.cmake_export_compile_commands:
            tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = option_on_off(self.options.cmake_export_compile_commands)

        tc.generate()
        tc = CMakeDeps(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()

        if not self.options.cmake_export_compile_commands:
            cmake.build()
            #Note: Cmake Tests and Visual Studio doesn't work
            if self.options.tests:
                cmake.test()
                # cmake.test(target="tests")

    # def imports(self):
    #     self.copy("*.h", "", "include")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        # rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        # rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        # rmdir(self, os.path.join(self.package_folder, "res"))
        # rmdir(self, os.path.join(self.package_folder, "share"))

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libs = ["database"]

