/**
 * @file main.cpp
 * @brief Python bindings entry point for the lzcomplexity library.
 *
 * This file defines the main Python module and registers all submodules
 * for Lempel-Ziv complexity analysis.
 */

#include <nanobind/nanobind.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)
#define VERSION_INFO 0.10.0

namespace py = nanobind;

// Forward declarations for submodule registration functions
void PyCore(py::module_&);
void PyMetrics(py::module_&);
void PySpectral(py::module_&);

NB_MODULE(lzcomplexity, m) {
  m.doc() = R"pbdoc(
lzcomplexity - Lempel-Ziv Complexity Analysis Library
======================================================

A high-performance library for computing entropy and complexity measures
of symbolic sequences based on the Lempel-Ziv 76 (LZ76) factorization algorithm.

Main Features
-------------
- **LZ76 Factorization**: Compute the Lempel-Ziv 76 complexity of sequences
- **Entropy Density**: Normalized entropy rate estimation
- **Shuffle Complexity**: Effective complexity via random/paired shuffling
- **Information Distance**: Compression-based distance between sequences
- **Spectral Analysis**: Power spectral density and spectral entropy

Quick Start
-----------
>>> import lzcomplexity as lz
>>> seq = "ABRACADABRA"
>>> result = lz.lz76(seq)
>>> print(f"Complexity: {result.complexity}")
>>> print(f"Entropy: {result.entropy}")

Functions
---------
- lz76(): Complete LZ76 analysis
- factorization(): Compute LZ76 complexity (number of factors)
- factors(): Get factorization boundary positions
- entropy_density(): Compute normalized entropy density
- emc(): Effective measure complexity via random shuffling
- lz76PairedShuffleComplexity(): Effective measure complexity via paired shuffling
- metrics.nid(): Compression-based distance between sequences
- metrics.rid(): Shuffle-based information distance
- spectral.entropy(): Spectral entropy of time-domain signals
- spectral.psd(): Power spectral density via FFT
- spectral.semc(): Non-random spectral structure measure
- CaPS: Cache-friendly Parallel Suffix array constructor

Version: 0.10.0
Authors: Efren Aragon Perez, Ernesto Estevez Rams
Contact: efrenaragon96@gmail.com, estevez@fisica.uh.cu
License: MIT
)pbdoc";

  py::module_ spectral = m.def_submodule("spectral", "Spectral analysis functions");
  py::module_ metric = m.def_submodule("metrics", "Metric functions");

  // Core functions
  PyCore(m);
  // Metrics module
  PyMetrics(metric);
  // Spectral module
  PySpectral(spectral);

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}