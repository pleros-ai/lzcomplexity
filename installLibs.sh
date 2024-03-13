mkdir build && cd build

cmake -DLZ_ONLY_LIBS=ON ..

make install

cd .. && rm -rf build