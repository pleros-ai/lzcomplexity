/**
 * @file PyUtils.cpp
 * @brief Python bindings for utility classes and enumerations.
 */

#include <lz/sa_structures.h>
#include <lz/types.h>
#include <lz/utils.h>
#include <nanobind/operators.h>

namespace py  = nanobind;
namespace utl = lz::utils;

void PyUtils(py::module_& m) {
   using namespace py::literals;

   // =========================================================================
   // INPUT_FLAG enumeration
   // =========================================================================
   py::enum_<utl::INPUT_FLAG>(m, "INPUT_FLAG", R"pbdoc(
Enumeration specifying the type of input data.

Values
------
text : INPUT_FLAG
    Input is provided as a raw text string.
path : INPUT_FLAG
    Input is provided as a file path to read from.
)pbdoc")
      .value("text", utl::INPUT_FLAG::text, "Input is a raw text string.")
      .value("path", utl::INPUT_FLAG::path, "Input is a file path.")
      .export_values();

   // =========================================================================
   // LZ_Extra class - Additional information-theoretic measures
   // =========================================================================
   py::class_<utl::LZ_Extra> LZ_Extra(m, "LZ_Extra", R"pbdoc(
Container for additional information-theoretic measures.

This class stores supplementary metrics computed during LZ76 analysis,
providing deeper insight into the statistical structure of sequences.
These measures are computed by comparing the first and second halves
of a sequence.

Attributes
----------
lz_rajski_distance : float
    Rajski distance between sequence halves. A normalized measure of
    the difference in information content, ranging from 0 (identical)
    to 1 (completely different). Computed as: 1 - I(X;Y) / H(X,Y)
    where I(X;Y) is mutual information and H(X,Y) is joint entropy.
redundancy : float
    Redundancy measure indicating the fraction of information that is
    shared between the two halves. Higher values indicate more
    predictability between halves.
fh_uncertainty : float
    First-half uncertainty (conditional entropy H(X|Y)). Measures how
    much uncertainty remains about the first half given knowledge of
    the second half.
lh_uncertainty : float
    Last-half (second-half) uncertainty (conditional entropy H(Y|X)).
    Measures how much uncertainty remains about the second half given
    knowledge of the first half.
lz_pearson_coefficient : float
    Pearson correlation coefficient between the complexity profiles
    of the two halves. Ranges from -1 to 1, where 1 indicates perfect
    positive correlation.

Examples
--------
>>> import lzcomplexity as lz
>>> result = lz.lz76("ABRACADABRA")
>>> extras = result.extras
>>> print(f"Rajski distance: {extras.lz_rajski_distance}")
>>> print(f"Redundancy: {extras.redundancy}")
)pbdoc");

   LZ_Extra.def(py::init(), "Create an empty LZ_Extra object.")
      .def("__repr__",
           [](const utl::LZ_Extra& self) {
              return "LZ_Extra(rajski=" + std::to_string(self.lz_rajski_distance) +
                     ", redundancy=" + std::to_string(self.redundancy) + ")";
           })
      .def_rw("lz_rajski_distance",
              &utl::LZ_Extra::lz_rajski_distance,
              "float: Rajski distance between sequence halves (0=identical, 1=different).")
      .def_rw("redundancy",
              &utl::LZ_Extra::redundancy,
              "float: Redundancy measure indicating shared information between halves.")
      .def_rw("fh_uncertainty",
              &utl::LZ_Extra::fh_uncertainty,
              "float: First-half conditional entropy H(X|Y). Uncertainty about first half given second.")
      .def_rw("lh_uncertainty",
              &utl::LZ_Extra::lh_uncertainty,
              "float: Last-half conditional entropy H(Y|X). Uncertainty about second half given first.")
      .def_rw("lz_pearson_coefficient",
              &utl::LZ_Extra::lz_pearson_coefficient,
              "float: Pearson correlation coefficient between complexity profiles of halves.");
}