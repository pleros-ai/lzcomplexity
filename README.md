
<div style="display: flex; justify-content: center; width: 100%;">
  <div>
    <img src="./Pleros_AI.webp" alt="drawing" style="width:200px;"/>
  </div>

  <pre style="height: 200px; display: flex; justify-content: flex-start; align-items: center; width: 500px;">
    <code>
            ---------- Presents ------------
      lzcomplexity: a LempelZiv analysis library
            ------------------------------
                         2024
    </code>
  </pre>
</div>

[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE) [![python](https://img.shields.io/badge/Python-3.9-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org) ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=flat&logo=c%2B%2B&logoColor=white)


# lzcomplexity

lzcomplexity is a C++ library that provides a suite of entropy measures for time series data, based on the Lempel-Ziv 76 (LZ76) factorization algorithm.

### Prerequisites
---

- Make sure you have installed CMake version >=3.5 on your system. LempelZiv uses CMake build configuration.
- The project uses C++20 features so it needs a compiler compatible with it.
  - apple-clang >= 14
  - clang >= 17
  - GNU >= 9.4

### Prepare the local workspace
---

The project uses the submodules oneTBB for parallel works and pybind11 for build the python binding of the library

1. First you need initialize the submodules

```bash
git submodule init
```
2. Then clone it into your local directory 

```bash
git submodule update
```

### Build locally
---

The library use cmake for management the build process.

1. Create a build directory a move into the directory (optional)

```bash
mkdir build && cd build
```

2. Execute cmake for config the build with the build options

```bash
cmake -DCMAKE_INSTALL_PREFIX=[path/to/install] -DCMAKE_BUILD_TYPE=[Debug | Release] [OPTIONS] ..
```

The possible cmake OPTIONS can be found in the `CMakeLists.txt` file:

- `BUILTIN_TBB` (**OFF** by default): use local TBB project instead of system one.
- `LZ_SHARE` (**ON** by default): build shared library.
- `LZ_ONLY_LIBS` (**OFF** by default): build only the libraries (LZCore and LZApp).
- `LZ_ONLY_CORE` (**OFF** by default): build only the core library.
- `BUILD_PYTHON` (**OFF** by default): enable python binding.
- `ASAN` (**OFF** by default): configure the build with sanitizer for debug the application (only for clang).
- `ENABLE_ADDRESS_SANITIZER` (**OFF** by default): activate the sanitizer address option for detect memory error.
- `ENABLE_MEMORY_SANITIZER` (**OFF** by default): activate the sanitizer memory option for detect uninitialized memory reads.
- `ENABLE_UNDEFINED_SANITIZER` (**OFF** by default): activate the sanitizer undefined option for detect undefined behavior.

3. Install

```bash
make install
```
