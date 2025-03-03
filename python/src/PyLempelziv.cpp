#include "utils.hpp"

// Factorization
// Get function signature for the python wrapper
auto factorization_sequence      = py::overload_cast<const lz::sequence&>(&lz::lz76Factorization);
auto factorization_sequence_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76Factorization);
// Factorization using sequence class
auto lz76FactorizationSeqWithoutArgs = generateFunctionSequenceWithoutArgs<lz::lz_uint>(factorization_sequence);
auto lz76FactorizationSeqWithArgs    = generateFunctionSequenceWithArgs<lz::lz_uint>(factorization_sequence_args);
// Factorization using variant type for vector<int>, vector<char> and string
auto lz76FactorizationWithoutArgs = generateFunctionWithoutArgs<lz::lz_uint>(factorization_sequence);
auto lz76FactorizationWithArgs    = generateFunctionWithArgs<lz::lz_uint>(factorization_sequence_args);
// Factorization for python style function with flags
auto lz76FactorizationSeq = generateFunctionSequenceWithArgsAndFlags<lz::lz_uint>(factorization_sequence_args);
auto lz76Factorization    = generateFunctionWithArgsAndFlags<lz::lz_uint>(factorization_sequence_args);

// Factors
// Get function signature for the python wrapper
auto factors_sequence      = py::overload_cast<const lz::sequence&>(&lz::lz76Factors);
auto factors_sequence_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76Factors);
// Factors using sequence class
auto lz76FactorsSeqWithoutArgs = generateFunctionSequenceWithoutArgs<lz::lz_uint>(factors_sequence);
auto lz76FactorsSeqWithArgs    = generateFunctionSequenceWithArgs<lz::lz_uint>(factors_sequence_args);
// Factors using variant type for vector<int>, vector<char> and string
auto lz76FactorsWithoutArgs = generateFunctionWithoutArgs<lz::internal::LZ_Result>(factors_sequence);
auto lz76FactorsWithArgs    = generateFunctionWithArgs<lz::internal::LZ_Result>(factors_sequence_args);
// Factors for python style function with flags
auto lz76FactorsSeq = generateFunctionSequenceWithArgsAndFlags<lz::internal::LZ_Result>(factors_sequence_args);
auto lz76Factors    = generateFunctionWithArgsAndFlags<lz::internal::LZ_Result>(factors_sequence_args);

// Entropy density
// Get function signature for the python wrapper
auto entropy_density_sequence      = py::overload_cast<const lz::sequence&>(&lz::lz76EntropyDensity);
auto entropy_density_sequence_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76EntropyDensity);
// Entropy density using sequence class
auto lz76EntropyDensitySeqWithoutArgs = generateFunctionSequenceWithoutArgs<lz::lz_double>(entropy_density_sequence);
auto lz76EntropyDensitySeqWithArgs    = generateFunctionSequenceWithArgs<lz::lz_double>(entropy_density_sequence_args);
// Entropy density using variant type for vector<int>, vector<char> and string
auto lz76EntropyDensityWithoutArgs = generateFunctionWithoutArgs<lz::lz_double>(entropy_density_sequence);
auto lz76EntropyDensityWithArgs    = generateFunctionWithArgs<lz::lz_double>(entropy_density_sequence_args);
// Entropy density for python style function with flags
auto lz76EntropyDensitySeq = generateFunctionSequenceWithArgsAndFlags<lz::lz_double>(entropy_density_sequence_args);
auto lz76EntropyDensity    = generateFunctionWithArgsAndFlags<lz::lz_double>(entropy_density_sequence_args);

// Paired Shuffle Complexity
// Get function signature for the python wrapper
auto paired_shuffle_seq      = py::overload_cast<const lz::sequence&>(&lz::lz76PairedShuffleComplexity);
auto paired_shuffle_seq_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76PairedShuffleComplexity);
// Paired shuffle using sequence class
auto lz76PairedShuffleSeqWithoutArgs = generateFunctionSequenceWithoutArgs<utl::LZ_Shuffle>(paired_shuffle_seq);
auto lz76PairedShuffleSeqWithArgs    = generateFunctionSequenceWithArgs<utl::LZ_Shuffle>(paired_shuffle_seq_args);
// Paired shuffle using variant type for vector<int>, vector<char> and string
auto lz76PairedShuffleWithoutArgs = generateFunctionWithoutArgs<utl::LZ_Shuffle>(paired_shuffle_seq);
auto lz76PairedShuffleWithArgs    = generateFunctionWithArgs<utl::LZ_Shuffle>(paired_shuffle_seq_args);
// Paired shuffle for python style function with flags
auto lz76PairedShuffleSeq =
   generateFunctionSequenceWithArgsAndFlagsForShuffle<utl::LZ_Shuffle>(paired_shuffle_seq_args);
auto lz76PairedShuffle = generateFunctionWithArgsAndFlagsForShuffle<utl::LZ_Shuffle>(paired_shuffle_seq_args);

// Randoms shuffle complexity
// Get function signature for the python wrapper
auto random_shuffle_seq      = py::overload_cast<const lz::sequence&>(&lz::lz76RandomShuffleComplexity);
auto random_shuffle_seq_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76RandomShuffleComplexity);
// Random shuffle using sequence class
auto lz76RandomShuffleSeqWithoutArgs = generateFunctionSequenceWithoutArgs<utl::LZ_Shuffle>(random_shuffle_seq);
auto lz76RandomShuffleSeqWithArgs    = generateFunctionSequenceWithArgs<utl::LZ_Shuffle>(random_shuffle_seq_args);
// Random shuffle using variant type for vector<int>, vector<char> and string
auto lz76RandomShuffleWithoutArgs = generateFunctionWithoutArgs<utl::LZ_Shuffle>(random_shuffle_seq);
auto lz76RandomShuffleWithArgs    = generateFunctionWithArgs<utl::LZ_Shuffle>(random_shuffle_seq_args);
// Random shuffle for python style function with flags
auto lz76RandomShuffleSeq =
   generateFunctionSequenceWithArgsAndFlagsForShuffle<utl::LZ_Shuffle>(random_shuffle_seq_args);
auto lz76RandomShuffle = generateFunctionWithArgsAndFlagsForShuffle<utl::LZ_Shuffle>(random_shuffle_seq_args);

// Information distance
// Get function signature for the python wrapper
auto distance_seq = py::overload_cast<const lz::sequence&, const lz::sequence&>(&lz::lz76InformationDistance);
auto distance_seq_args =
   py::overload_cast<const lz::sequence&, const lz::sequence&, utl::LZ_Args>(&lz::lz76InformationDistance);
// Information distance using sequence class
auto lz76InformationDistanceSeqWithoutArgs =
   generateFunctionSequenceWithoutArgsForDistance<lz::lz_double>(distance_seq);
auto lz76InformationDistanceSeqWithArgs = generateFunctionSequenceWithArgsForDistance<lz::lz_double>(distance_seq_args);
// Information distance using variant type for vector<int>, vector<char> and string
auto lz76InformationDistanceWithoutArgs = generateFunctionWithoutArgsForDistance<lz::lz_double>(distance_seq);
auto lz76InformationDistanceWithArgs    = generateFunctionWithArgsForDistance<lz::lz_double>(distance_seq_args);
// Information distance for python style function with flags
auto lz76InformationDistanceSeq = generateFunctionSequenceWithArgsAndFlagsForDistance<lz::lz_double>(distance_seq_args);
auto lz76InformationDistance    = generateFunctionWithArgsAndFlagsForDistance<lz::lz_double>(distance_seq_args);

// Random shuffle distance
// Get function signature for the python wrapper
auto shuffle_distance_seq = py::overload_cast<const lz::sequence&, const lz::sequence&>(&lz::lz76RandomShuffleDistance);
auto shuffle_distance_seq_args =
   py::overload_cast<const lz::sequence&, const lz::sequence&, utl::LZ_Args>(&lz::lz76RandomShuffleDistance);
// Random shuffle distance using sequence class
auto lz76RandomShuffleDistanceSeqWithoutArgs =
   generateFunctionSequenceWithoutArgsForDistance<lz::lz_double>(shuffle_distance_seq);
auto lz76RandomShuffleDistanceSeqWithArgs =
   generateFunctionSequenceWithArgsForDistance<lz::lz_double>(shuffle_distance_seq_args);
// Random shuffle distance using variant type for vector<int>, vector<char> and string
auto lz76RandomShuffleDistanceWithoutArgs = generateFunctionWithoutArgsForDistance<lz::lz_double>(shuffle_distance_seq);
auto lz76RandomShuffleDistanceWithArgs = generateFunctionWithArgsForDistance<lz::lz_double>(shuffle_distance_seq_args);
// Random shuffle distance for python style function with flags
auto lz76RandomShuffleDistanceSeq =
   generateFunctionSequenceWithArgsAndFlagsForDistance<lz::lz_double>(shuffle_distance_seq_args);
auto lz76RandomShuffleDistance = generateFunctionWithArgsAndFlagsForDistance<lz::lz_double>(shuffle_distance_seq_args);

// Calculate all
auto lz76_seq_args = py::overload_cast<const lz::sequence&, utl::LZ_Args>(&lz::lz76);
// Compute all using sequence class
auto lz76SeqWithArgs = generateFunctionSequenceWithArgs<utl::LempelZiv>(lz76_seq_args);
// Compute all using variant type for vector<int>, vector<char> and string
auto lz76WithArgs = generateFunctionWithArgs<utl::LempelZiv>(lz76_seq_args);
// Compute all for python style function with flags
auto lz76Seq = generateFunctionSequenceWithArgsAndFlagsForShuffle<utl::LempelZiv>(lz76_seq_args);
auto lz76    = generateFunctionWithArgsAndFlagsForShuffle<utl::LempelZiv>(lz76_seq_args);

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
           "_random_shuffle_complexity"_a,
           "_paired_shuffle_complexity"_a,
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
      .def_property_readonly("paired_shuffle_complexity", &utl::LempelZiv::getPairedShuffleComplexity)
      .def_property_readonly("random_shuffle_complexity", &utl::LempelZiv::getRandomShuffleComplexity)
      .def_property_readonly("lz_normal_error", &utl::LempelZiv::getNormalError)
      .def_property_readonly("lz_poison_error", &utl::LempelZiv::getPoisonError)
      .def_property_readonly("extras", &utl::LempelZiv::getExtras)
      .def_property_readonly("factors", &utl::LempelZiv::getFactors)
      .def_property_readonly("factors_stddev", &utl::LempelZiv::getFactorsStddev);

   m.def("lz76", lz76SeqWithArgs, "seq"_a, "args"_a, R"pbdoc(
    The lz76 function

    Parameters
    ----------
    seq : sequence
        The sequence to be analyzed.
    args : LZ_Args
        The arguments object with parameters for the analysis.

    Returns
    -------
    LempelZiv
        The Lempel-Ziv complexity.
)pbdoc")
      .def("lz76", lz76WithArgs, "seq"_a, "args"_a, R"pbdoc(
    The lz76 function

    Parameters
    ----------
    seq : Union[str, List[char], List[int]]
        The sequence to be analyzed.
    args : LZ_Args
        The arguments object with parameters for the analysis.

    Returns
    -------
    LempelZiv
        The Lempel-Ziv complexity.
)pbdoc")
      .def("lz76",
           lz76Seq,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency(),
           R"pbdoc(
           The lz76 function.

           Parameters
           ----------
           seq : sequence
               The sequence to be analyzed.
           partitions : int
               The number of partitions used for the parallel suffix array algorithm.
           alphabet : int
               Size of the alphabet.
           log_base : int
               base for the logarithm.
           max_block_size : int
               Max length for the block in random shuffle complexity.
           jobs : int
               Number of threads.

           Returns
           -------
           LempelZiv
               The Lempel-Ziv complexity.
           )pbdoc")
      .def("lz76",
           lz76,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency(),
           R"pbdoc(
           The lz76 function.

           Parameters
           ----------
           seq : Union[str, List[char], List[int]]
               The sequence to be analyzed.
           partitions : int
               The number of partitions used for the parallel suffix array algorithm.
           alphabet : int
               Size of the alphabet.
           log_base : int
               base for the logarithm.
           max_block_size : int
               Max length for the block in random shuffle complexity.
           jobs : int
               Number of threads.

           Returns
           -------
           LempelZiv
               The Lempel-Ziv complexity.
           )pbdoc")
      //!> LZ76 factorization
      .def("lz76Factorization", lz76FactorizationSeqWithoutArgs, "seq"_a)
      .def("lz76Factorization", lz76FactorizationWithoutArgs, "seq"_a)
      .def("lz76Factorization", lz76FactorizationSeqWithArgs, "seq"_a, "args"_a)
      .def("lz76Factorization", lz76FactorizationWithArgs, "seq"_a, "args"_a)
      //!> Function with all args
      .def("lz76Factorization",
           lz76FactorizationSeq,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      .def("lz76Factorization",
           lz76Factorization,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Factor function
      .def("lz76Factors", lz76FactorsSeqWithoutArgs, "seq"_a)
      .def("lz76Factors", lz76FactorsWithoutArgs, "seq"_a)
      .def("lz76Factors", lz76FactorsSeqWithArgs, "seq"_a, "args"_a)
      .def("lz76Factors", lz76FactorsWithArgs, "seq"_a, "args"_a)
      .def("lz76Factors",
           lz76FactorsSeq,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      .def("lz76Factors",
           lz76Factors,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Entropy density
      .def("lz76EntropyDensity", lz76EntropyDensitySeqWithoutArgs, "seq"_a)
      .def("lz76EntropyDensity", lz76EntropyDensityWithoutArgs, "seq"_a)
      .def("lz76EntropyDensity", lz76EntropyDensitySeqWithArgs, "seq"_a, "args"_a)
      .def("lz76EntropyDensity", lz76EntropyDensityWithArgs, "seq"_a, "args"_a)
      .def("lz76EntropyDensity",
           lz76EntropyDensitySeq,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      .def("lz76EntropyDensity",
           lz76EntropyDensity,
           "seq"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Random shuffle complexity
      .def("lz76PairedShuffleComplexity", lz76PairedShuffleSeqWithoutArgs, "seq"_a)
      .def("lz76PairedShuffleComplexity", lz76PairedShuffleWithoutArgs, "seq"_a)
      .def("lz76PairedShuffleComplexity", lz76PairedShuffleSeqWithArgs, "seq"_a, "args"_a)
      .def("lz76PairedShuffleComplexity", lz76PairedShuffleWithArgs, "seq"_a, "args"_a)
      .def("lz76PairedShuffleComplexity",
           lz76PairedShuffleSeq,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      .def("lz76PairedShuffleComplexity",
           lz76PairedShuffle,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      .def("lz76RandomShuffleComplexity", lz76RandomShuffleSeqWithoutArgs, "seq"_a)
      .def("lz76RandomShuffleComplexity", lz76RandomShuffleWithoutArgs, "seq"_a)
      .def("lz76RandomShuffleComplexity", lz76RandomShuffleSeqWithArgs, "seq"_a, "args"_a)
      .def("lz76RandomShuffleComplexity", lz76RandomShuffleWithArgs, "seq"_a, "args"_a)
      .def("lz76RandomShuffleComplexity",
           lz76RandomShuffleSeq,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      .def("lz76RandomShuffleComplexity",
           lz76RandomShuffle,
           "seq"_a,
           "partitions"_a     = 1,
           "alphabet"_a       = 2,
           "log_base"_a       = 2,
           "max_block_size"_a = -1,
           "jobs"_a           = std::thread::hardware_concurrency())
      //!> Information distance
      .def("lz76InformationDistance", lz76InformationDistanceSeqWithoutArgs, "seq1"_a, "seq2"_a)
      .def("lz76InformationDistance", lz76InformationDistanceWithoutArgs, "seq1"_a, "seq2"_a)
      .def("lz76InformationDistance", lz76InformationDistanceSeqWithArgs, "seq1"_a, "seq2"_a, "args"_a)
      .def("lz76InformationDistance", lz76InformationDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a)
      .def("lz76InformationDistance",
           lz76InformationDistanceSeq,
           "seq1"_a,
           "seq2"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      .def("lz76InformationDistance",
           lz76InformationDistance,
           "seq1"_a,
           "seq2"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      //!> Information distance
      .def("lz76RandomShuffleDistance", lz76RandomShuffleDistanceSeqWithoutArgs, "seq1"_a, "seq2"_a)
      .def("lz76RandomShuffleDistance", lz76RandomShuffleDistanceWithoutArgs, "seq1"_a, "seq2"_a)
      .def("lz76RandomShuffleDistance", lz76RandomShuffleDistanceSeqWithArgs, "seq1"_a, "seq2"_a, "args"_a)
      .def("lz76RandomShuffleDistance", lz76RandomShuffleDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a)
      .def("lz76RandomShuffleDistance",
           lz76RandomShuffleDistanceSeq,
           "seq1"_a,
           "seq2"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency())
      .def("lz76RandomShuffleDistance",
           lz76RandomShuffleDistance,
           "seq1"_a,
           "seq2"_a,
           "partitions"_a = 1,
           "alphabet"_a   = 2,
           "log_base"_a   = 2,
           "jobs"_a       = std::thread::hardware_concurrency());
}
