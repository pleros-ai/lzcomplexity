#include <lz/structures.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py = pybind11;
namespace utl = lz::utils;

void PyStructures(py::module& m) {
   using namespace pybind11::literals;

   py::class_<utl::LZ_ExcessInfo> LZ_ExcessInfo(m, "LZ_ExcessInfo");
   // Define constructors and methods
   LZ_ExcessInfo.def(py::init())
       .def(py::init<lz::lz_int, lz::lz_double, lz::lz_double>(), "max_block_size_"_a, "excess_value_"_a,
            "multi_information_"_a)
       .def("__str__",
            [](const utl::LZ_ExcessInfo& self) {
               return "shuffle entropy deficit: " + std::to_string(self.excess_value) +
                      ", block size: " + std::to_string(self.max_block_size);
            })
       .def("__copy__", [](const utl::LZ_ExcessInfo& self) { return utl::LZ_ExcessInfo(self); })
       .def(
           "__deepcopy__", [](const utl::LZ_ExcessInfo& self, py::dict) { return utl::LZ_ExcessInfo(self); }, "memo"_a);
   // Define attributes
   LZ_ExcessInfo
       .def_readwrite("max_block_size", &utl::LZ_ExcessInfo::max_block_size,
                      "Max block size for shuffle entropy deficit.")
       .def_readwrite("excess_value", &utl::LZ_ExcessInfo::excess_value, "Shuffle entropy deficit value.")
       .def_readwrite("multi_information", &utl::LZ_ExcessInfo::multi_information,
                      "Multi information value (block_size = 1).")
       .def_readwrite("excess_by_terms", &utl::LZ_ExcessInfo::excess_by_terms,
                      "The terms form shuffle entropy deficit sum.");

   py::class_<utl::LZ_Args> LZ_Args(m, "LZ_Args");
   // Define constructors and methods
   LZ_Args.def(py::init())
       .def(py::init<lz::lz_int>(), "_chunks"_a)
       .def(py::init<lz::lz_int, lz::lz_int>(), "_chunks"_a, "_max_context"_a)
       .def(py::init<lz::lz_int, lz::lz_int, lz::lz_int>(), "_chunks"_a, "_max_context"_a, "_block_size"_a)
       .def(py::init<lz::lz_int, lz::lz_int, lz::lz_int, lz::lz_uint>(), "_chunks"_a, "_max_context"_a, "_block_size"_a,
            "alphabet_"_a)
       .def(py::self == py::self)
       .def("__copy__", [](const utl::LZ_Args& self) { return utl::LZ_Args(self); })
       .def(
           "__deepcopy__", [](const utl::LZ_Args& self, py::dict) { return utl::LZ_Args(self); }, "memo"_a);
   // Define attributes
   LZ_Args.def_readwrite("chunks", &utl::LZ_Args::chunks, "Number of partitions for parallel suffix array algorithm")
       .def_readwrite("max_context", &utl::LZ_Args::max_context, "Max context for lexicographical comparison")
       .def_readwrite("get_shuffle_terms", &utl::LZ_Args::get_shuffle_terms, "Flag for shuffle entropy deficit")
       .def_readwrite("alphabet", &utl::LZ_Args::alphabet, "Size of the alphabet")
       .def_readwrite("log_base", &utl::LZ_Args::log_base,
                      "Alphabet base (most of the time is the same as the alphabet size)")
       .def_readwrite("block_size", &utl::LZ_Args::block_size, "Max size of the block for shuffle entropy deficit");
}