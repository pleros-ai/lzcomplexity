mkdir build && cd build

cmake -DLZ_DISTANCE=ON ..

make install

cd .. && rm -rf build