```
                               _    ____  ____ __
                              | |  |_  /_|__  / /          -----------------------------
                              | |__ / /___|/ / _ \      LempelZiv analysis engine v0.7 2024.
                              |____/___|  /_/\___/
```
[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

### Prepare the workspace

The project use submodules (tbb and pybind11), so first it is needs initialize the modules:

```bash
$ git submodule init
```
After that clone the submodules:

```bash
$ git submodule update
```

### Build

1. Create build directory and move into it

```bash
mkdir build && cd build
```

2. Config cmake for build

```bash
cmake -DCMAKE_INSTALL_PREFIX=../ -DCMAKE_BUILD_TYPE=[Debug | Release] [OPTIONS] ..
```

The possibles cmake OPTIONS can be found in the `CMakeLists.txt` file:

- `asan` (**OFF** by default): configure the build with address sanitizer
- `binding_python` (**OFF** by default): enable python binding
- `builtin_tbb` (**OFF** by default): use local TBB project instead of system one


1. Install the program

```bash
make install
```