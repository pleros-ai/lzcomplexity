// #include <lz/flags.h>
#include <lz/sa_structures.h>
#include <lz/types.h>
#include <lz/utils.h>
#include <nanobind/operators.h>

namespace py  = nanobind;
namespace utl = lz::utils;

void PyUtils(py::module_& m) {
   using namespace py::literals;

   py::enum_<utl::INPUT_FLAG>(m, "INPUT_FLAG")
      .value("text", utl::INPUT_FLAG::text)
      .value("path", utl::INPUT_FLAG::path)
      .export_values();

   py::class_<utl::LZ_Extra> LZ_Extra(m, "LZ_Extra");
   LZ_Extra.def(py::init())
      .def_rw("lz_rajski_distance", &utl::LZ_Extra::lz_rajski_distance)
      .def_rw("redundancy", &utl::LZ_Extra::redundancy)
      .def_rw("fh_uncertainty", &utl::LZ_Extra::fh_uncertainty)
      .def_rw("lh_uncertainty", &utl::LZ_Extra::lh_uncertainty)
      .def_rw("lz_pearson_coefficient", &utl::LZ_Extra::lz_pearson_coefficient);
}