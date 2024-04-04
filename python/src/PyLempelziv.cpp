#include <lz/lempelziv.h>
#include <pybind11/stl.h>

#include <variant>

using seq_type = std::variant<lz::sequence, std::string, std::vector<char>>;

namespace py  = pybind11;
namespace utl = lz::utils;

auto LempelZivFactorizationWithoutArgs(const seq_type& seq) {
   lz::lz_uint cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::LempelZivFactorization(s); }, seq);
   return cpx;
}
auto LempelZivFactorizationWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::lz_uint cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::LempelZivFactorization(s, args); }, seq);
   return cpx;
}

// Python style function
auto LempelZivFactorization(const seq_type& seq,
                            lz::lz_int      chunks   = 0,
                            lz::lz_uint     alphabet = lz::ALPHABET_SIZE,
                            lz::lz_uint     log_base = lz::ALPHABET_SIZE) {
   utl::LZ_Args args;
   args.chunks     = chunks;
   args.alphabet   = alphabet;
   args.log_base   = log_base;
   lz::lz_uint cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::LempelZivFactorization(s, args); }, seq);
   return cpx;
}

auto LempelZivFactorsWithoutArgs(const seq_type& seq) {
   lz::lz76::LZ_Result cpx;

   std::visit([&](auto&& s) { cpx = lz::LempelZivFactors(s); }, seq);
   return cpx;
}
auto LempelZivFactorsWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::lz76::LZ_Result cpx;

   std::visit([&](auto&& s) { cpx = lz::LempelZivFactors(s, args); }, seq);
   return cpx;
}

// Python style function
auto LempelZivFactors(const seq_type& seq,
                      lz::lz_int      chunks   = 0,
                      lz::lz_uint     alphabet = lz::ALPHABET_SIZE,
                      lz::lz_uint     log_base = lz::ALPHABET_SIZE) {
   utl::LZ_Args args;
   args.chunks   = chunks;
   args.alphabet = alphabet;
   args.log_base = log_base;
   lz::lz76::LZ_Result cpx;

   std::visit([&](auto&& s) { cpx = lz::LempelZivFactors(s, args); }, seq);
   return cpx;
}

auto EntropyDensityWithoutArgs(const seq_type& seq) {
   lz::lz_double cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::EntropyDensity(s); }, seq);
   return cpx;
}
auto EntropyDensityWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::lz_double cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::EntropyDensity(s, args); }, seq);
   return cpx;
}

// Python style function
auto EntropyDensity(const seq_type& seq,
                    lz::lz_int      chunks   = 0,
                    lz::lz_uint     alphabet = lz::ALPHABET_SIZE,
                    lz::lz_uint     log_base = lz::ALPHABET_SIZE) {
   utl::LZ_Args args;
   args.chunks       = chunks;
   args.alphabet     = alphabet;
   args.log_base     = log_base;
   lz::lz_double cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::EntropyDensity(s, args); }, seq);
   return cpx;
}

// auto ExcessEntropyDistanceWithoutArgs(const lz::sequence& seq) {
//    return lz::ExcessEntropyDistance(seq);
// }
// auto ExcessEntropyDistanceWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
//    return lz::ExcessEntropyDistance(seq, args);
// }

// auto LZEffectiveComplexityWithoutArgs(const lz::sequence& seq) {
//    return lz::LZEffectiveComplexity(seq);
// }
// auto LZEffectiveComplexityStringWithoutArgs(const std::string& seq) {
//    return lz::LZEffectiveComplexity(seq);
// }
// auto LZEffectiveComplexityWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
//    return lz::LZEffectiveComplexity(seq, args);
// }
// auto LZEffectiveComplexityStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
//    return lz::LZEffectiveComplexity(seq, args);
// }
// auto LZEffectiveComplexityNormalizedWithoutArgs(const lz::sequence& seq) {
//    return lz::LZEffectiveComplexityNormalized(seq);
// }
// auto LZEffectiveComplexityNormalizedStringWithoutArgs(const std::string& seq) {
//    return lz::LZEffectiveComplexityNormalized(seq);
// }
// auto LZEffectiveComplexityNormalizedWithArgs(const lz::sequence& seq, utl::LZ_Args& args) {
//    return lz::LZEffectiveComplexityNormalized(seq, args);
// }
// auto LZEffectiveComplexityNormalizedStringWithArgs(const std::string& seq, utl::LZ_Args& args) {
//    return lz::LZEffectiveComplexityNormalized(seq, args);
// }

auto RandomShuffleComplexityWithoutArgs(const seq_type& seq) {
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::RandomShuffleComplexity(s); }, seq);
   return cpx;
}
auto RandomShuffleComplexityWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::RandomShuffleComplexity(s, args); }, seq);
   return cpx;
}

// Python style function
auto RandomShuffleComplexity(const seq_type& seq,
                             lz::lz_int      chunks         = 0,
                             lz::lz_uint     alphabet       = lz::ALPHABET_SIZE,
                             lz::lz_uint     log_base       = lz::ALPHABET_SIZE,
                             lz::lz_int      max_block_size = -1) {
   utl::LZ_Args args;
   args.chunks     = chunks;
   args.alphabet   = alphabet;
   args.log_base   = log_base;
   args.block_size = max_block_size;
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::RandomShuffleComplexity(s, args); }, seq);
   return cpx;
}

auto InformationsDistanceWithoutArgs(const seq_type& seq1, const seq_type& seq2) {
   lz::lz_double cpx;

   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::InformationDistance(s1, s2); }, seq1, seq2);
   return cpx;
}
auto InformationsDistanceWithArgs(const seq_type& seq1, const seq_type& seq2, utl::LZ_Args& args) {
   lz::lz_double cpx;

   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::InformationDistanceZ(s1, s2, args); }, seq1, seq2);
   return cpx;
}

// Calculate all
auto LZ(const seq_type& seq, utl::LZ_Args& args) {
   lz::utils::LempelZiv cpx;

   std::visit([&](auto&& s) { cpx = lz::LZ(s, args); }, seq);
   return cpx;
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
           "_lz_factors"_a,
           "_entropy_density"_a,
           "_whole_random_shuffle_complexity"_a,
           "_random_shuffle_complexity"_a,
           "_lz_normal_errors"_a,
           "_lz_poison_errors"_a,
           "_eps"_a,
           "_factors_stddev"_a,
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

   m.def("LZ", &::LZ, "seq"_a, "args"_a)
      //!> LZ76 factorization
      .def("LempelZivFactorization", &::LempelZivFactorizationWithoutArgs, "seq"_a)
      .def("LempelZivFactorization", &::LempelZivFactorizationWithArgs, "seq"_a, "args"_a)
      //!> Function with all args
      .def("LempelZivFactorization", &::LempelZivFactorization, "seq"_a, "partitions"_a, "alphabet"_a, "log_base"_a)
      //!> Factor function
      .def("LempelZivFactors", &::LempelZivFactorsWithoutArgs, "seq"_a)
      .def("LempelZivFactors", &::LempelZivFactorsWithArgs, "seq"_a, "args"_a)
      .def("LempelZivFactors", &::LempelZivFactors, "seq"_a, "partitions"_a, "alphabet"_a, "log_base"_a)
      //!> Entropy density
      .def("EntropyDensity", &::EntropyDensityWithoutArgs, "seq"_a)
      .def("EntropyDensity", &::EntropyDensityWithArgs, "seq"_a, "args"_a)
      .def("EntropyDensity", &::EntropyDensity, "seq"_a, "partitions"_a, "alphabet"_a, "log_base"_a)
      //!> Shuffle entropy deficit
      .def("RandomShuffleComplexity", &::RandomShuffleComplexityWithoutArgs, "seq"_a)
      .def("RandomShuffleComplexity", &::RandomShuffleComplexityWithArgs, "seq"_a, "args"_a)
      .def("RandomShuffleComplexity",
           &::RandomShuffleComplexity,
           "seq"_a,
           "partitions"_a,
           "alphabet"_a,
           "log_base"_a,
           "max_block_size"_a)
      // Information distance
      .def("InformationDistance", &::InformationsDistanceWithoutArgs, "seq1"_a, "seq2"_a)
      .def("InformationDistance", &::InformationsDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a);
}