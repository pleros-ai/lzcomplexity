#include <lz/structures.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace py  = nanobind;
namespace utl = lz::utils;

void PyStructures(py::module_& m) {
   using namespace py::literals;

   py::class_<utl::LZ_Shuffle> LZ_Shuffle(m, "LZ_Shuffle");
   // Define constructors and methods
   LZ_Shuffle.def(py::init())
      .def(py::init<lz::lz_int, lz::lz_double, lz::lz_double, std::vector<lz::lz_double>>(),
           "max_block_size_"_a,
           "excess_value_"_a,
           "multi_information_"_a,
           "summands_"_a = std::vector<lz::lz_double>())
      .def("__str__",
           [](const utl::LZ_Shuffle& self) {
              return "shuffle entropy deficit: " + std::to_string(self.excess_value) +
                     ", block size: " + std::to_string(self.max_block_size);
           })
      .def("__copy__", [](const utl::LZ_Shuffle& self) { return utl::LZ_Shuffle(self); })
      .def(
         "__deepcopy__", [](const utl::LZ_Shuffle& self, py::dict) { return utl::LZ_Shuffle(self); }, "memo"_a);
   // Define attributes
   LZ_Shuffle.def_rw("max_block_size", &utl::LZ_Shuffle::max_block_size, "Max block size for shuffle entropy deficit.")
      .def_rw("excess_value", &utl::LZ_Shuffle::excess_value, "Shuffle entropy deficit value.")
      .def_rw("multi_information", &utl::LZ_Shuffle::multi_information, "Multi information value (block_size = 1).")
      .def_rw("summands", &utl::LZ_Shuffle::summands, "The terms form shuffle entropy deficit sum.");

   py::class_<utl::LZ_Args> LZ_Args(m, "LZ_Args");
   // Define constructors and methods
   LZ_Args.def(py::init())
      .def(py::init<lz::lz_int>(), "_chunks"_a)
      .def(py::init<lz::lz_int, lz::lz_int>(), "_chunks"_a, "_block_size"_a)
      .def(py::init<lz::lz_int, lz::lz_int, lz::lz_uint>(), "_chunks"_a, "_block_size"_a, "alphabet_"_a)
      .def(py::init<lz::lz_int, lz::lz_int, lz::lz_uint, lz::lz_uint>(),
           "_chunks"_a,
           "_block_size"_a,
           "alphabet_"_a,
           "log_base"_a)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__copy__", [](const utl::LZ_Args& self) { return utl::LZ_Args(self); })
      .def(
         "__deepcopy__", [](const utl::LZ_Args& self, py::dict) { return utl::LZ_Args(self); }, "memo"_a);
   // Define attributes
   LZ_Args
      .def_rw("chunks", &utl::LZ_Args::chunks, "Number of partitions for parallel suffix array algorithm")
      // .def_rw("max_context", &utl::LZ_Args::max_context, "Max context for lexicographical comparison")
      .def_rw("get_shuffle_terms", &utl::LZ_Args::get_shuffle_terms, "Flag for shuffle entropy deficit")
      .def_rw("alphabet", &utl::LZ_Args::alphabet, "Size of the alphabet")
      .def_rw("log_base", &utl::LZ_Args::log_base, "Alphabet base (most of the time is the same as the alphabet size)")
      .def_rw("block_size", &utl::LZ_Args::block_size, "Max size of the block for shuffle entropy deficit");
}