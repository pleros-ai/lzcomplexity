#include <lz/caps.h>
#include <lz/sais_lite.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <tuple>

// #include "lz_opaques.h"

namespace py  = nanobind;
namespace suf = lz::suffixarray;
namespace utl = lz::utils;

std::unique_ptr<suf::CaPS_SA>
   constructor_str(std::string T, lz::lz_int n, lz::lz_int subproblem_count, lz::lz_int max_context) {
   return std::unique_ptr<suf::CaPS_SA>{new suf::CaPS_SA({T.begin(), T.end()}, n, subproblem_count, max_context)};
}

void PyCaps(py::module_& m) {
   using namespace py::literals;

   py::class_<suf::CaPS_SA> caps(m, "CaPS");
   // Constructors and methods
   // Check why with subproblem_count = 0 BOOM!!! when should initialize p_ with default_subproblem_count
   caps
      .def(py::init<std::vector<lz::lz_char>, lz::lz_int, lz::lz_int, lz::lz_int>(),
           "T"_a,
           "n"_a,
           "subproblem_count"_a = 0,
           "max_context"_a      = 0)
      .def(py::new_(&constructor_str), "T"_a, "n"_a, "subproblem_count"_a = 0u, "max_context"_a = 0u)
      .def(py::init<lz::lz_int, lz::lz_int>(), "subproblem_count"_a = 0u, "max_context"_a = 0u)
      .def(py::init<utl::SA_Args>(), "args"_a)
      .def("__copy__", [](const suf::CaPS_SA& self) { return suf::CaPS_SA(self); })
      .def(
         "__deepcopy__", [](const suf::CaPS_SA& self, py::dict) { return suf::CaPS_SA(self); }, "memo"_a)
      .def(
         "construct",
         [](suf::CaPS_SA& self) { return self.construct(); },
         "Generate the suffix array from the text use for build the class",
         py::rv_policy::copy)
      .def("construct",
           py::overload_cast<std::vector<lz::lz_char>, lz::lz_int>(&suf::CaPS_SA::construct),
           "Generate the suffix array from a vector of characters",
           py::rv_policy::copy)
      .def("construct",
           py::overload_cast<const std::string&>(&suf::CaPS_SA::construct),
           "Generate the suffix array from the text",
           py::rv_policy::copy);

   // Attributes
   caps.def_prop_ro("n_", &suf::CaPS_SA::n)
      .def_prop_ro("T_", &suf::CaPS_SA::T)
      .def_prop_ro("LCP_", &suf::CaPS_SA::LCP)
      .def_prop_ro("SA_", &suf::CaPS_SA::SA);
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

   py::class_<utl::LZ_SuffixArray> LZ_SuffixArray(m, "LZ_SuffixArray");

   // Constructors and methods
   LZ_SuffixArray.def(py::init())
      .def(py::init<std::vector<lz::lz_uint>, std::vector<lz::lz_uint>, lz::lz_uint>(), "SA_"_a, "LCP_"_a, "n_"_a)
      .def(py::init<std::vector<lz::lz_uint>, lz::lz_uint>(), "SA_"_a, "n_"_a)
      .def(py::init<lz::lz_uint* const, lz::lz_uint* const, lz::lz_uint>(), "SA_"_a, "LCP_"_a, "n_"_a)
      // .def(py::init<lz::lz_int* const, lz::lz_int* const, lz::lz_uint>(), "SA_"_a, "LCP_"_a, "n_"_a)
      .def(py::self == py::self)
      .def("Clear", &utl::LZ_SuffixArray::Clear, "Clear the SA and LCP arrays")
      .def("__copy__", [](const utl::LZ_SuffixArray& self) { return utl::LZ_SuffixArray(self); })
      .def(
         "__deepcopy__", [](const utl::LZ_SuffixArray& self, py::dict) { return utl::LZ_SuffixArray(self); }, "memo"_a)
      .def("__getstate__", [](const utl::LZ_SuffixArray& sa) { return std::make_tuple(sa.SA, sa.LCP, sa.n); })
      .def("__setstate__",
           [](utl::LZ_SuffixArray&                                                               sa,
              const std::tuple<std::vector<lz::lz_uint>, std::vector<lz::lz_uint>, lz::lz_uint>& state) {
              new (&sa) utl::LZ_SuffixArray(std::get<0>(state), std::get<1>(state), std::get<2>(state));
           });

   // Attributes
   LZ_SuffixArray.def_rw("SA", &utl::LZ_SuffixArray::SA)
      .def_rw("LCP", &utl::LZ_SuffixArray::LCP)
      .def_rw("n", &utl::LZ_SuffixArray::n);
}