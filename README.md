<div align="center" style="width: 100%;">
  <img src="./Pleros_AI.webp" alt="drawing" align="center" height="200"/>

  <h2 align="center">
    lzcomplexity: the LempelZiv analysis library
  </h2>

[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE) 
[![python](https://img.shields.io/badge/Python-3.9-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org) 
![C++](https://img.shields.io/badge/C++-20-A0599C.svg?style=flat&logo=c%2B%2B&logoColor=white)
</div>

# lzcomplexity

**lzcomplexity** is a high-performance C++ library for computing entropy and complexity measures of symbolic sequences, based on the Lempel-Ziv 76 (LZ76) factorization algorithm [1]. The library provides efficient implementations for analyzing time series data, with applications in information theory, signal processing, neuroscience, and complex systems analysis.

## Overview

The Lempel-Ziv complexity is a non-parametric measure of algorithmic complexity that quantifies the rate at which new patterns appear in a sequence. Unlike Shannon entropy, which assumes statistical stationarity and independence, LZ complexity captures the sequential structure and can estimate the entropy rate of ergodic sources asymptotically [2].

### Key Features

- **LZ76 Factorization**: Efficient computation of Lempel-Ziv complexity using the CaPS (Cache-friendly Parallel Suffix array) algorithm [3]
- **Entropy Density**: Normalized entropy rate estimation based on LZ factorization
- **Effective Complexity**: Excess entropy measures quantifying statistical dependencies between sequence halves
- **Shuffle-based Entropy**: Monte Carlo estimation of entropy excess through random permutations
- **Information Distance**: Normalized compression-based distance metrics for sequence comparison
- **Parallel Processing**: Multi-threaded computation using Intel OneTBB for large-scale analysis

### Theoretical Background

The LZ76 complexity *c(S)* of a sequence *S* of length *n* is defined as the minimum number of factors in a factorization where each factor is either:
1. A symbol not previously seen, or
2. The longest substring that has appeared earlier in the sequence

For a random sequence over an alphabet of size *k*, the expected complexity grows as *n / log_k(n)*. The normalized entropy density *h* is estimated as:

```
h ≈ c(S) · log_k(n) / n
```

which converges to the true entropy rate for ergodic sources as *n → ∞* [2].

## Prerequisites

- CMake version >= 3.5
- C++20 compatible compiler:
  - apple-clang >= 14
  - clang >= 17
  - GNU >= 9.4

## Prepare the Local Workspace

The project uses external submodules:
- **[oneTBB](https://github.com/oneapi-src/oneTBB)**: Intel's Threading Building Blocks for parallel computation
- **[nanobind](https://github.com/wjakob/nanobind)**: Small binding library for C++/Python interoperability for the Python bindings

1. Initialize the submodules:

```bash
git submodule init
```
2. Clone the submodules into your local directory:

```bash
git submodule update --recursive
```

3. Apply the oneTBB patch:

```bash
patch external/tbb/CMakeLists.txt patches/tbb.patch 
```

## Build and Install

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
```

## Documentation

After installation, access the command-line documentation via man pages:

```bash
man lzcomplexity
man lzdistance
```

To disable man page installation, use `-DLZ_INSTALL_MAN=OFF` during CMake configuration.

## CMake Options

### Build Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `BUILTIN_TBB` | OFF | Use bundled oneTBB instead of system installation |
| `LZ_SHARE` | ON | Build as shared library (`.so`/`.dylib`) |
| `LZ_ONLY_LIBS` | OFF | Build only libraries (lzcore, lzapp, lzdist) without executables |
| `LZ_ONLY_CORE` | OFF | Build only the core library |
| `LZ_APP` | ON | Build the `lzcomplexity` standalone application |
| `LZ_DISTANCE` | ON | Build the `lzdistance` standalone application |
| `LZ_INSTALL_MAN` | ON | Install man pages for command-line tools |
| `BUILD_PYTHON` | OFF | Enable Python bindings via nanobind |

### Debug and Sanitizer Options

| Option | Default | Description |
|--------|---------|-------------|
| `ASAN` | OFF | Enable sanitizers for debugging (Clang only) |
| `ENABLE_ADDRESS_SANITIZER` | OFF | Detect memory errors (buffer overflows, use-after-free) |
| `ENABLE_MEMORY_SANITIZER` | OFF | Detect uninitialized memory reads |
| `ENABLE_UNDEFINED_SANITIZER` | OFF | Detect undefined behavior |

## Example Usage

### C++ Example

```cpp
#include <lz/lempelziv.h>
#include <iostream>

int main() {
    // Create a symbolic sequence
    lz::sequence seq = "ABRACADABRA";
    
    // Compute LZ76 complexity (number of factors)
    auto complexity = lz::lz76Factorization(seq);
    
    // Compute normalized entropy density
    auto entropy = lz::lz76EntropyDensity(seq);
    
    // Compute effective complexity (excess entropy)
    auto effective = lz::lz76EffectiveComplexity(seq);
    
    std::cout << "LZ76 Complexity: " << complexity << std::endl;
    std::cout << "Entropy density: " << entropy << std::endl;
    std::cout << "Effective complexity: " << effective << std::endl;
    
    return 0;
}
```

### Python Example

```python
import lzcomplexity as lz

# Analyze a symbolic sequence
seq = "ABRACADABRA"
result = lz.lz76(seq)

print(f"LZ76 Complexity: {result.complexity}")
print(f"Entropy density: {result.entropy}")
print(f"Effective complexity: {result.effective_complexity}")
```

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences. *IEEE Transactions on Information Theory*, 22(1), 75-81.

2. Kontoyiannis, I., Algoet, P. H., Suhov, Y. M., & Wyner, A. J. (1998). Nonparametric entropy estimation for stationary processes and random fields, with applications to English text. *IEEE Transactions on Information Theory*, 44(3), 1319-1327.

3. Khan, J., Rubel, T., Dhulipala, L., Molloy, E., & Patro, R. (2023). Fast, Parallel, and Cache-Friendly Suffix Array Construction. *arXiv preprint arXiv:2305.07024*.

## Citation

If you use this library in your research, please cite:

```bibtex
@article{Aragon-Perez_Estévez-Rams_2025, 
  title={Lzcomplexity: Entropy Measurement Library}, 
  url={https://www.revistacubanadefisica.org/index.php/rcf/article/view/31}, 
  volume={42}, 
  number={2}, 
  journal={Revista Cubana de Física}, 
  year={2025}, 
  month={Dec.}, 
  pages={80–86} 
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.