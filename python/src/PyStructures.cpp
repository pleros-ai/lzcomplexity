/**
 * @file PyStructures.cpp
 * @brief Python bindings for LZ76 data structures (LZ_Shuffle, LZ_Args).
 */

#include <lz/structures.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace py = nanobind;
namespace utl = lz::utils;

void PyStructures(py::module_& m) {
  using namespace py::literals;

  // =========================================================================
  // LZ_Shuffle class - Results from shuffle-based complexity analysis
  // =========================================================================
  py::class_<utl::LZ_Shuffle> LZ_Shuffle(m, "LZ_Shuffle", R"pbdoc(
Results container for shuffle-based complexity analysis.

This class stores the results from random shuffle or paired shuffle complexity
calculations. The shuffle complexity measures the excess entropy by comparing
the original sequence's complexity with randomly shuffled versions.

Attributes
----------
max_block_size : int
    Maximum block size used during the shuffling process. Larger blocks
    preserve more local structure during shuffling.
emc_value : float
    The shuffle entropy deficit (effective complexity). This is the difference
    between the expected complexity of a random sequence and the actual
    complexity, representing the non-random information content.
multi_information : float
    Multi-information value computed with block_size=1. This measures the
    total statistical dependence among all symbols in the sequence.
summands : List[float]
    Individual complexity contributions at each block size level. These
    values sum to give the total emc_value.

Examples
--------
>>> import lzcomplexity as lz
>>> result = lz.lz76RandomShuffleComplexity("ABRACADABRA")
>>> print(f"Effective complexity: {result.emc_value}")
>>> print(f"Multi-information: {result.multi_information}")
)pbdoc");

  LZ_Shuffle.def(py::init(), "Create an empty LZ_Shuffle object.")
    .def(py::init<lz::lz_int, lz::lz_double, lz::lz_double, std::vector<lz::lz_double>>(),
         "max_block_size_"_a,
         "emc_value_"_a,
         "multi_information_"_a,
         "summands_"_a = std::vector<lz::lz_double>(),
         R"pbdoc(
Create an LZ_Shuffle object with specified values.

Parameters
----------
max_block_size_ : int
    Maximum block size used for shuffling.
emc_value_ : float
    The computed shuffle entropy deficit.
multi_information_ : float
    Multi-information value (block_size=1).
summands_ : List[float], optional
    Individual complexity contributions at each level.
)pbdoc")
    .def("__str__",
         [](const utl::LZ_Shuffle& self) {
           return "LZ_Shuffle(emc_value=" + std::to_string(self.emc_value)
             + ", max_block_size=" + std::to_string(self.max_block_size)
             + ", multi_information=" + std::to_string(self.multi_information) + ")";
         })
    .def("__repr__",
         [](const utl::LZ_Shuffle& self) {
           return "LZ_Shuffle(emc_value=" + std::to_string(self.emc_value)
             + ", max_block_size=" + std::to_string(self.max_block_size) + ")";
         })
    .def("__copy__", [](const utl::LZ_Shuffle& self) { return utl::LZ_Shuffle(self); })
    .def(
      "__deepcopy__", [](const utl::LZ_Shuffle& self, py::dict) { return utl::LZ_Shuffle(self); }, "memo"_a);

  // Define attributes with detailed docstrings
  LZ_Shuffle
    .def_rw("max_block_size",
            &utl::LZ_Shuffle::max_block_size,
            "int: Maximum block size used for shuffling. Larger values preserve more local structure.")
    .def_rw("emc_value",
            &utl::LZ_Shuffle::emc_value,
            "float: Shuffle entropy deficit (effective complexity). Measures non-random information content.")
    .def_rw(
      "multi_information",
      &utl::LZ_Shuffle::multi_information,
      "float: Multi-information value computed with block_size=1. Measures total statistical dependence.")
    .def_rw("summands",
            &utl::LZ_Shuffle::summands,
            "List[float]: Individual complexity contributions at each block size level.");

  // =========================================================================
  // LZ_Args class - Configuration parameters for LZ76 algorithms
  // =========================================================================
  py::class_<utl::LZ_Args> LZ_Args(m, "LZ_Args", R"pbdoc(
Configuration parameters for LZ76 complexity algorithms.

This class encapsulates all configuration options for controlling the behavior
of LZ76 factorization and related complexity measures. It allows fine-tuning
of parallel processing, alphabet handling, and shuffle complexity parameters.

Attributes
----------
chunks : int
    Number of partitions for the parallel suffix array algorithm (CaPS).
    Higher values may improve performance on multi-core systems for long sequences.
    Default is 0 (automatic selection based on sequence length).
alphabet : int
    Size of the symbol alphabet. For binary sequences use 2, for DNA use 4,
    for ASCII text use 256. Default is 2.
log_base : int
    Base for logarithm calculations in entropy measures. Use 2 for bits,
    e (≈2.718) for nats, 10 for decimal. Default equals alphabet size.
block_size : int
    Maximum block size for shuffle entropy deficit calculation. Use -1 for
    automatic size determination based on sequence length. Default is -1.
get_shuffle_terms : bool
    If True, compute and store individual summand terms during shuffle
    complexity calculation. Default is False.

Examples
--------
>>> import lzcomplexity as lz
>>> # Create args for DNA sequence analysis
>>> args = lz.LZ_Args()
>>> args.alphabet = 4      # DNA has 4 bases
>>> args.log_base = 4      # Use base-4 logarithm
>>> args.chunks = 8        # Use 8 parallel partitions
>>> result = lz.lz76("ACGTACGTACGT", args)
)pbdoc");

  LZ_Args.def(py::init(), "Create LZ_Args with default values.")
    .def(py::init<lz::lz_int>(), "_chunks"_a, "Create LZ_Args with specified number of parallel partitions.")
    .def(py::init<lz::lz_int, lz::lz_int>(),
         "_chunks"_a,
         "_block_size"_a,
         "Create LZ_Args with partitions and block size.")
    .def(py::init<lz::lz_int, lz::lz_int, lz::lz_uint>(),
         "_chunks"_a,
         "_block_size"_a,
         "alphabet_"_a,
         "Create LZ_Args with partitions, block size, and alphabet.")
    .def(py::init<lz::lz_int, lz::lz_int, lz::lz_uint, lz::lz_uint>(),
         "_chunks"_a,
         "_block_size"_a,
         "alphabet_"_a,
         "log_base"_a,
         "Create LZ_Args with all parameters specified.")
    .def(py::self == py::self, "Check equality between two LZ_Args objects.")
    .def(py::self != py::self, "Check inequality between two LZ_Args objects.")
    .def("__copy__", [](const utl::LZ_Args& self) { return utl::LZ_Args(self); })
    .def("__repr__",
         [](const utl::LZ_Args& self) {
           return "LZ_Args(chunks=" + std::to_string(self.chunks)
             + ", alphabet=" + std::to_string(self.alphabet) + ", log_base=" + std::to_string(self.log_base)
             + ", block_size=" + std::to_string(self.block_size) + ")";
         })
    .def("__deepcopy__", [](const utl::LZ_Args& self, py::dict) { return utl::LZ_Args(self); }, "memo"_a);

  // Define attributes with detailed docstrings
  LZ_Args
    .def_rw("chunks",
            &utl::LZ_Args::chunks,
            "int: Number of partitions for parallel suffix array construction. 0 = automatic.")
    .def_rw("max_context",
            &utl::LZ_Args::max_context,
            "int: Maximum prefix length for suffix comparison. 0 = unlimited")
    .def_rw("get_shuffle_terms",
            &utl::LZ_Args::get_shuffle_terms,
            "bool: If True, compute individual summand terms for shuffle complexity.")
    .def_rw("alphabet",
            &utl::LZ_Args::alphabet,
            "int: Size of the symbol alphabet (e.g., 2 for binary, 4 for DNA, 256 for ASCII).")
    .def_rw("log_base",
            &utl::LZ_Args::log_base,
            "int: Base for logarithm in entropy calculations. Usually equals alphabet size.")
    .def_rw("block_size",
            &utl::LZ_Args::block_size,
            "int: Maximum block size for shuffle complexity. -1 = automatic selection.");
}