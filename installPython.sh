mkdir build && cd build

cmake -Dbinding_python=ON ..

make install

cd .. && rm -rf build