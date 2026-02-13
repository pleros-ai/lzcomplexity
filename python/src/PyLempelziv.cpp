/**
 * @file PyLempelziv.cpp
 * @brief Python bindings for LZ76 complexity analysis functions.
 *
 * This file provides Python bindings for all Lempel-Ziv 76 complexity
 * analysis functions including factorization, entropy density, shuffle
 * complexity, and information distance measures.
 */

#include "inc/PyLempelZiv.hpp"

// Factorization
// Get function signature for the python wrapper
auto factorization_sequence = py::overload_cast<const lz::sequence&>(&lz::lz76Factorization);
auto factorization_sequence_args
  = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76Factorization);
// Factorization using variant type for sequence, vector<int>, vector<char> and string
auto lz76FactorizationWithoutArgs = generateFunctionWithoutArgs<lz::lz_uint>(factorization_sequence);
auto lz76FactorizationWithArgs = generateFunctionWithArgs<lz::lz_uint>(factorization_sequence_args);
// Factorization for python style function with flags
auto lz76Factorization = generateFunctionWithArgsAndFlags<lz::lz_uint>(factorization_sequence_args);

// Factors
// Get function signature for the python wrapper
auto factors_sequence = py::overload_cast<const lz::sequence&>(&lz::lz76Factors);
auto factors_sequence_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76Factors);
// Factors using variant type for sequence, vector<int>, vector<char> and string
auto lz76FactorsWithoutArgs = generateFunctionWithoutArgs<lz::internal::LZ_Result>(factors_sequence);
auto lz76FactorsWithArgs = generateFunctionWithArgs<lz::internal::LZ_Result>(factors_sequence_args);
// Factors for python style function with flags
auto lz76Factors = generateFunctionWithArgsAndFlags<lz::internal::LZ_Result>(factors_sequence_args);

// Entropy density
// Get function signature for the python wrapper
auto entropy_density_sequence = py::overload_cast<const lz::sequence&>(&lz::lz76EntropyDensity);
auto entropy_density_sequence_args
  = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76EntropyDensity);
// Entropy density using variant type for sequence, vector<int>, vector<char> and string
auto lz76EntropyDensityWithoutArgs = generateFunctionWithoutArgs<lz::lz_double>(entropy_density_sequence);
auto lz76EntropyDensityWithArgs = generateFunctionWithArgs<lz::lz_double>(entropy_density_sequence_args);
// Entropy density for python style function with flags
auto lz76EntropyDensitySeq
  = generateFunctionSequenceWithArgsAndFlags<lz::lz_double>(entropy_density_sequence_args);
auto lz76EntropyDensity = generateFunctionWithArgsAndFlags<lz::lz_double>(entropy_density_sequence_args);

// Paired Shuffle Complexity
// Get function signature for the python wrapper
auto paired_shuffle_seq = py::overload_cast<const lz::sequence&>(&lz::lz76PairedShuffleComplexity);
auto paired_shuffle_seq_args
  = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76PairedShuffleComplexity);
// Paired shuffle using variant type for sequence,vector<int>, vector<char> and string
auto lz76PairedShuffleWithoutArgs = generateFunctionWithoutArgs<utl::LZ_Shuffle>(paired_shuffle_seq);
auto lz76PairedShuffleWithArgs = generateFunctionWithArgs<utl::LZ_Shuffle>(paired_shuffle_seq_args);
// Paired shuffle for python style function with flags
auto lz76PairedShuffle = generateFunctionWithArgsAndFlagsForShuffle<utl::LZ_Shuffle>(paired_shuffle_seq_args);

// Randoms shuffle complexity
// Get function signature for the python wrapper
auto random_shuffle_seq = py::overload_cast<const lz::sequence&>(&lz::lz76RandomShuffleComplexity);
auto random_shuffle_seq_args
  = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76RandomShuffleComplexity);
// Random shuffle using variant type for sequence, vector<int>, vector<char> and string
auto lz76RandomShuffleWithoutArgs = generateFunctionWithoutArgs<utl::LZ_Shuffle>(random_shuffle_seq);
auto lz76RandomShuffleWithArgs = generateFunctionWithArgs<utl::LZ_Shuffle>(random_shuffle_seq_args);
// Random shuffle for python style function with flags
auto lz76RandomShuffle = generateFunctionWithArgsAndFlagsForShuffle<utl::LZ_Shuffle>(random_shuffle_seq_args);

// Information distance
// Get function signature for the python wrapper
auto distance_seq = py::overload_cast<const lz::sequence&, const lz::sequence&>(&lz::lz76InformationDistance);
auto distance_seq_args
  = py::overload_cast<const lz::sequence&, const lz::sequence&, utl::LZ_Args>(&lz::lz76InformationDistance);
// Information distance using variant type for sequence, vector<int>, vector<char> and string
auto lz76InformationDistanceWithoutArgs = generateFunctionWithoutArgsForDistance<lz::lz_double>(distance_seq);
auto lz76InformationDistanceWithArgs = generateFunctionWithArgsForDistance<lz::lz_double>(distance_seq_args);
// Information distance for python style function with flags
auto lz76InformationDistance = generateFunctionWithArgsAndFlagsForDistance<lz::lz_double>(distance_seq_args);

// Random shuffle distance
// Get function signature for the python wrapper
auto shuffle_distance_seq
  = py::overload_cast<const lz::sequence&, const lz::sequence&>(&lz::lz76RandomShuffleDistance);
auto shuffle_distance_seq_args
  = py::overload_cast<const lz::sequence&, const lz::sequence&, utl::LZ_Args>(&lz::lz76RandomShuffleDistance);
// Random shuffle distance using variant type for vector<int>, vector<char> and string
auto lz76RandomShuffleDistanceWithoutArgs
  = generateFunctionWithoutArgsForDistance<lz::lz_double>(shuffle_distance_seq);
auto lz76RandomShuffleDistanceWithArgs
  = generateFunctionWithArgsForDistance<lz::lz_double>(shuffle_distance_seq_args);
// Random shuffle distance for python style function with flags
auto lz76RandomShuffleDistance
  = generateFunctionWithArgsAndFlagsForDistance<lz::lz_double>(shuffle_distance_seq_args);

// Calculate all
auto lz76_seq_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76);
// Compute all using variant type for sequence, vector<int>, vector<char> and string
auto lz76WithArgs = generateFunctionWithArgs<utl::LempelZiv>(lz76_seq_args);
// Compute all for python style function with flags
auto lz76 = generateFunctionWithArgsAndFlagsForShuffle<utl::LempelZiv>(lz76_seq_args);

void PyLempelZiv(py::module_& m) {
  using namespace nanobind::literals;

  // =========================================================================
  // LZ_Result class - Raw factorization results
  // =========================================================================
  py::class_<lz::internal::LZ_Result> LZ_Result(m, "LZ_Result", R"pbdoc(
Raw results from LZ76 factorization.

This class contains the raw factorization data including the complexity
value and the positions of factor boundaries.

Attributes
----------
factorization : int
    The LZ76 complexity (number of factors).
lzf : List[int]
    List of factor boundary positions in the sequence.

Examples
--------
>>> import lzcomplexity as lz
>>> result = lz.lz76Factors("ABRACADABRA")
>>> print(f"Complexity: {result.factorization}")
>>> print(f"Factor positions: {result.lzf}")
)pbdoc");

  LZ_Result.def(py::init(), "Create an empty LZ_Result object.")
    .def("__copy__", [](const lz::internal::LZ_Result& self) { return lz::internal::LZ_Result(self); })
    .def("__repr__",
         [](const lz::internal::LZ_Result& self) {
           return "LZ_Result(factorization=" + std::to_string(self.factorization) + ")";
         })
    .def(
      "__deepcopy__",
      [](const lz::internal::LZ_Result& self, py::dict) { return lz::internal::LZ_Result(self); },
      "memo"_a)
    .def_rw("factorization",
            &lz::internal::LZ_Result::factorization,
            "int: The LZ76 complexity (number of factors in the factorization).")
    .def_rw("lzf", &lz::internal::LZ_Result::lzf, "List[int]: Factor boundary positions in the sequence.");

  // =========================================================================
  // LempelZiv class - Complete analysis results
  // =========================================================================
  py::class_<utl::LempelZiv> LempelZiv(m, "LempelZiv", R"pbdoc(
Complete results from LZ76 complexity analysis.

This class contains all computed measures from a comprehensive LZ76 analysis,
including complexity, entropy density, shuffle complexity, and additional
information-theoretic measures.

Attributes
----------
complexity : int
    The LZ76 complexity (number of factors in the factorization).
entropy : float
    Normalized entropy density. Estimated as c(S) * log_k(n) / n,
    where c(S) is complexity, k is alphabet size, n is sequence length.
factors : List[int]
    Factor boundary positions in the sequence.
factors_stddev : float
    Standard deviation of factor lengths.
random_shuffle_complexity : LZ_Shuffle
    Effective complexity computed via random block shuffling.
paired_shuffle_complexity : LZ_Shuffle
    Effective complexity computed via paired shuffling method.
lz_normal_error : float
    Error estimate assuming normal distribution of factor sizes.
lz_poison_error : float
    Error estimate assuming Poisson distribution of factor sizes.
extras : LZ_Extra
    Additional measures (Rajski distance, redundancy, uncertainties).

Examples
--------
>>> import lzcomplexity as lz
>>> result = lz.lz76("ABRACADABRA")
>>> print(f"Complexity: {result.complexity}")
>>> print(f"Entropy density: {result.entropy}")
>>> print(f"Effective complexity: {result.random_shuffle_complexity.excess_value}")

Notes
-----
The LempelZiv object is returned by the lz76() function which performs
a complete analysis. For individual measures, use the specific functions
like lz76Factorization(), lz76EntropyDensity(), etc.
)pbdoc");

  LempelZiv.def(py::init(), "Create an empty LempelZiv object.")
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
         "_random_shuffle_complexity"_a,
         "_paired_shuffle_complexity"_a,
         "_lz_normal_errors"_a,
         "_lz_poison_errors"_a,
         "_eps"_a,
         "_factors_stddev"_a,
         "_extra"_a,
         "Create a LempelZiv object with all analysis results.")
    .def("__copy__", [](const utl::LempelZiv& self) { return utl::LempelZiv(self); })
    .def("__repr__",
         [](const utl::LempelZiv& self) {
           return "LempelZiv(complexity=" + std::to_string(self.getComplexity())
             + ", entropy=" + std::to_string(self.getEntropyDensity()) + ")";
         })
    .def("__deepcopy__", [](const utl::LempelZiv& self, py::dict) { return utl::LempelZiv(self); }, "memo"_a);

  // Properties with detailed docstrings
  LempelZiv
    .def_prop_ro("complexity",
                 &utl::LempelZiv::getComplexity,
                 "int: LZ76 complexity (number of factors in the factorization).")
    .def_prop_ro("entropy",
                 &utl::LempelZiv::getEntropyDensity,
                 "float: Normalized entropy density estimate h ≈ c(S) * log_k(n) / n.")
    .def_prop_ro("paired_shuffle_complexity",
                 &utl::LempelZiv::getPairedShuffleComplexity,
                 "LZ_Shuffle: Effective complexity via paired shuffling method.")
    .def_prop_ro("random_shuffle_complexity",
                 &utl::LempelZiv::getRandomShuffleComplexity,
                 "LZ_Shuffle: Effective complexity via random block shuffling.")
    .def_prop_ro("lz_normal_error",
                 &utl::LempelZiv::getNormalError,
                 "float: Error estimate assuming normal distribution of factor sizes.")
    .def_prop_ro("lz_poison_error",
                 &utl::LempelZiv::getPoisonError,
                 "float: Error estimate assuming Poisson distribution of factor sizes.")
    .def_prop_ro("extras",
                 &utl::LempelZiv::getExtras,
                 "LZ_Extra: Additional measures (Rajski distance, redundancy, etc.).")
    .def_prop_ro(
      "factors", &utl::LempelZiv::getFactors, "List[int]: Factor boundary positions in the sequence.")
    .def_prop_ro(
      "factors_stddev", &utl::LempelZiv::getFactorsStddev, "float: Standard deviation of factor lengths.");

  m.def("lz76", lz76WithArgs, "seq"_a, "args"_a, R"pbdoc(
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
     LempelZiv
         Object containing analysis results:
         - complexity: LZ76 complexity measure
         - entropy: Entropy density
         - factors: LZ76 factorization
         - paired_shuffle_complexity: Effective complexity using paired shuffling
         - random_shuffle_complexity: Effective complexity using random shuffling
         - extras: Additional metrics (Rajski distance, redundancy, uncertainties)
     )pbdoc")
    .def("lz76",
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
     LempelZiv
         Object containing analysis results:
         - complexity: LZ76 complexity measure
         - entropy: Entropy density
         - factors: LZ76 factorization
         - paired_shuffle_complexity: Effective complexity using paired shuffling
         - random_shuffle_complexity: Effective complexity using random shuffling
     )pbdoc")
    //!> LZ76 factorization
    .def("lz76Factorization", lz76FactorizationWithoutArgs, "seq"_a, R"pbdoc(
    Computes the LZ76 complexity or the number of factors of the Lempel-Ziv 76 factorization of a sequence.

     Parameters:
     -----------
     seq : Union[sequence, str, List[char], List[int]]
         Input sequence to factorize.

     Returns:
     --------
     int
         The Lempel-Ziv 76 factorization.
     )pbdoc")
    .def("lz76Factorization", lz76FactorizationWithArgs, "seq"_a, "args"_a, R"pbdoc(
     Computes the LZ76 complexity or the number of factors of the Lempel-Ziv 76 factorization of a sequence.

     Parameters:
     -----------
     seq : Union[sequence, str, List[char], List[int]]
         Input sequence to factorize.

     Returns:
     --------
     int
         The Lempel-Ziv 76 factorization.
     )pbdoc")
    //!> Function with all args
    .def("lz76Factorization",
         lz76Factorization,
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
    .def("lz76Factors", lz76FactorsWithoutArgs, "seq"_a, R"pbdoc(
    Computes the Lempel-Ziv 76 factorization of a sequence.
    Factorization splits the sequence into minimal substrings that haven't appeared before.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to analyze.
    
    Returns:
    --------
    List[int]
        Indices where each new factor begins in the sequence.
      )pbdoc")
    .def("lz76Factors", lz76FactorsWithArgs, "seq"_a, "args"_a, R"pbdoc(
    Computes the Lempel-Ziv 76 factorization of a sequence.
    Factorization splits the sequence into minimal substrings that haven't appeared before.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to analyze.
    args : LZ_Args
        Object with parameters for execute the analysis.
    
    Returns:
    --------
    List[int]
       Indices where each new factor begins in the sequence.
      )pbdoc")
    .def("lz76Factors",
         lz76Factors,
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
    List[int]
        Indices where each new factor begins in the sequence.
           )pbdoc")
    //!> Entropy density
    .def("lz76EntropyDensity", lz76EntropyDensityWithoutArgs, "seq"_a, R"pbdoc(
    Calculates the entropy density of a sequence.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to analyze.
    
    Returns:
    --------
    float
        The entropy density.
      )pbdoc")
    .def("lz76EntropyDensity", lz76EntropyDensityWithArgs, "seq"_a, "args"_a, R"pbdoc(
    Calculates the entropy density of a sequence.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to analyze.
    args : LZ_Args
        Object with parameters for execute the analysis.
    
    Returns:
    --------
    float
        The entropy density.
      )pbdoc")
    .def("lz76EntropyDensity",
         lz76EntropyDensity,
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
    .def("lz76PairedShuffleComplexity", lz76PairedShuffleWithoutArgs, "seq"_a, R"pbdoc(
    Calculates the effective complexity measure of a sequence using paired shuffle complexity algorithm.
    The function apply the random shuffle complexity method to the sequence produced by mixing the two halves of the original sequence.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
        Input sequence to analyze.
    
    Returns:
    --------
    LZ_Shuffle
        Object containing:
        - excess_value: The excess complexity measures estimation
        - multi_information: Multi-information using block size 1
        - summands: Individual complexity contributions
        - max_block_size: Block size used for analysis
      )pbdoc")
    .def("lz76PairedShuffleComplexity", lz76PairedShuffleWithArgs, "seq"_a, "args"_a, R"pbdoc(
    Calculates the effective complexity measure of a sequence using paired shuffle complexity algorithm.
    The function apply the random shuffle complexity method to the sequence produced by mixing the two halves of the original sequence.

    Parameters:
    -----------
    seq : sequence object
        Input sequence to analyze.
    args : LZ_Args
        Object with parameters for execute the analysis.
    
    Returns:
    --------
    LZ_Shuffle
        Object containing:
        - excess_value: The excess complexity measures estimation
        - multi_information: Multi-information using block size 1
        - summands: Individual complexity contributions
        - max_block_size: Block size used for analysis
      )pbdoc")
    .def("lz76PairedShuffleComplexity",
         lz76PairedShuffle,
         "seq"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "max_block_size"_a = -1,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(
    Calculates the effective complexity measure of a sequence using paired shuffle complexity algorithm.
    The function apply the random shuffle complexity method to the sequence produced by mixing the two halves of the original sequence.

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
    LZ_Shuffle
        Object containing:
        - excess_value: The excess complexity measures estimation
        - multi_information: Multi-information using block size 1
        - summands: Individual complexity contributions
        - max_block_size: Block size used for analysis
           )pbdoc")
    .def("lz76RandomShuffleComplexity", lz76RandomShuffleWithoutArgs, "seq"_a, R"pbdoc(
    Calculates the effective complexity measure of a sequence using random shuffle complexity algorithm.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
         Input sequence to analyze.
    
    Returns:
    --------
    LZ_Shuffle
        Object containing:
        - excess_value: The excess complexity measures estimation
        - multi_information: Multi-information using block size 1
        - summands: Individual complexity contributions
        - max_block_size: Block size used for analysis
      )pbdoc")
    .def("lz76RandomShuffleComplexity", lz76RandomShuffleWithArgs, "seq"_a, "args"_a, R"pbdoc(
    Calculates the effective complexity measure of a sequence using random shuffle complexity algorithm.

    Parameters:
    -----------
    seq : Union[sequence, str, List[char], List[int]]
         Input sequence to analyze.
    args : LZ_Args
        Object with parameters for execute the analysis.
    
    Returns:
    --------
    LZ_Shuffle
        Object containing:
        - excess_value: The excess complexity measures estimation
        - multi_information: Multi-information using block size 1
        - summands: Individual complexity contributions
        - max_block_size: Block size used for analysis
      )pbdoc")
    .def("lz76RandomShuffleComplexity",
         lz76RandomShuffle,
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
     LZ_Shuffle
         Object containing:
         - excess_value: The excess complexity measures estimation
         - multi_information: Multi-information using block size 1
         - summands: Individual complexity contributions
         - max_block_size: Block size used for analysis
     )pbdoc")
    //!> Information distance
    .def("lz76InformationDistance", lz76InformationDistanceWithoutArgs, "seq1"_a, "seq2"_a, R"pbdoc(
    Estimates the information distance between two sequences using LZ76 complexity.
    This metric measures how different two sequences are in terms of their information content.

    Parameters:
    -----------
    seq1 : Union[sequence, str, List[char], List[int]]
        First input sequence.
    seq2 : Union[sequence, str, List[char], List[int]]
        Second input sequence.
    
    Returns:
    --------
    float
        The information distance.
      )pbdoc")
    .def("lz76InformationDistance", lz76InformationDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a, R"pbdoc(
    Calculates the estimation of information distance between two sequences using lz76 complexity.

    Parameters:
    -----------
    seq1 : Union[sequence, str, List[char], List[int]]
        First input sequence.
    seq2 : Union[sequence, str, List[char], List[int]]
        Second input sequence.
    args : LZ_Args
        Object with parameters for execute the analysis.
    
    Returns:
    --------
    float
        The information distance.
      )pbdoc")
    .def("lz76InformationDistance",
         lz76InformationDistance,
         "seq1"_a,
         "seq2"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(    
    Estimates the information distance between two sequences using LZ76 complexity.
    This metric measures how different two sequences are in terms of their information content.

    Parameters:
    -----------
    seq1 : Union[sequence, str, List[char], List[int]]
        First input sequence.
    seq2 : Union[sequence, str, List[char], List[int]]
        Second input sequence.
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
        The information distance.
           )pbdoc")
    //!> Information distance
    .def("lz76RandomShuffleDistance", lz76RandomShuffleDistanceWithoutArgs, "seq1"_a, "seq2"_a, R"pbdoc(
    Estimate the information distance between two sequences 
    using random shuffle complexity as estimation of mutual information.

    Parameters:
    -----------
    seq1 : Union[sequence, str, List[char], List[int]]
        First input sequence.
    seq2 : Union[sequence, str, List[char], List[int]]
        Second input sequence.
    
    Returns:
    --------
    float
        The information distance.
      )pbdoc")
    .def(
      "lz76RandomShuffleDistance", lz76RandomShuffleDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a, R"pbdoc(
    Calculates the estimation of information distance between two sequences 
    using random shuffle complexity as estimation of mutual information.

    Parameters:
    -----------
    seq1 : Union[sequence, str, List[char], List[int]]
        First input sequence.
    seq2 : Union[sequence, str, List[char], List[int]]
        Second input sequence.
    args : LZ_Args
        Object with parameters for execute the analysis.
    
    Returns:
    --------
    float
        The information distance.
      )pbdoc")
    .def("lz76RandomShuffleDistance",
         lz76RandomShuffleDistance,
         "seq1"_a,
         "seq2"_a,
         "partitions"_a = 1,
         "alphabet"_a = 2,
         "log_base"_a = 2,
         "jobs"_a = std::thread::hardware_concurrency(),
         R"pbdoc(
    Calculates the estimation of information distance between two sequences 
    using random shuffle complexity as estimation of mutual information.

    Parameters:
    -----------
    seq1 : Union[sequence, str, List[char], List[int]]
        First input sequence.
    seq2 : Union[sequence, str, List[char], List[int]]
        Second input sequence.
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
        The information distance.
           )pbdoc");
}
