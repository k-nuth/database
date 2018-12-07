mkdir build
cd build
conan install .. -o use_domain=False -o db=new_full_async  -o with_tests=True
conan build ..