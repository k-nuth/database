#
# Copyright (c) 2017 Bitprim developers (see AUTHORS)
#
# This file is part of Bitprim.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import os
from conans import ConanFile, CMake

def option_on_off(option):
    return "ON" if option else "OFF"

class BitprimDatabaseConan(ConanFile):
    name = "bitprim-database"
    version = "0.7"
    license = "http://www.boost.org/users/license.html"
    url = "https://github.com/bitprim/bitprim-database/tree/conan-build/conanfile.py"
    description = "Bitcoin High Performance Blockchain Database"
    settings = "os", "compiler", "build_type", "arch"

    # options = {"shared": [True, False]}
    # default_options = "shared=False"

    options = {"shared": [True, False],
               "fPIC": [True, False],
               "with_tests": [True, False],
               "with_tools": [True, False],
    }
    # "not_use_cpp11_abi": [True, False]

    default_options = "shared=False", \
        "fPIC=True", \
        "with_tests=False", \
        "with_tools=False"

    # "not_use_cpp11_abi=False"

    # with_tests = False
    # with_tools = False

    generators = "cmake"
    exports_sources = "src/*", "CMakeLists.txt", "cmake/*", "bitprim-databaseConfig.cmake.in", "include/*", "test/*", "tools/*"
    package_files = "build/lbitprim-database.a"
    build_policy = "missing"

    requires = (("bitprim-conan-boost/1.66.0@bitprim/stable"),
                ("bitprim-core/0.7@bitprim/testing"))

    def package_id(self):
        self.info.options.with_tests = "ANY"
        self.info.options.with_tools = "ANY"

    def build(self):
        cmake = CMake(self)
        
        cmake.definitions["USE_CONAN"] = option_on_off(True)
        cmake.definitions["NO_CONAN_AT_ALL"] = option_on_off(False)
        cmake.definitions["CMAKE_VERBOSE_MAKEFILE"] = option_on_off(False)
        cmake.definitions["ENABLE_SHARED"] = option_on_off(self.options.shared)
        cmake.definitions["ENABLE_POSITION_INDEPENDENT_CODE"] = option_on_off(self.options.fPIC)

        # cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(self.options.not_use_cpp11_abi)
        cmake.definitions["WITH_TESTS"] = option_on_off(self.options.with_tests)
        cmake.definitions["WITH_TOOLS"] = option_on_off(self.options.with_tools)

        # cmake.definitions["WITH_TESTS"] = option_on_off(self.with_tests)
        # cmake.definitions["WITH_TOOLS"] = option_on_off(self.with_tools)

        if self.settings.compiler == "gcc":
            if float(str(self.settings.compiler.version)) >= 5:
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(False)
            else:
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(True)


        cmake.definitions["BITPRIM_BUILD_NUMBER"] = os.getenv('BITPRIM_BUILD_NUMBER', '-')
        cmake.configure(source_dir=self.source_folder)
        cmake.build()

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
        self.cpp_info.libs = ["bitprim-database"]
