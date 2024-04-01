#include <lz/lempelziv.h>
#include <pybind11/stl.h>

namespace py = pybind11;
namespace utl = lz::utils;

auto LempelZivFactorizationWithoutArgs(const lz::sequence& seq) { return lz::LempelZivFactorization(seq); }
auto LempelZivFactorizationStringWithoutArgs(const std::string& seq) { return lz::LempelZivFactorization(seq); }
auto LempelZivFactorizationWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactorization(seq, args);
}
auto LempelZivFactorizationStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactorization(seq, args);
}

auto LempelZivFactorsWithoutArgs(const lz::sequence& seq) { return lz::LempelZivFactors(seq); }
auto LempelZivFactorsStringWithoutArgs(const std::string& seq) { return lz::LempelZivFactors(seq); }
auto LempelZivFactorsWithArgs(const lz::sequence& seq, utl::LZ_Args& args) { return lz::LempelZivFactors(seq, args); }
auto LempelZivFactorsStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactors(seq, args);
}

auto EntropyDensityWithoutArgs(const lz::sequence& seq) { return lz::EntropyDensity(seq); }
auto EntropyDensityStringWithoutArgs(const std::string& seq) { return lz::EntropyDensity(seq); }
auto EntropyDensityWithArgs(const lz::sequence& seq, utl::LZ_Args& args) { return lz::EntropyDensity(seq, args); }
auto EntropyDensityStringWithArgs(const std::string& seq, utl::LZ_Args& args) { return lz::EntropyDensity(seq, args); }

auto ExcessEntropyDistanceWithoutArgs(const lz::sequence& seq) { return lz::ExcessEntropyDistance(seq); }
auto ExcessEntropyDistanceWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::ExcessEntropyDistance(seq, args);
}

auto LZEffectiveComplexityWithoutArgs(const lz::sequence& seq) { return lz::LZEffectiveComplexity(seq); }
auto LZEffectiveComplexityStringWithoutArgs(const std::string& seq) { return lz::LZEffectiveComplexity(seq); }
auto LZEffectiveComplexityWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::LZEffectiveComplexity(seq, args);
}
auto LZEffectiveComplexityStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::LZEffectiveComplexity(seq, args);
}
auto LZEffectiveComplexityNormalizedWithoutArgs(const lz::sequence& seq) {
   return lz::LZEffectiveComplexityNormalized(seq);
}
auto LZEffectiveComplexityNormalizedStringWithoutArgs(const std::string& seq) {
   return lz::LZEffectiveComplexityNormalized(seq);
}
auto LZEffectiveComplexityNormalizedWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::LZEffectiveComplexityNormalized(seq, args);
}
auto LZEffectiveComplexityNormalizedStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::LZEffectiveComplexityNormalized(seq, args);
}

auto ShuffleEntropyDeficitWithoutArgs(const lz::sequence& seq) { return lz::RandomShuffleComplexity(seq); }
auto ShuffleEntropyDeficitStringWithoutArgs(const std::string& seq) { return lz::RandomShuffleComplexity(seq); }
auto ShuffleEntropyDeficitWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::RandomShuffleComplexity(seq, args);
}
auto ShuffleEntropyDeficitStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::RandomShuffleComplexity(seq, args);
}

auto InformationsDistanceWithoutArgs(const lz::sequence& seq1, const lz::sequence& seq2) {
   return lz::InformationDistance(seq1, seq2);
}
auto InformationsDistanceWithArgs(const lz::sequence& seq1, const lz::sequence& seq2, utl::LZ_Args& args) {
   return lz::InformationDistance(seq1, seq2, args);
}

void PyLempelZiv(py::module& m) {
   using namespace pybind11::literals;

   py::class_<lz::lz76::LZ76_Result> LZ76_Result(m, "LZ76_Result");
   LZ76_Result.def(py::init())
       .def("__copy__", [](const lz::lz76::LZ76_Result& self) { return lz::lz76::LZ76_Result(self); })
       .def(
           "__deepcopy__", [](const lz::lz76::LZ76_Result& self, py::dict) { return lz::lz76::LZ76_Result(self); },
           "memo"_a)
       .def_readwrite("factorization", &lz::lz76::LZ76_Result::factorization)
       .def_readwrite("lzf", &lz::lz76::LZ76_Result::lzf);

   m.def("LempelZivFactorization", &::LempelZivFactorizationWithoutArgs)
       .def("LempelZivFactorization", &::LempelZivFactorizationWithArgs)
       .def("LempelZivFactorization", &::LempelZivFactorizationStringWithoutArgs)
       .def("LempelZivFactorization", &::LempelZivFactorizationStringWithArgs)
       //!> Factor function
       .def("LempelZivFactors", &::LempelZivFactorsWithoutArgs)
       .def("LempelZivFactors", &::LempelZivFactorsWithArgs)
       .def("LempelZivFactors", &::LempelZivFactorsStringWithoutArgs)
       .def("LempelZivFactors", &::LempelZivFactorsStringWithArgs)
       //!> Entropy density
       .def("EntropyDensity", &::EntropyDensityWithoutArgs)
       .def("EntropyDensity", &::EntropyDensityWithArgs)
       .def("EntropyDensity", &::EntropyDensityStringWithoutArgs)
       .def("EntropyDensity", &::EntropyDensityStringWithArgs)
       //!> LZ effective complexity
       .def("LZEffectiveComplexity", &::LZEffectiveComplexityWithoutArgs)
       .def("LZEffectiveComplexity", &::LZEffectiveComplexityWithArgs)
       .def("LZEffectiveComplexityNormalized", &::LZEffectiveComplexityNormalizedWithoutArgs)
       .def("LZEffectiveComplexityNormalized", &::LZEffectiveComplexityNormalizedWithArgs)
       .def("LZEffectiveComplexity", &::LZEffectiveComplexityStringWithoutArgs)
       .def("LZEffectiveComplexity", &::LZEffectiveComplexityStringWithArgs)
       .def("LZEffectiveComplexityNormalized", &::LZEffectiveComplexityNormalizedStringWithoutArgs)
       .def("LZEffectiveComplexityNormalized", &::LZEffectiveComplexityNormalizedStringWithArgs)
       //!> Excess entropy by distance
       .def("ExcessEntropyDistance", &::ExcessEntropyDistanceWithoutArgs)
       .def("ExcessEntropyDistance", &::ExcessEntropyDistanceWithArgs)
       //!> Shuffle entropy deficit
       .def("ShuffleEntropyDeficit", &::ShuffleEntropyDeficitWithoutArgs)
       .def("ShuffleEntropyDeficit", &::ShuffleEntropyDeficitWithArgs)
       .def("ShuffleEntropyDeficit", &::ShuffleEntropyDeficitStringWithoutArgs)
       .def("ShuffleEntropyDeficit", &::ShuffleEntropyDeficitStringWithArgs)
       // Information distance
       .def("InformationDistance", &::InformationsDistanceWithoutArgs)
       .def("InformationDistance", &::InformationsDistanceWithArgs);
}