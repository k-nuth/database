mkdir build
cd build
# rm -rf *
conan install .. -o db=new_full_async -o use_domain=False -s build_type=Debug -o with_tests=True
conan build ..