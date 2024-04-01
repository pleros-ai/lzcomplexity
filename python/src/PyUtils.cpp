// #include <lz/flags.h>
#include <lz/sa_structures.h>
#include <lz/types.h>
#include <lz/utils.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py  = pybind11;
namespace utl = lz::utils;

void PyUtils(py::module& m) {
   using namespace pybind11::literals;

   // py::enum_<utl::SA_ALGS>(m, "SA_ALGS")
   //     .value("sais", utl::SA_ALGS::sais)
   //     .value("caps", utl::SA_ALGS::caps)
   //     .export_values();

   py::enum_<utl::INPUT_FLAG>(m, "INPUT_FLAG")
      .value("text", utl::INPUT_FLAG::text)
      .value("path", utl::INPUT_FLAG::path)
      .export_values();

   py::class_<utl::LZ_Extra> LZ_Extra(m, "LZ_Extra");
   LZ_Extra.def(py::init())
      .def_readwrite("lz_rajski_distance", &utl::LZ_Extra::lz_rajski_distance)
      .def_readwrite("redundancy", &utl::LZ_Extra::redundancy)
      .def_readwrite("fh_uncertainty", &utl::LZ_Extra::fh_uncertainty)
      .def_readwrite("lh_uncertainty", &utl::LZ_Extra::lh_uncertainty)
      .def_readwrite("lz_pearson_coefficient", &utl::LZ_Extra::lz_pearson_coefficient);

   // py::class_<utl::LZ_FLAGS> LZ_FLAGS(m, "LZ_FLAGS");
   // // Define constructors and methods
   // LZ_FLAGS.def(py::init<std::string, lz::lz_int, lz::lz_int>(),
   //    "input"_a, "chunks"_a = 0, "max_conext"_a = 0)
   //    .def(py::init<std::vector<std::string>, lz::lz_int, lz::lz_int>(),
   //       "input"_a, "chunks"_a = 0, "max_context"_a = 0)
   //    .def(py::init<utl::sa_type&, std::string, lz::lz_int, lz::lz_int>(),
   //       "alg"_a, "input"_a, "chunks"_a = 0, "max_context"_a = 0)
   //    .def(py::init<utl::sa_type&, std::vector<std::string>, lz::lz_int, lz::lz_int>(),
   //       "alg"_a, "input"_a, "chunks"_a = 0, "max_context"_a = 0)
   //    .def(py::self == py::self)
   //    .def("__copy__", [](const utl::LZ_FLAGS& self) { return utl::LZ_FLAGS(self); })
   //    .def("__deepcopy__", [](const utl::LZ_FLAGS& self, py::dict) { return utl::LZ_FLAGS(self); }, "memo"_a)
   //    .def("addData", py::overload_cast<std::string>(&utl::LZ_FLAGS::addData), R"pbdoc(Add a new string to the input
   //    data)pbdoc", py::arg("data")) .def("addData",
   //    py::overload_cast<std::vector<std::string>>(&utl::LZ_FLAGS::addData), R"pbdoc(Add a new set of string to the
   //    input data)pbdoc", py::arg("data"));
   // // Define attributes
   // LZ_FLAGS.def_readwrite("algorithm", &utl::LZ_FLAGS::algorithm, "The algorithm used to construct the suffix array")
   //    .def_readwrite("sa_impl", &utl::LZ_FLAGS::sa_impl, "The suffix array implementation class")
   //    .def_readwrite("input", &utl::LZ_FLAGS::input, "An array of strings to be processed")
   //    .def_readwrite("chunks_count", &utl::LZ_FLAGS::chunks_count, "The number of chunks to split the input data
   //    (uses in CaPS algorithm)") .def_readwrite("max_context", &utl::LZ_FLAGS::max_context, "The maximum context
   //    length to consider in comparisons (uses in CaPS algorithm)") .def_readwrite("complexity",
   //    &utl::LZ_FLAGS::complexity, "The complexity value calculated from input data")
   //    .def_readwrite("entropy_density", &utl::LZ_FLAGS::entropy_density, "The entropy density calculated from input
   //    data") .def_readwrite("excess_entropy_mi", &utl::LZ_FLAGS::excess_entropy_mi, "The excess of entropy as mutual
   //    information calculated form input data") .def_readwrite("info_distance", &utl::LZ_FLAGS::info_distance, "The
   //    information distance for two consecutive string in input data (uses for multiple inputs)");

   // py::class_<utl::LempelZiv> LempelZiv(m, "LempelZiv");
   // LempelZiv.def(py::init<>())
   //    .def("__copy__", [](const utl::LempelZiv& self) { return utl::LempelZiv(self); })
   //    .def("__deepcopy__", [](const utl::LempelZiv& self, py::dict) { return utl::LempelZiv(self); }, "memo"_a);

   // LempelZiv.def_readwrite("complexity", &utl::LempelZiv::complexity, "The complexity value calculated from input
   // data")
   //    .def_readwrite("entropy_density", &utl::LempelZiv::entropy_density, "The entropy density calculated from input
   //    data") .def_readwrite("excess_entropy_mi", &utl::LempelZiv::excess_entropy_mi, "The excess of entropy as mutual
   //    information calculated form input data") .def_readwrite("excess_entropy_dist",
   //    &utl::LempelZiv::excess_entropy_dist, "The excess of entropy by the distance of the sequence")
   //    .def_readwrite("sequence_info_distance", &utl::LempelZiv::sequence_info_distance, "The information distance of
   //    each sequences") .def_readwrite("info_distance", &utl::LempelZiv::info_distance, "The information distance for
   //    two consecutive string in input data (uses for multiple inputs)");
}