#include <lz/lempelziv.h>
#include <lz/utils.h>
#include <pybind11/stl.h>

#include <variant>

using seq_type = std::variant<lz::sequence, std::string, std::vector<char>>;

namespace py  = pybind11;
namespace utl = lz::utils;

auto lz76FactorizationWithoutArgs(const seq_type& seq) {
   lz::lz_uint cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::lz76Factorization(s); }, seq);
   return cpx;
}
auto lz76FactorizationWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::lz_uint cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::lz76Factorization(s, args); }, seq);
   return cpx;
}

// Python style function
auto lz76Factorization(const seq_type& seq,
                       lz::lz_int      partitions = 1,
                       lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                       lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                       lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks     = partitions;
   args.alphabet   = alphabet;
   args.log_base   = log_base;
   lz::lz_uint cpx = 0;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s) { cpx = lz::lz76Factorization(s, args); }, seq);
   lz::utils::DisabledMT();
   return cpx;
}

auto lz76FactorsWithoutArgs(const seq_type& seq) {
   lz::internal::LZ_Result cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76Factors(s); }, seq);
   return cpx;
}
auto lz76FactorsWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::internal::LZ_Result cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76Factors(s, args); }, seq);
   return cpx;
}

// Python style function
auto lz76Factors(const seq_type& seq,
                 lz::lz_int      partitions = 1,
                 lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                 lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                 lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks   = partitions;
   args.alphabet = alphabet;
   args.log_base = log_base;
   lz::internal::LZ_Result cpx;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s) { cpx = lz::lz76Factors(s, args); }, seq);
   lz::utils::DisabledMT();
   return cpx;
}

auto lz76EntropyDensityWithoutArgs(const seq_type& seq) {
   lz::lz_double cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::lz76EntropyDensity(s); }, seq);
   return cpx;
}
auto lz76EntropyDensityWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::lz_double cpx = 0;

   std::visit([&](auto&& s) { cpx = lz::lz76EntropyDensity(s, args); }, seq);
   return cpx;
}

// Python style function
auto lz76EntropyDensity(const seq_type& seq,
                        lz::lz_int      partitions = 1,
                        lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                        lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                        lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks       = partitions;
   args.alphabet     = alphabet;
   args.log_base     = log_base;
   lz::lz_double cpx = 0;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s) { cpx = lz::lz76EntropyDensity(s, args); }, seq);
   lz::utils::DisabledMT();
   return cpx;
}

auto lz76RandomShuffleComplexityWithoutArgs(const seq_type& seq) {
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76RandomShuffleComplexity(s); }, seq);
   return cpx;
}
auto lz76RandomShuffleComplexityWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76RandomShuffleComplexity(s, args); }, seq);
   return cpx;
}

// Python style function
auto lz76RandomShuffleComplexity(const seq_type& seq,
                                 lz::lz_int      partitions     = 1,
                                 lz::lz_uint     alphabet       = lz::ALPHABET_SIZE,
                                 lz::lz_uint     log_base       = lz::ALPHABET_SIZE,
                                 lz::lz_int      max_block_size = -1,
                                 lz::lz_uint     jobs           = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks     = partitions;
   args.alphabet   = alphabet;
   args.log_base   = log_base;
   args.block_size = max_block_size;
   lz::utils::LZ_Shuffle cpx;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s) { cpx = lz::lz76RandomShuffleComplexity(s, args); }, seq);
   lz::utils::DisabledMT();
   return cpx;
}

auto lz76WholeRandomShuffleComplexityWithoutArgs(const seq_type& seq) {
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76WholeRandomShuffleComplexity(s); }, seq);
   return cpx;
}
auto lz76WholeRandomShuffleComplexityWithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::utils::LZ_Shuffle cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76WholeRandomShuffleComplexity(s, args); }, seq);
   return cpx;
}

// Python style function
auto lz76WholeRandomShuffleComplexity(const seq_type& seq,
                                      lz::lz_int      partitions     = 1,
                                      lz::lz_uint     alphabet       = lz::ALPHABET_SIZE,
                                      lz::lz_uint     log_base       = lz::ALPHABET_SIZE,
                                      lz::lz_int      max_block_size = -1,
                                      lz::lz_uint     jobs           = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks     = partitions;
   args.alphabet   = alphabet;
   args.log_base   = log_base;
   args.block_size = max_block_size;
   lz::utils::LZ_Shuffle cpx;
   lz::utils::EnabledMT(jobs);
   std::visit([&](auto s) { cpx = lz::lz76WholeRandomShuffleComplexity(s, args); }, seq);
   lz::utils::DisabledMT();
   return cpx;
}

auto lz76InformationDistanceWithoutArgs(const seq_type& seq1, const seq_type& seq2) {
   lz::lz_double cpx;

   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::lz76InformationDistanceZ(s1, s2); }, seq1, seq2);
   return cpx;
}
auto lz76InformationDistanceWithArgs(const seq_type& seq1, const seq_type& seq2, utl::LZ_Args& args) {
   lz::lz_double cpx;

   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::lz76InformationDistanceZ(s1, s2, args); }, seq1, seq2);
   return cpx;
}

// Python style function
auto lz76InformationDistance(const seq_type& seq1,
                             const seq_type& seq2,
                             lz::lz_int      partitions = 1,
                             lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                             lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                             lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks   = partitions;
   args.alphabet = alphabet;
   args.log_base = log_base;
   lz::lz_double cpx;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::lz76InformationDistanceZ(s1, s2, args); }, seq1, seq2);
   lz::utils::DisabledMT();
   return cpx;
}

auto lz76RandomShuffleDistanceWithoutArgs(const seq_type& seq1, const seq_type& seq2) {
   lz::lz_double cpx;

   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::lz76RandomShuffleDistance(s1, s2); }, seq1, seq2);
   return cpx;
}
auto lz76RandomShuffleDistanceWithArgs(const seq_type& seq1, const seq_type& seq2, utl::LZ_Args& args) {
   lz::lz_double cpx;

   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::lz76RandomShuffleDistance(s1, s2, args); }, seq1, seq2);
   return cpx;
}

// Python style function
auto lz76RandomShuffleDistance(const seq_type& seq1,
                               const seq_type& seq2,
                               lz::lz_int      partitions = 1,
                               lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                               lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                               lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks   = partitions;
   args.alphabet = alphabet;
   args.log_base = log_base;
   lz::lz_double cpx;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s1, auto&& s2) { cpx = lz::lz76RandomShuffleDistance(s1, s2, args); }, seq1, seq2);
   lz::utils::DisabledMT();
   return cpx;
}

// Calculate all
auto lz76WithArgs(const seq_type& seq, utl::LZ_Args& args) {
   lz::utils::LempelZiv cpx;

   std::visit([&](auto&& s) { cpx = lz::lz76(s, args); }, seq);
   return cpx;
}

auto lz76(const seq_type& seq,
          lz::lz_int      partitions     = 1,
          lz::lz_uint     alphabet       = lz::ALPHABET_SIZE,
          lz::lz_uint     log_base       = lz::ALPHABET_SIZE,
          lz::lz_int      max_block_size = -1,
          lz::lz_uint     jobs           = std::thread::hardware_concurrency()) {
   utl::LZ_Args args;
   args.chunks     = partitions;
   args.alphabet   = alphabet;
   args.log_base   = log_base;
   args.block_size = max_block_size;
   lz::utils::LempelZiv cpx;

   lz::utils::EnabledMT(jobs);
   std::visit([&](auto&& s) { cpx = lz::lz76(s, args); }, seq);
   lz::utils::DisabledMT();
   return cpx;
}

void PyLempelZiv(py::module& m) {
   using namespace pybind11::literals;

   py::class_<lz::internal::LZ_Result> LZ_Result(m, "LZ_Result");
   LZ_Result.def(py::init())
      .def("__copy__", [](const lz::internal::LZ_Result& self) { return lz::internal::LZ_Result(self); })
      .def(
         "__deepcopy__",
         [](const lz::internal::LZ_Result& self, py::dict) { return lz::internal::LZ_Result(self); },
         "memo"_a)
      .def_readwrite("factorization", &lz::internal::LZ_Result::factorization)
      .def_readwrite("lzf", &lz::internal::LZ_Result::lzf);

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
      .def_property_readonly("whole_random_shuffle_complexity", &utl::LempelZiv::getAllRandomShuffleComplexity)
      .def_property_readonly("lz_normal_error", &utl::LempelZiv::getNormalError)
      .def_property_readonly("lz_poison_error", &utl::LempelZiv::getPoisonError)
      .def_property_readonly("extras", &utl::LempelZiv::getExtras)
      .def_property_readonly("factors", &utl::LempelZiv::getFactors)
      .def_property_readonly("factors_stddev", &utl::LempelZiv::getFactorsStddev);

   m.def("lz76", &::lz76WithArgs, "seq"_a, "args"_a)
      .def("lz76",
           &::lz76,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      //!> LZ76 factorization
      .def("lz76Factorization", &::lz76FactorizationWithoutArgs, "seq"_a)
      .def("lz76Factorization", &::lz76FactorizationWithArgs, "seq"_a, "args"_a)
      //!> Function with all args
      .def("lz76Factorization",
           &::lz76Factorization,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Factor function
      .def("lz76Factors", &::lz76FactorsWithoutArgs, "seq"_a)
      .def("lz76Factors", &::lz76FactorsWithArgs, "seq"_a, "args"_a)
      .def("lz76Factors",
           &::lz76Factors,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Entropy density
      .def("lz76EntropyDensity", &::lz76EntropyDensityWithoutArgs, "seq"_a)
      .def("lz76EntropyDensity", &::lz76EntropyDensityWithArgs, "seq"_a, "args"_a)
      .def("lz76EntropyDensity",
           &::lz76EntropyDensity,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Random shuffle complexity
      .def("lz76RandomShuffleComplexity", &::lz76RandomShuffleComplexityWithoutArgs, "seq"_a)
      .def("lz76RandomShuffleComplexity", &::lz76RandomShuffleComplexityWithArgs, "seq"_a, "args"_a)
      .def("lz76RandomShuffleComplexity",
           &::lz76RandomShuffleComplexity,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      .def("lz76WholeRandomShuffleComplexity", &::lz76WholeRandomShuffleComplexityWithoutArgs, "seq"_a)
      .def("lz76WholeRandomShuffleComplexity", &::lz76WholeRandomShuffleComplexityWithArgs, "seq"_a, "args"_a)
      .def("lz76WholeRandomShuffleComplexity",
           &::lz76WholeRandomShuffleComplexity,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      //!> Information distance
      .def("lz76InformationDistance", &::lz76InformationDistanceWithoutArgs, "seq1"_a, "seq2"_a)
      .def("lz76InformationDistance", &::lz76InformationDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a)
      .def("lz76InformationDistance",
           &::lz76InformationDistance,
           "seq1"_a,
           "seq2"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Information distance
      .def("lz76RandomShuffleDistance", &::lz76RandomShuffleDistanceWithoutArgs, "seq1"_a, "seq2"_a)
      .def("lz76RandomShuffleDistance", &::lz76RandomShuffleDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a)
      .def("lz76RandomShuffleDistance",
           &::lz76RandomShuffleDistance,
           "seq1"_a,
           "seq2"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency());
}