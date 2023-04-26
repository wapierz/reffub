cmake  -S./\
 -B./build/debug \
 -D CMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Debug .
cd build/debug/
make
