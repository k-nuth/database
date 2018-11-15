mkdir build
cd build
# rm -rf *
conan install .. -o db=new -o use_domain=False -s build_type=Debug -o with_tests=True
conan build ..