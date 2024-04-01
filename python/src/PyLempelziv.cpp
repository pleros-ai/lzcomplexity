#include <lz/lempelziv.h>
#include <pybind11/stl.h>

namespace py  = pybind11;
namespace utl = lz::utils;

auto LempelZivFactorizationWithoutArgs(const lz::sequence& seq) {
   return lz::LempelZivFactorization(seq);
}
auto LempelZivFactorizationStringWithoutArgs(const std::string& seq) {
   return lz::LempelZivFactorization(seq);
}
auto LempelZivFactorizationWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactorization(seq, args);
}
auto LempelZivFactorizationStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactorization(seq, args);
}

// Python style function
auto LempelZivFactorization(const lz::sequence& seq,
                            lz::lz_int          chunks   = 0,
                            lz::lz_uint         alphabet = lz::ALPHABET_SIZE,
                            lz::lz_uint         log_base = lz::ALPHABET_SIZE) {
   utl::LZ_Args args;
   args.chunks   = chunks;
   args.alphabet = alphabet;
   args.log_base = log_base;
   return lz::LempelZivFactorization(seq, args);
}

auto LempelZivFactorsWithoutArgs(const lz::sequence& seq) {
   return lz::LempelZivFactors(seq);
}
auto LempelZivFactorsStringWithoutArgs(const std::string& seq) {
   return lz::LempelZivFactors(seq);
}
auto LempelZivFactorsWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactors(seq, args);
}
auto LempelZivFactorsStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::LempelZivFactors(seq, args);
}

auto EntropyDensityWithoutArgs(const lz::sequence& seq) {
   return lz::EntropyDensity(seq);
}
auto EntropyDensityStringWithoutArgs(const std::string& seq) {
   return lz::EntropyDensity(seq);
}
auto EntropyDensityWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::EntropyDensity(seq, args);
}
auto EntropyDensityStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
   return lz::EntropyDensity(seq, args);
}

auto ExcessEntropyDistanceWithoutArgs(const lz::sequence& seq) {
   return lz::ExcessEntropyDistance(seq);
}
auto ExcessEntropyDistanceWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::ExcessEntropyDistance(seq, args);
}

auto LZEffectiveComplexityWithoutArgs(const lz::sequence& seq) {
   return lz::LZEffectiveComplexity(seq);
}
auto LZEffectiveComplexityStringWithoutArgs(const std::string& seq) {
   return lz::LZEffectiveComplexity(seq);
}
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

auto ShuffleEntropyDeficitWithoutArgs(const lz::sequence& seq) {
   return lz::RandomShuffleComplexity(seq);
}
auto ShuffleEntropyDeficitStringWithoutArgs(const std::string& seq) {
   return lz::RandomShuffleComplexity(seq);
}
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

// Calculate all
auto LZ(const lz::sequence& seq, utl::LZ_Args& args) {
   return lz::LZ(seq, args);
}
auto LZWithString(const std::string& seq, utl::LZ_Args& args) {
   return lz::LZ(lz::sequence{seq}, args);
}

void PyLempelZiv(py::module& m) {
   using namespace pybind11::literals;

   py::class_<lz::lz76::LZ_Result> LZ_Result(m, "LZ_Result");
   LZ_Result.def(py::init())
      .def("__copy__", [](const lz::lz76::LZ_Result& self) { return lz::lz76::LZ_Result(self); })
      .def(
         "__deepcopy__", [](const lz::lz76::LZ_Result& self, py::dict) { return lz::lz76::LZ_Result(self); }, "memo"_a)
      .def_readwrite("factorization", &lz::lz76::LZ_Result::factorization)
      .def_readwrite("lzf", &lz::lz76::LZ_Result::lzf);

   py::class_<utl::LempelZiv> LempelZiv(m, "LempelZiv");
   LempelZiv.def(py::init())
      .def(py::init<lz::lz_uint,
                    std::vector<lz::lz_uint>,
                    lz::lz_double,
                    utl::LZ_Shuffle,
                    utl::LZ_Shuffle,
                    lz::lz_double,
                    lz::lz_double,
                    lz::lz_double,
                    lz::lz_double,
                    utl::LZ_Extra>(),
           "_complexity"_a,
           "lzf"_a,
           "_entropy_density"_a,
           "_whole_random_shuffle_complexity"_a,
           "_random_shuffle_complexity"_a,
           "_lz_normal_errors"_a,
           "_lz_poison_errors"_a,
           "eps"_a,
           "f_stddev"_a,
           "_extra"_a)
      .def("__copy__", [](const utl::LempelZiv& self) { return utl::LempelZiv(self); })
      .def(
         "__deepcopy__", [](const utl::LempelZiv& self, py::dict) { return utl::LempelZiv(self); }, "memo"_a);

   LempelZiv.def_property_readonly("complexity", &utl::LempelZiv::getComplexity)
      .def_property_readonly("entropy", &utl::LempelZiv::getEntropyDensity)
      .def_property_readonly("random_shuffle_complexity", &utl::LempelZiv::getRandomShuffleComplexity)
      .def_property_readonly("whole_random_shuffle_complexity", &utl::LempelZiv::getWholeRandomShuffleComplexity)
      .def_property_readonly("lz_normal_error", &utl::LempelZiv::getNormalError)
      .def_property_readonly("lz_poison_error", &utl::LempelZiv::getPoisonError)
      .def_property_readonly("extras", &utl::LempelZiv::getExtras)
      .def_property_readonly("factors", &utl::LempelZiv::getFactors)
      .def_property_readonly("factors_stddev", &utl::LempelZiv::getFactorsStddev);

   m.def("LZ", &::LZ)
      .def("LZ", &::LZWithString)
      .def("LempelZivFactorization", &::LempelZivFactorizationWithoutArgs)
      .def("LempelZivFactorization", &::LempelZivFactorizationWithArgs)
      .def("LempelZivFactorization", &::LempelZivFactorizationStringWithoutArgs)
      .def("LempelZivFactorization", &::LempelZivFactorizationStringWithArgs)
      // Function with all args
      .def("LempelZivFactorization", &::LempelZivFactorization, "seq"_a, "partitions"_a, "alphabet"_a, "log_base"_a)
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