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
#define VERSION_INFO 0.9.15

namespace py = nanobind;

// Forward declarations for submodule registration functions
void PyUtils(py::module_&);
void PySequence(py::module_&);
void PySaStructure(py::module_&);
void PyStructures(py::module_&);
void PyCaps(py::module_&);
void PyLempelZiv(py::module_&);
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

Classes
-------
- sequence: Symbolic sequence container with manipulation methods
- LZ_Args: Configuration parameters for LZ76 algorithms
- LZ_Shuffle: Results from shuffle-based complexity analysis
- LempelZiv: Complete LZ76 analysis results
- CaPS: Cache-friendly Parallel Suffix array constructor

Functions
---------
- lz76(): Complete LZ76 analysis
- lz76Factorization(): Compute LZ76 complexity (number of factors)
- lz76Factors(): Get factorization boundary positions
- lz76EntropyDensity(): Compute normalized entropy density
- lz76RandomShuffleComplexity(): Effective measure complexity via random shuffling
- lz76PairedShuffleComplexity(): Effective measure complexity via paired shuffling
- lz76InformationDistance(): Compression-based distance between sequences
- lz76RandomShuffleDistance(): Shuffle-based information distance
- SpectralEntropy(): Spectral entropy of time-domain signals
- PowerSpectralDensity(): Power spectral density via FFT
- EffectiveSpectralComplexity(): Non-random spectral structure measure

Version: 0.9.15
Authors: Efren Aragon Perez, Ernesto Estevez Rams
Contact: efrenaragon96@gmail.com, estevez@fisica.uh.cu
License: MIT
)pbdoc";

  // Utils bindings
  PyUtils(m);
  PySaStructure(m);
  PyStructures(m);
  PySequence(m);
  // Algorithms bindings
  PyCaps(m);
  // PySais(m);
  // Lempel-ziv 76 functions
  PyLempelZiv(m);
  PySpectral(m);

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}