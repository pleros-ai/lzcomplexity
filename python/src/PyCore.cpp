/**
 * @file PyLempelziv.cpp
 * @brief Python bindings for LZ76 complexity analysis functions.
 *
 * This file provides Python bindings for all Lempel-Ziv 76 complexity
 * analysis functions including factorization, entropy density, shuffle
 * complexity, and information distance measures.
 */

#include <tuple>

#include "inc/utils.hpp"
#include "lz/general.h"
#include "lz/lempelziv.h"
#include "lz/structures.h"

std::tuple<uint,                            // complexity
           double,                          // entropy density
           std::vector<uint>,               // factors
           std::tuple<int, double, double>  // shuffle complexity
           >
  lz76(const seq_type& seq,
       lz::lz_int      partitions = 1,
       lz::lz_uint     alphabet = lz::details::ALPHABET_SIZE,
       lz::lz_uint     log_base = lz::details::ALPHABET_SIZE,
       lz::lz_int      max_block_size = -1,
       lz::lz_uint     jobs = std::thread::hardware_concurrency()) {
  utl::LZ_Args args;
  args.chunks = partitions;
  args.alphabet = alphabet;
  args.log_base = log_base;
  args.block_size = max_block_size;
  lz::utils::LempelZiv cpx;

  utl::EnabledMT(jobs);
  std::visit(overload{[&](auto&& s) { cpx = lz::lz76(s, args); },
                      [&](std::vector<int> s) {
                        auto string_view
                          = s | std::views::transform([](int num) { return std::to_string(num); });
                        auto str = std::accumulate(
                          std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                        cpx = lz::lz76(str, args);
                      }},
             seq);
  utl::DisabledMT();

  auto shuffle_tuple = std::make_tuple(cpx.getRandomShuffleComplexity().max_block_size,
                                       cpx.getRandomShuffleComplexity().emc_value,
                                       cpx.getRandomShuffleComplexity().multi_information);
  return std::make_tuple(cpx.getComplexity(), cpx.getEntropyDensity(), cpx.getFactors(), shuffle_tuple);
}

lz::lz_uint factorization(const seq_type& seq,
                          lz::lz_int      partitions = 1,
                          lz::lz_uint     alphabet = lz::details::ALPHABET_SIZE,
                          lz::lz_uint     log_base = lz::details::ALPHABET_SIZE,
                          lz::lz_uint     jobs = std::thread::hardware_concurrency()) {
  utl::LZ_Args args;
  args.chunks = partitions;
  args.alphabet = alphabet;
  args.log_base = log_base;
  lz::lz_uint cpx;

  utl::EnabledMT(jobs);
  std::visit(overload{[&](auto&& s) { cpx = lz::lz76Factorization(s, args); },
                      [&](std::vector<int> s) {
                        auto string_view
                          = s | std::views::transform([](int num) { return std::to_string(num); });
                        auto str = std::accumulate(
                          std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                        cpx = lz::lz76Factorization(str, args);
                      }},
             seq);
  utl::DisabledMT();
  return cpx;
};

std::tuple<lz::lz_uint, std::vector<lz::lz_uint>> factors(const seq_type& seq,
                                                          lz::lz_int      partitions = 1,
                                                          lz::lz_uint alphabet = lz::details::ALPHABET_SIZE,
                                                          lz::lz_uint log_base = lz::details::ALPHABET_SIZE,
                                                          lz::lz_uint jobs
                                                          = std::thread::hardware_concurrency()) {
  utl::LZ_Args args;
  args.chunks = partitions;
  args.alphabet = alphabet;
  args.log_base = log_base;
  lz::internal::LZ_Result cpx;

  utl::EnabledMT(jobs);
  std::visit(overload{[&](auto&& s) { cpx = lz::lz76Factors(s, args); },
                      [&](std::vector<int> s) {
                        auto string_view
                          = s | std::views::transform([](int num) { return std::to_string(num); });
                        auto str = std::accumulate(
                          std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                        cpx = lz::lz76Factors(str, args);
                      }},
             seq);
  utl::DisabledMT();
  return std::make_tuple(cpx.factorization, cpx.lzf);
};

lz::lz_double entropy_density(const seq_type& seq,
                              lz::lz_int      partitions = 1,
                              lz::lz_uint     alphabet = lz::details::ALPHABET_SIZE,
                              lz::lz_uint     log_base = lz::details::ALPHABET_SIZE,
                              lz::lz_uint     jobs = std::thread::hardware_concurrency()) {
  utl::LZ_Args args;
  args.chunks = partitions;
  args.alphabet = alphabet;
  args.log_base = log_base;
  lz::lz_double cpx;

  utl::EnabledMT(jobs);
  std::visit(overload{[&](auto&& s) { cpx = lz::lz76EntropyDensity(s, args); },
                      [&](std::vector<int> s) {
                        auto string_view
                          = s | std::views::transform([](int num) { return std::to_string(num); });
                        auto str = std::accumulate(
                          std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                        cpx = lz::lz76EntropyDensity(str, args);
                      }},
             seq);
  utl::DisabledMT();
  return cpx;
};

std::tuple<int, double, double> emc(const seq_type& seq,
                                    lz::lz_int      partitions = 1,
                                    lz::lz_uint     alphabet = lz::details::ALPHABET_SIZE,
                                    lz::lz_uint     log_base = lz::details::ALPHABET_SIZE,
                                    lz::lz_int      max_block_size = -1,
                                    lz::lz_uint     jobs = std::thread::hardware_concurrency()) {
  utl::LZ_Args args;
  args.chunks = partitions;
  args.alphabet = alphabet;
  args.log_base = log_base;
  args.block_size = max_block_size;
  lz::utils::LZ_Shuffle cpx;

  utl::EnabledMT(jobs);
  std::visit(overload{[&](auto&& s) { cpx = lz::lz76RandomShuffleComplexity(s, args); },
                      [&](std::vector<int> s) {
                        auto string_view
                          = s | std::views::transform([](int num) { return std::to_string(num); });
                        auto str = std::accumulate(
                          std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                        cpx = lz::lz76RandomShuffleComplexity(str, args);
                      }},
             seq);
  utl::DisabledMT();

  return std::make_tuple(cpx.max_block_size, cpx.emc_value, cpx.multi_information);
}

void PyCore(py::module_& m) {
  using namespace nanobind::literals;

  m.def("lz76",
        lz76,
        "seq"_a,
        "partitions"_a = 1,
        "alphabet"_a = 2,
        "log_base"_a = 2,
        "max_block_size"_a = -1,
        "jobs"_a = std::thread::hardware_concurrency(),
        R"pbdoc(
     Performs comprehensive Lempel-Ziv (LZ76) analysis on an input sequence.
     Calculates complexity measures, entropy density, and effective complexity.

     Parameters:
     -----------
     seq : Union[sequence, str, List[char], List[int]]
         Input sequence to analyze. Can be a string, character list, or integer list.
     partitions : int, optional (default=1)
         Number of partitions for parallel suffix array computation. Higher values may improve
         performance on longer sequences.
     alphabet : int, optional (default=2)
         Size of the symbol alphabet. For binary sequences use 2, for DNA use 4, etc.
     log_base : int, optional (default=2)
         Base for logarithm calculations. Use 2 for bits, e for nats, 10 for decimal.
     max_block_size : int, optional (default=-1)
         Maximum block size for random shuffle complexity calculation.
         Use -1 for automatic size determination.
     jobs : int, optional (default=hardware_concurrency)
         Number of parallel threads to use. Defaults to available CPU cores.

     Returns:
     --------
     tuple
         Tuple containing:
         - complexity: LZ76 complexity measure
         - entropy: Entropy density
         - factors: LZ76 factorization
         - shuffle_complexity: Tuple containing (max_block_size, emc_value, multi_information)
     )pbdoc")
    //!> Function with all args
    .def("factorization",
         factorization,
         "seq"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(
    Computes the LZ76 complexity or the number of factors of the Lempel-Ziv 76 factorization of a sequence.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to factorize.
    partitions : int, optional (default=1)
        Number of partitions for parallel processing.
    alphabet : int, optional (default=2)
        Size of the symbol alphabet.
    log_base : int, optional (default=2)
        Base for logarithm calculations.
    jobs : int, optional (default=hardware_concurrency)
        Number of parallel threads to use.

    Returns:
    --------
    int
        The Lempel-Ziv 76 factorization.
           )pbdoc")
    //!> Factor function
    .def("factors",
         factors,
         "seq"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(
    Computes the Lempel-Ziv 76 factorization of a sequence.
    Factorization splits the sequence into minimal substrings that haven't appeared before.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to factorize.
    partitions : int, optional (default=1)
        Number of partitions for parallel processing.
    alphabet : int, optional (default=2)
        Size of the symbol alphabet.
    log_base : int, optional (default=2)
        Base for logarithm calculations.
    jobs : int, optional (default=hardware_concurrency)
        Number of parallel threads to use.

    Returns:
    --------
    tuple
        A tuple containing (complexity, list of factor positions).
           )pbdoc")
    //!> Entropy density
    .def("entropy_density",
         entropy_density,
         "seq"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(
    Calculates the entropy density of a sequence.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to factorize.
    partitions : int, optional (default=1)
        Number of partitions for parallel processing.
    alphabet : int, optional (default=2)
        Size of the symbol alphabet.
    log_base : int, optional (default=2)
        Base for logarithm calculations.
    jobs : int, optional (default=hardware_concurrency)
        Number of parallel threads to use.

    Returns:
    --------
    float
        The entropy density.
           )pbdoc")
    //!> Random shuffle complexity
    .def("emc",
         emc,
         "seq"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "max_block_size"_a = -1,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(
     Calculates effective complexity measure using random shuffle complexity algorithm.
     This method estimates the non-random information content by comparing original
     and randomly shuffled sequence complexities.

     Parameters:
     -----------
     seq : Union[sequence, str, List[char], List[int]]
         Input sequence to analyze.
     partitions : int, optional (default=1)
         Number of partitions for parallel processing.
     alphabet : int, optional (default=2)
         Size of the symbol alphabet.
     log_base : int, optional (default=2)
         Base for logarithm calculations.
     max_block_size : int, optional (default=-1)
         Maximum size of blocks for shuffling. -1 for automatic.
     jobs : int, optional (default=hardware_concurrency)
         Number of parallel threads to use.

     Returns:
     --------
     tuple
         A tuple containing (max block size, emc value, multi information).
     )pbdoc");
}
