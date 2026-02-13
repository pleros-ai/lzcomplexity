/**
 * @file flags.hpp
 * @brief Input/output structures for batch LZ76 complexity analysis.
 *
 * This header defines the core data structures used for processing multiple
 * sequences through the LZ76 analysis pipeline. It provides containers for
 * input configuration (LZ_Flags) and output results (LZ_Output).
 *
 * @see lzApp.hpp for the functions that use these structures.
 */

#pragma once

#include <lz/sequence.h>
#include <lz/structures.h>
#include <lz/types.h>

#include <vector>

namespace lz {
  namespace utils {

    /**
     * @brief Container for storing results from LZ76 complexity analysis.
     *
     * This structure holds all computed results from various LZ76 analysis
     * functions. It stores per-sequence results (complexity, entropy density,
     * shuffle complexity) as well as inter-sequence metrics (information
     * distance, mutual information).
     *
     * @note Results are stored in vectors indexed by sequence position.
     *       Use the setter methods to safely populate results with automatic
     *       capacity management.
     */
    struct LZ_Output {
      std::vector<LempelZiv> data;  ///< Per-sequence LZ76 analysis results.

      std::vector<lz_bool>
        calculated_complexity;  ///< Flags indicating if complexity was computed for each sequence.

      std::vector<lz_uint> half_complexity;  ///< LZ76 complexity of the first half of each sequence.
      std::vector<lz_double>
        lz_effective_complexity;  ///< Effective complexity (excess entropy via mutual information).
      std::vector<lz_double> excess_entropy_dist;  ///< Excess entropy estimated via distance method.
      std::vector<lz_double> mutual_information;   ///< Mutual information between sequence halves.
      std::vector<lz_double> info_distance;        ///< Information distance between consecutive sequences.

      std::vector<lz_double>
        random_shuffle_distance;  ///< Shuffle-based information distance between consecutive sequences.
      std::vector<lz_double> sequence_info_distance;  ///< Information distance computed within each sequence.

      std::vector<LZ_Extra>
        extra;  ///< Additional measures (Rajski distance, uncertainties, redundancy, etc.).

      std::vector<lz_double> mixed_entropy_density;  ///< Mixed entropy density of consecutive sequence pairs.

      /**
       * @brief Default constructor.
       */
      LZ_Output() = default;

      /**
       * @brief Constructs an output container with pre-allocated capacity.
       * @param size Number of sequences to allocate space for.
       */
      LZ_Output(lz_size size) {
        data = std::vector<LempelZiv>(size);
        calculated_complexity = std::vector<bool>(size, false);
      };

      /**
       * @brief Copy constructor.
       * @param lz Source output container to copy from.
       */
      LZ_Output(const LZ_Output& lz) = default;

      /**
       * @brief Move constructor.
       * @param lz Source output container to move from.
       */
      LZ_Output(LZ_Output&& lz) = default;

      /**
       * @brief Sets the LZ76 complexity for a specific sequence.
       * @param index Sequence index (0-based).
       * @param complexity The computed LZ76 complexity value.
       */
      auto setComplexity(lz_size index, lz_uint complexity) -> void;

      /**
       * @brief Sets the entropy density for a specific sequence.
       * @param index Sequence index (0-based).
       * @param entropy The computed normalized entropy density.
       */
      auto setEntropyDensity(lz_size index, lz_double entropy) -> void;

      /**
       * @brief Sets the LZ76 factorization positions for a specific sequence.
       * @param index Sequence index (0-based).
       * @param factors Vector of factor boundary positions.
       */
      auto setFactors(lz_size index, std::vector<lz_uint> factors) -> void;

      /**
       * @brief Sets the paired shuffle complexity for a specific sequence.
       * @param index Sequence index (0-based).
       * @param shuffle The computed paired shuffle complexity result.
       */
      auto setPairedShuffleComplexity(lz_size index, LZ_Shuffle shuffle) -> void;

      /**
       * @brief Sets the random shuffle complexity for a specific sequence.
       * @param index Sequence index (0-based).
       * @param shuffle The computed random shuffle complexity result.
       */
      auto setRandomShuffleComplexity(lz_size index, LZ_Shuffle shuffle) -> void;

      /**
       * @brief Sets the epsilon (average factor length) for a specific sequence.
       * @param index Sequence index (0-based).
       * @param epsilon The computed average factor length.
       */
      auto setEpsilon(lz_size index, lz_double epsilon) -> void;

      /**
       * @brief Sets the standard deviation of factor sizes for a specific sequence.
       * @param index Sequence index (0-based).
       * @param stddev The computed standard deviation.
       */
      auto setFactorsStddev(lz_size index, lz_double stddev) -> void;

      /**
       * @brief Sets the normal distribution error estimate for a specific sequence.
       * @param index Sequence index (0-based).
       * @param error The computed error assuming normal distribution of factor sizes.
       */
      auto setNormalError(lz_size index, lz_double error) -> void;

      /**
       * @brief Sets the Poisson distribution error estimate for a specific sequence.
       * @param index Sequence index (0-based).
       * @param error The computed error assuming Poisson distribution of factor sizes.
       */
      auto setPoisonError(lz_size index, lz_double error) -> void;

      /**
       * @brief Sets additional measures for a specific sequence.
       * @param index Sequence index (0-based).
       * @param extras Structure containing Rajski distance, uncertainties, redundancy, etc.
       */
      auto setExtras(lz_size index, LZ_Extra extras) -> void;

      /**
       * @brief Ensures the internal vectors have sufficient capacity.
       * @param size Required minimum capacity.
       *
       * Automatically resizes internal vectors if the requested size
       * exceeds current capacity.
       */
      auto checkCapacity(lz_size size) -> void;

      /**
       * @brief Copy assignment operator (by value, uses copy-and-swap idiom).
       * @param rhs Source output container.
       * @return Reference to this object.
       */
      LZ_Output& operator=(LZ_Output rhs) {
        std::swap(this->lz_effective_complexity, rhs.lz_effective_complexity);
        std::swap(this->excess_entropy_dist, rhs.excess_entropy_dist);
        std::swap(this->info_distance, rhs.info_distance);
        std::swap(this->sequence_info_distance, rhs.sequence_info_distance);
        std::swap(this->data, rhs.data);

        std::swap(this->mixed_entropy_density, rhs.mixed_entropy_density);

        return *this;
      };

      /**
       * @brief Copy assignment operator (by reference).
       * @param rhs Source output container.
       * @return Reference to this object.
       */
      LZ_Output& operator=(LZ_Output& rhs) {
        if (this != &rhs) {
          this->~LZ_Output();
          new (this) LZ_Output(rhs);
        }
        return *this;
      };
    };

    /**
     * @brief Input configuration for batch LZ76 complexity analysis.
     *
     * This structure encapsulates all input data and configuration parameters
     * needed for LZ76 analysis. It holds the sequences to be analyzed along
     * with algorithm parameters such as suffix array configuration and
     * shuffle entropy calculation range.
     *
     * @note Use this structure with the functions declared in lzApp.hpp
     *       to perform batch analysis on multiple sequences.
     *
     * @see LZ_Output for storing the analysis results.
     * @see LZ_Args for algorithm configuration parameters.
     */
    struct LZ_Flags {
      lz_int shuffle_init_line = details::UNDEFINED_LINES;  ///< Starting line index for shuffle entropy
                                                            ///< deficit calculation (-1 = undefined).
      lz_int shuffle_end_line = details::UNDEFINED_LINES;   ///< Ending line index for shuffle entropy deficit
                                                            ///< calculation (-1 = undefined).
      LZ_Args sa_args;  ///< Configuration parameters for suffix array construction and core algorithms.

      std::vector<sequence> input;  ///< Collection of input sequences to analyze.

      /**
       * @brief Constructs flags from a string with algorithm arguments.
       * @param text Input string to convert to a sequence.
       * @param _sa_args Algorithm configuration parameters.
       */
      LZ_Flags(std::string text, LZ_Args _sa_args)
        : sa_args(_sa_args), input({text}) {};

      /**
       * @brief Constructs flags from a single sequence with algorithm arguments.
       * @param text Input sequence to analyze.
       * @param _sa_args Algorithm configuration parameters.
       */
      LZ_Flags(sequence text, LZ_Args _sa_args)
        : sa_args(_sa_args), input({text}) {};

      /**
       * @brief Constructs flags from multiple sequences with algorithm arguments.
       * @param data Vector of input sequences to analyze.
       * @param _sa_args Algorithm configuration parameters.
       */
      LZ_Flags(std::vector<sequence> data, LZ_Args _sa_args)
        : sa_args(_sa_args), input(data) {};

      /**
       * @brief Copy constructor.
       * @param flags Source flags to copy from.
       */
      LZ_Flags(const LZ_Flags& flags)
        : sa_args(flags.sa_args), input(flags.input) {};

      /**
       * @brief Move constructor.
       * @param flags Source flags to move from.
       */
      LZ_Flags(LZ_Flags&& flags)
        : sa_args(std::move(flags.sa_args)), input(std::move(flags.input)) {};

      /**
       * @brief Destructor.
       */
      ~LZ_Flags() {};

      /**
       * @brief Appends a single string as a new sequence to the input collection.
       * @param data String to add as a sequence.
       */
      auto addData(std::string data) -> void { input.emplace_back(data); };

      /**
       * @brief Appends multiple strings as new sequences to the input collection.
       * @param data Vector of strings to add as sequences.
       */
      auto addData(std::vector<std::string> data) -> void {
#ifdef __cpp_lib_ranges
        std::ranges::for_each(data, [&](auto&& elem) { input.emplace_back(elem); });
#else
        std::for_each(data.begin(), data.end(), [&](auto&& elem) { input.emplace_back(elem); });
#endif
      };

      /**
       * @brief Copy assignment operator (by value, uses copy-and-swap idiom).
       * @param rhs Source flags to copy.
       * @return Reference to this object.
       */
      LZ_Flags& operator=(LZ_Flags rhs) {
        std::swap(this->input, rhs.input);
        std::swap(this->sa_args, rhs.sa_args);
        std::swap(this->shuffle_init_line, rhs.shuffle_init_line);
        std::swap(this->shuffle_end_line, rhs.shuffle_end_line);

        return *this;
      };

      /**
       * @brief Copy assignment operator (by reference).
       * @param rhs Source flags to copy.
       * @return Reference to this object.
       */
      LZ_Flags& operator=(LZ_Flags& rhs) {
        if (this != &rhs) {
          this->~LZ_Flags();
          new (this) LZ_Flags(rhs);
        }
        return *this;
      };

      /**
       * @brief Equality comparison operator.
       * @param lhs Left-hand side flags.
       * @param rhs Right-hand side flags.
       * @return True if input sequences and arguments are equal.
       */
      friend bool operator==(const LZ_Flags& lhs, const LZ_Flags& rhs) {
        return lhs.input == rhs.input && lhs.sa_args == rhs.sa_args;
      };

      /**
       * @brief Inequality comparison operator.
       * @param lhs Left-hand side flags.
       * @param rhs Right-hand side flags.
       * @return True if flags are not equal.
       */
      friend bool operator!=(const LZ_Flags& lhs, const LZ_Flags& rhs) { return !operator==(lhs, rhs); };
    };

  }  // namespace utils
}  // namespace lz
