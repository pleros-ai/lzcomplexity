```
       _    ____  ____ __
      | |  |_  /_|__  / /          -----------------------------
      | |__ / /___|/ / _ \      LempelZiv analysis engine v0.7 2024.
      |____/___|  /_/\___/
```
[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE) [![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit)](https://github.com/pre-commit/pre-commit)

### Prerequisites
---

- Make sure you have installed CMake version 3.5 (or newer) on your system. LempelZiv uses CMake build configuration.
- The project uses C++20 features so it needs a compiler compatible with it.
  - clang >= 17 || apple-clang >= 14
  - GNU >= 9.4

### Prepare the workspace
---

The project uses submodules (tbb and pybind11), so first it needs to initialize the modules:

```bash
$ git submodule init
```
After that clone the submodules:

```bash
$ git submodule update
```

### Build
---

1. Create a build directory and move into it

```bash
mkdir build && cd build
```

2. Config cmake for build

```bash
cmake -DCMAKE_INSTALL_PREFIX=[path/to/install] -DCMAKE_BUILD_TYPE=[Debug | Release] [OPTIONS] ..
```

The possible cmake OPTIONS can be found in the `CMakeLists.txt` file:

- `asan` (**OFF** by default): configure the build with address sanitizer
- `BUILD_PYTHON` (**OFF** by default): enable python binding
- `builtin_tbb` (**OFF** by default): use local TBB project instead of system one
- `LZ_SHARE` (**ON** by default): build shared library
- `LZ_ONLY_LIBS` (**OFF** by default): build only the libraries (LZCore and LZApp)
- `LZ_ONLY_CORE` (**OFF** by default): build only the core library

##### Example
>
> cmake -DCMAKE_INSTALL_PREFIX=../ -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON=ON ..
>


3. Install

```bash
make install
```
