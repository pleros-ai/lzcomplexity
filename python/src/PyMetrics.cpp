/**
 * @file PyLempelziv.cpp
 * @brief Python bindings for LZ76 complexity analysis functions.
 *
 * This file provides Python bindings for all Lempel-Ziv 76 complexity
 * analysis functions including factorization, entropy density, shuffle
 * complexity, and information distance measures.
 */

#include "inc/PyLempelZiv.hpp"

// Information distance
// Get function signature for the python wrapper
auto distance_seq = py::overload_cast<const lz::sequence&, const lz::sequence&>(&lz::lz76InformationDistance);
auto distance_seq_args
  = py::overload_cast<const lz::sequence&, const lz::sequence&, utl::LZ_Args&>(&lz::lz76InformationDistance);
// Information distance using variant type for sequence, vector<int>, vector<char> and string
auto lz76InformationDistanceWithoutArgs = generateFunctionWithoutArgsForDistance<lz::lz_double>(distance_seq);
auto lz76InformationDistanceWithArgs = generateFunctionWithArgsForDistance<lz::lz_double>(distance_seq_args);
// Information distance for python style function with flags
auto lz76InformationDistance = generateFunctionWithArgsAndFlagsForDistance<lz::lz_double>(distance_seq_args);

// Random shuffle distance
// Get function signature for the python wrapper
auto shuffle_distance_seq
  = py::overload_cast<const lz::sequence&, const lz::sequence&>(&lz::lz76RandomShuffleDistance);
auto shuffle_distance_seq_args = py::overload_cast<const lz::sequence&, const lz::sequence&, utl::LZ_Args&>(
  &lz::lz76RandomShuffleDistance);
// Random shuffle distance using variant type for vector<int>, vector<char> and string
auto lz76RandomShuffleDistanceWithoutArgs
  = generateFunctionWithoutArgsForDistance<lz::lz_double>(shuffle_distance_seq);
auto lz76RandomShuffleDistanceWithArgs
  = generateFunctionWithArgsForDistance<lz::lz_double>(shuffle_distance_seq_args);
// Random shuffle distance for python style function with flags
auto lz76RandomShuffleDistance
  = generateFunctionWithArgsAndFlagsForDistance<lz::lz_double>(shuffle_distance_seq_args);

void PyMetrics(py::module_& m) {
  using namespace nanobind::literals;

  //!> Information distance
  m.def("nid", lz76InformationDistanceWithoutArgs, "seq1"_a, "seq2"_a, R"pbdoc(
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
    .def("nid", lz76InformationDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a, R"pbdoc(
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
    .def("nid",
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
    .def("rid", lz76RandomShuffleDistanceWithoutArgs, "seq1"_a, "seq2"_a, R"pbdoc(
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
    .def("rid", lz76RandomShuffleDistanceWithArgs, "seq1"_a, "seq2"_a, "args"_a, R"pbdoc(
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
    .def("rid",
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
