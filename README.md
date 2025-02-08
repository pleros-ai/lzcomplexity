<div align="center" style="width: 100%;">
  <img src="./Pleros_AI.webp" alt="drawing" align="center" height="200"/>

  <h2 align="center">
    lzcomplexity: the LempelZiv analysis library
  </h2>

[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE) 
[![python](https://img.shields.io/badge/Python-3.9-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org) 
![C++](https://img.shields.io/badge/C++-17-00599C.svg?style=flat&logo=c%2B%2B&logoColor=white)
</div>

# lzcomplexity

lzcomplexity is a C++ library that provides a suite of entropy measures for time series data, based on the Lempel-Ziv 76 (LZ76) factorization algorithm.

## Prerequisites

- CMake version >= 3.5
- C++20 compatible compiler:
  - apple-clang >= 14
  - clang >= 17
  - GNU >= 9.4

## Prepare the Local Workspace

The project uses the submodules oneTBB for parallel works and pybind11 for building the Python binding of the library.

1. Initialize the submodules:

```bash
git submodule init
```
2. Clone the submodules into your local directory:

```bash
git submodule update
```

3. Apply the oneTBB patch:

```bash
patch external/tbb/CMakeLists.txt patches/tbb.patch 
```

## Build and install locally
---

1. Create a build directory and navigate to it:

```bash
mkdir build && cd build
```

2. Configure the build process using CMake:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

3. Build and install the library:

```bash
make install
lzcomplexity
```

## CMake Options
---

- `BUILTIN_TBB` (**OFF** by default): use local oneTBB project instead of system one.
- `LZ_SHARE` (**ON** by default): build shared library.
- `LZ_ONLY_LIBS` (**OFF** by default): build only the libraries (LZCore and LZApp).
- `LZ_ONLY_CORE` (**OFF** by default): build only the core library.
- `LZ_APP` (**ON** by default): build the lzcomplexity standalone application.
- `LZ_DISTANCE` (**ON** by default): build the lzdistance standalone application.
- `BUILD_PYTHON` (**OFF** by default): enable python binding.
- `ASAN` (**OFF** by default): configure the build with sanitizer for debug the application (only for clang).
- `ENABLE_ADDRESS_SANITIZER` (**OFF** by default): activate the sanitizer address option for detect memory error.
- `ENABLE_MEMORY_SANITIZER` (**OFF** by default): activate the sanitizer memory option for detect uninitialized memory reads.
- `ENABLE_UNDEFINED_SANITIZER` (**OFF** by default): activate the sanitizer undefined option for detect undefined behavior.

## Example Usage

### C++ Example

```cpp
#include <lz/lempelziv.h>

int main() {
    lz::sequence seq = "some data sequence";
    lz::internal::LZ_Result result = lz::internal::LempelZiv76().Factorize(seq);
    std::cout << "LZ76 Complexity: " << result.factorization << std::endl;
    return 0;
}
```

### Python Example

```python
import lzcomplexity as lz

seq = "some data sequence"
result = lz.lz76(seq)
print("LZ76 Complexity:", result.complexity)
```

# License
---

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.