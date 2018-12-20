mkdir build
cd build
conan install .. -o use_domain=False -o db=new  -o with_tests=True
conan build ..