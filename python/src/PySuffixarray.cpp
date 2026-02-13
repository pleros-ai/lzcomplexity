/**
 * @file PySuffixarray.cpp
 * @brief Python bindings for suffix array construction algorithms.
 */

#include <lz/caps.h>
#include <lz/sais_lite.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <tuple>

namespace py = nanobind;
namespace suf = lz::suffixarray;
namespace utl = lz::utils;

std::unique_ptr<suf::CaPS_SA> constructor_str(std::string T,
                                              lz::lz_int  n,
                                              lz::lz_int  subproblem_count,
                                              lz::lz_int  max_context) {
  return std::unique_ptr<suf::CaPS_SA>{
    new suf::CaPS_SA({T.begin(), T.end()}, n, subproblem_count, max_context)};
}

void PyCaps(py::module_& m) {
  using namespace py::literals;

  // =========================================================================
  // CaPS class - Cache-friendly Parallel Suffix array constructor
  // =========================================================================
  py::class_<suf::CaPS_SA> caps(m, "CaPS", R"pbdoc(
Cache-friendly Parallel Suffix array (CaPS) constructor.

This class implements a high-performance suffix array construction algorithm
optimized for modern multi-core CPUs. It uses cache-friendly memory access
patterns and parallel processing to efficiently build suffix arrays and
LCP (Longest Common Prefix) arrays.

The CaPS algorithm divides the construction into subproblems that can be
solved independently in parallel, then merges the results efficiently.

Parameters
----------
T : Union[str, List[char]]
    The input text/sequence for suffix array construction.
n : int
    Length of the input text.
subproblem_count : int, optional
    Number of subproblems for parallel processing. Default is 0 (automatic).
    Higher values may improve performance on multi-core systems.
max_context : int, optional
    Maximum context length for LCP computation. Default is 0 (full context).

Attributes
----------
n_ : int
    Length of the input text.
T_ : List[char]
    The input text as a character list.
SA_ : List[int]
    The computed suffix array.
LCP_ : List[int]
    The computed LCP (Longest Common Prefix) array.

Examples
--------
>>> import lzcomplexity as lz
>>> # Create CaPS object and construct suffix array
>>> caps = lz.CaPS("ABRACADABRA", 11)
>>> sa = caps.construct()
>>> print(f"Suffix array: {caps.SA_}")
>>> print(f"LCP array: {caps.LCP_}")

Notes
-----
- The suffix array SA[i] contains the starting position of the i-th
  lexicographically smallest suffix.
- The LCP array LCP[i] contains the length of the longest common prefix
  between suffixes SA[i-1] and SA[i].
- For optimal performance, use subproblem_count equal to the number of
  CPU cores available.

References
----------
Khan, J., et al. (2023). Fast, Parallel, and Cache-Friendly Suffix Array
Construction. arXiv:2305.07024.
)pbdoc");

  // Constructors
  caps
    .def(py::init<std::vector<lz::lz_char>, lz::lz_int, lz::lz_int, lz::lz_int>(),
         "T"_a,
         "n"_a,
         "subproblem_count"_a = 0,
         "max_context"_a = 0,
         "Create CaPS from character vector with specified parameters.")
    .def(py::new_(&constructor_str),
         "T"_a,
         "n"_a,
         "subproblem_count"_a = 0u,
         "max_context"_a = 0u,
         "Create CaPS from string with specified parameters.")
    .def(py::init<lz::lz_int, lz::lz_int>(),
         "subproblem_count"_a = 0u,
         "max_context"_a = 0u,
         "Create empty CaPS with configuration parameters.")
    .def(py::init<utl::SA_Args>(), "args"_a, "Create CaPS from SA_Args configuration object.")
    .def("__copy__", [](const suf::CaPS_SA& self) { return suf::CaPS_SA(self); })
    .def("__deepcopy__", [](const suf::CaPS_SA& self, py::dict) { return suf::CaPS_SA(self); }, "memo"_a);

  // Methods
  caps
    .def(
      "construct",
      [](suf::CaPS_SA& self) { return self.construct(); },
      R"pbdoc(
Construct the suffix array from the text provided at initialization.

Returns
-------
LZ_SuffixArray
    Object containing the computed suffix array (SA) and LCP array.

Notes
-----
This method uses the text that was provided when the CaPS object was created.
)pbdoc",
      py::rv_policy::copy)
    .def("construct",
         py::overload_cast<std::vector<lz::lz_char>, lz::lz_int>(&suf::CaPS_SA::construct),
         "T"_a,
         "n"_a,
         R"pbdoc(
Construct the suffix array from a character vector.

Parameters
----------
T : List[char]
    The input text as a character vector.
n : int
    Length of the input text.

Returns
-------
LZ_SuffixArray
    Object containing the computed suffix array (SA) and LCP array.
)pbdoc",
         py::rv_policy::copy)
    .def("construct",
         py::overload_cast<const std::string&>(&suf::CaPS_SA::construct),
         "T"_a,
         R"pbdoc(
Construct the suffix array from a string.

Parameters
----------
T : str
    The input text as a string.

Returns
-------
LZ_SuffixArray
    Object containing the computed suffix array (SA) and LCP array.
)pbdoc",
         py::rv_policy::copy);

  // Properties
  caps.def_prop_ro("n_", &suf::CaPS_SA::n, "int: Length of the input text.")
    .def_prop_ro("T_", &suf::CaPS_SA::T, "List[char]: The input text as a character list.")
    .def_prop_ro("LCP_", &suf::CaPS_SA::LCP, "List[int]: The computed LCP (Longest Common Prefix) array.")
    .def_prop_ro("SA_", &suf::CaPS_SA::SA, "List[int]: The computed suffix array.");
}

void PySais(py::module_& m) {
  using namespace py::literals;

  py::class_<suf::SAIS> sais(m, "SAIS");
  // Constructors and methods
  // Check why with subproblem_count = 0 BOOM!!! when should initialize p_ with default_subproblem_count
  sais.def(py::init<const lz::lz_char*, lz::lz_int>(), "T"_a, "n"_a)
    .def(
      "construct",
      [](suf::SAIS& self) { return self.construct(); },
      "Generate the suffix array from the text use for build the class",
      py::rv_policy::copy)
    .def("construct",
         py::overload_cast<const std::string&>(&suf::SAIS::construct),
         "Generate the suffix array from the text",
         py::rv_policy::copy);

  // Attributes
  sais.def_prop_ro("n_", &suf::SAIS::n)
    .def_prop_ro("T_", &suf::SAIS::T)
    .def_prop_ro("LCP_", &suf::SAIS::LCP)
    .def_prop_ro("SA_", &suf::SAIS::SA);
}

void PySaStructure(py::module_& m) {
  using namespace py::literals;

  // =========================================================================
  // LZ_SuffixArray class - Suffix array and LCP array container
  // =========================================================================
  py::class_<utl::LZ_SuffixArray> LZ_SuffixArray(m, "LZ_SuffixArray", R"pbdoc(
Container for suffix array (SA) and longest common prefix (LCP) array.

This class stores the results of suffix array construction, including
both the suffix array and the LCP array. It is returned by the CaPS.construct()
method and can be used for various string analysis tasks.

Attributes
----------
SA : List[int]
    The suffix array. SA[i] contains the starting position of the i-th
    lexicographically smallest suffix of the input text.
LCP : List[int]
    The LCP (Longest Common Prefix) array. LCP[i] contains the length
    of the longest common prefix between suffixes SA[i-1] and SA[i].
    LCP[0] is typically 0.
n : int
    Length of the original text.

Examples
--------
>>> import lzcomplexity as lz
>>> caps = lz.CaPS("BANANA", 6)
>>> result = caps.construct()
>>> print(f"Suffix array: {result.SA}")
>>> print(f"LCP array: {result.LCP}")

Notes
-----
- The suffix array enables efficient string searching in O(m log n) time,
  where m is the pattern length and n is the text length.
- The LCP array, combined with the suffix array, enables O(m + log n)
  string searching and various other string algorithms.
- This class supports Python's pickle protocol for serialization.
)pbdoc");

  // Constructors
  LZ_SuffixArray.def(py::init(), "Create an empty LZ_SuffixArray object.")
    .def(py::init<std::vector<lz::lz_uint>, std::vector<lz::lz_uint>, lz::lz_uint>(),
         "SA_"_a,
         "LCP_"_a,
         "n_"_a,
         "Create LZ_SuffixArray with suffix array, LCP array, and text length.")
    .def(py::init<std::vector<lz::lz_uint>, lz::lz_uint>(),
         "SA_"_a,
         "n_"_a,
         "Create LZ_SuffixArray with suffix array only (no LCP).")
    .def(py::init<lz::lz_uint* const, lz::lz_uint* const, lz::lz_uint>(),
         "SA_"_a,
         "LCP_"_a,
         "n_"_a,
         "Create LZ_SuffixArray from raw pointers.")
    .def(py::self == py::self, "Check equality between two LZ_SuffixArray objects.")
    .def("Clear",
         &utl::LZ_SuffixArray::Clear,
         R"pbdoc(
Clear the suffix array and LCP array.

Resets both arrays to empty state and sets n to 0.
)pbdoc")
    .def("__copy__", [](const utl::LZ_SuffixArray& self) { return utl::LZ_SuffixArray(self); })
    .def("__repr__",
         [](const utl::LZ_SuffixArray& self) { return "LZ_SuffixArray(n=" + std::to_string(self.n) + ")"; })
    .def(
      "__deepcopy__",
      [](const utl::LZ_SuffixArray& self, py::dict) { return utl::LZ_SuffixArray(self); },
      "memo"_a)
    .def("__getstate__", [](const utl::LZ_SuffixArray& sa) { return std::make_tuple(sa.SA, sa.LCP, sa.n); })
    .def("__setstate__",
         [](utl::LZ_SuffixArray&                                                               sa,
            const std::tuple<std::vector<lz::lz_uint>, std::vector<lz::lz_uint>, lz::lz_uint>& state) {
           new (&sa) utl::LZ_SuffixArray(std::get<0>(state), std::get<1>(state), std::get<2>(state));
         });

  // Attributes with docstrings
  LZ_SuffixArray
    .def_rw("SA",
            &utl::LZ_SuffixArray::SA,
            "List[int]: Suffix array. SA[i] = starting position of i-th smallest suffix.")
    .def_rw("LCP",
            &utl::LZ_SuffixArray::LCP,
            "List[int]: LCP array. LCP[i] = longest common prefix length between SA[i-1] and SA[i].")
    .def_rw("n", &utl::LZ_SuffixArray::n, "int: Length of the original text.");
}