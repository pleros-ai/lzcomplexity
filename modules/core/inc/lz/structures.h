#pragma once

#include <lz/sa_structures.h>
#include <lz/types.h>
#include <lz/utils.h>

/**
 * @file structures.h
 * @brief Core data structures for Lempel-Ziv complexity analysis.
 */
namespace lz {
   namespace utils {

      /**
       * @brief Results from shuffle-based entropy excess computation.
       *
       * Stores the outcome of computing excess entropy by comparing the complexity
       * of the original sequence against shuffled versions.
       */
      struct LZ_Shuffle {
         lz_int max_block_size = -1;  ///< Maximum block size used for shuffling (-1 if not computed).
         lz_double excess_value = 0;  ///< The computed excess entropy value.
         lz_double multi_information = 0;  ///< The multi-information (total correlation) value.
         std::vector<lz_double> summands;  ///< Per-term excess entropy values for detailed analysis.
      };

      /**
       * @brief Configuration parameters for Lempel-Ziv complexity computation.
       *
       * Extends SA_Args with additional parameters specific to LZ factorization
       * and entropy calculations.
       */
      struct LZ_Args : public SA_Args {
         static const lz_int ALL_LINES = -2;        ///< Sentinel value indicating all lines should be processed.
         static const lz_int UNDEFINED_LINES = -1;  ///< Sentinel value for undefined line count.

         lz_bool calculate_lz = true;  ///< If true, compute the full LZ factorization.

         /* Excess entropy by shuffling parameters */
         lz_int block_size = -1;  ///< Block size for shuffle-based entropy computation (-1 disables).
         lz_bool get_shuffle_terms = false;  ///< If true, return per-term entropy values in results.
         lz_uint alphabet = NO_ALPHABET;  ///< Alphabet size for entropy normalization.
         lz_uint log_base = NO_ALPHABET;  ///< Logarithm base for entropy computation.

         /**
          * @brief Default constructor.
          */
         LZ_Args() = default;

         /**
          * @brief Constructs with specified chunk count for parallel processing.
          * @param chunks Number of chunks for parallelization.
          */
         LZ_Args(lz_int chunks)
           : SA_Args(chunks){};

         /**
          * @brief Constructs with chunk count and block size.
          * @param chunks Number of chunks for parallelization.
          * @param block_size Block size for shuffle entropy computation.
          */
         LZ_Args(lz_int chunks, lz_int block_size)
           : SA_Args(chunks), block_size(block_size){};

         /**
          * @brief Constructs with chunk count, block size, and alphabet size.
          * @param chunks Number of chunks for parallelization.
          * @param block_size Block size for shuffle entropy computation.
          * @param alphabet Alphabet size (also used as log base).
          */
         LZ_Args(lz_int chunks, lz_int block_size, lz_uint alphabet)
           : SA_Args(chunks), block_size(block_size), alphabet(alphabet), log_base(alphabet){};

         /**
          * @brief Constructs with all parameters specified.
          * @param chunks Number of chunks for parallelization.
          * @param block_size Block size for shuffle entropy computation.
          * @param alphabet Alphabet size.
          * @param base Logarithm base for entropy computation.
          */
         LZ_Args(lz_int chunks, lz_int block_size, lz_uint alphabet, lz_uint base)
           : SA_Args(chunks), block_size(block_size), alphabet(alphabet), log_base(base){};

         /**
          * @brief Copy constructor.
          */
         LZ_Args(const LZ_Args& sa) = default;

         /**
          * @brief Move constructor.
          */
         LZ_Args(LZ_Args&& sa) = default;

         /**
          * @brief Copy-and-swap assignment operator.
          * @param rhs The arguments to assign from.
          * @return Reference to this object.
          */
         LZ_Args& operator=(LZ_Args rhs) {
            std::swap(this->chunks, rhs.chunks);
            std::swap(this->max_context, rhs.max_context);
            std::swap(this->block_size, rhs.block_size);
            std::swap(this->get_shuffle_terms, rhs.get_shuffle_terms);
            std::swap(this->alphabet, rhs.alphabet);
            std::swap(this->log_base, rhs.log_base);
            return *this;
         };

         /**
          * @brief Equality comparison operator.
          * @return true if all parameters match.
          */
         friend bool operator==(LZ_Args sa1, LZ_Args sa2) {
            return sa1.chunks == sa2.chunks && sa1.max_context == sa2.max_context && sa1.block_size == sa2.block_size &&
                   sa1.get_shuffle_terms == sa2.get_shuffle_terms && sa1.alphabet == sa2.alphabet;
         };

         /**
          * @brief Inequality comparison operator.
          * @return true if any parameter differs.
          */
         friend bool operator!=(LZ_Args sa1, LZ_Args sa2) { return !(sa1 == sa2); };

         /**
          * @brief Swaps the contents of two LZ_Args objects.
          * @param sa1 First object.
          * @param sa2 Second object.
          */
         friend void swap(LZ_Args& sa1, LZ_Args& sa2) {
            std::swap(sa1.chunks, sa2.chunks);
            std::swap(sa1.max_context, sa2.max_context);
            std::swap(sa1.block_size, sa2.block_size);
            std::swap(sa1.get_shuffle_terms, sa2.get_shuffle_terms);
            std::swap(sa1.alphabet, sa2.alphabet);
            std::swap(sa1.log_base, sa2.log_base);
         };
      };

      struct LZ_Output;

      /**
       * @brief Stores the results of Lempel-Ziv complexity analysis.
       *
       * Contains the LZ factorization, complexity measures, entropy estimates,
       * and various derived quantities from the analysis of a sequence.
       */
      class LempelZiv {
         friend LZ_Output;

     protected:
         lz_uint complexity;        ///< LZ complexity (number of factors in the factorization).
         lz_double entropy_density; ///< Normalized entropy density estimate.

         LZ_Shuffle random_shuffle_complexity;  ///< Entropy excess from random shuffling.
         LZ_Shuffle paired_shuffle_complexity;  ///< Entropy excess from paired/merged sequence shuffling.

         lz_double lz_normal_errors;  ///< Error estimate assuming normal distribution.
         lz_double lz_poison_errors;  ///< Error estimate assuming Poisson distribution.

         lz_double epsilon;              ///< Epsilon parameter used in computation.
         lz_double factors_stddev;       ///< Standard deviation of factor lengths.
         std::vector<lz_uint> factors;   ///< Factor boundaries (each element is the starting index of a factor).

         LZ_Extra extra;  ///< Additional computed quantities.

     public:
         /**
          * @brief Default constructor.
          */
         LempelZiv() = default;

         /**
          * @brief Constructs a LempelZiv result with all computed values.
          * @param _complexity The LZ complexity value.
          * @param lzf Vector of factor starting positions.
          * @param _entropy_density The computed entropy density.
          * @param _random_shuffle_complexity Results from random shuffle analysis.
          * @param _paired_shuffle_complexity Results from paired shuffle analysis.
          * @param _lz_normal_errors Normal distribution error estimate.
          * @param _lz_poison_errors Poisson distribution error estimate.
          * @param eps Epsilon parameter.
          * @param f_stddev Standard deviation of factor lengths.
          * @param _extra Additional computed data.
          */
         LempelZiv(lz_uint              _complexity,
                   std::vector<lz_uint> lzf,
                   lz_double            _entropy_density,
                   LZ_Shuffle           _random_shuffle_complexity,
                   LZ_Shuffle           _paired_shuffle_complexity,
                   lz_double            _lz_normal_errors,
                   lz_double            _lz_poison_errors,
                   lz_double            eps,
                   lz_double            f_stddev,
                   LZ_Extra             _extra)
           : complexity{_complexity}
           , entropy_density{_entropy_density}
           , random_shuffle_complexity{_random_shuffle_complexity}
           , paired_shuffle_complexity{_paired_shuffle_complexity}
           , lz_normal_errors{_lz_normal_errors}
           , lz_poison_errors{_lz_poison_errors}
           , epsilon{eps}
           , factors_stddev{f_stddev}
           , factors{lzf}
           , extra{_extra} {};

         /**
          * @brief Copy constructor.
          * @param r The LempelZiv object to copy.
          */
         LempelZiv(const LempelZiv& r)
           : complexity{r.complexity}
           , entropy_density{r.entropy_density}
           , random_shuffle_complexity{r.random_shuffle_complexity}
           , paired_shuffle_complexity{r.paired_shuffle_complexity}
           , lz_normal_errors{r.lz_normal_errors}
           , lz_poison_errors{r.lz_poison_errors}
           , epsilon{r.epsilon}
           , factors_stddev{r.factors_stddev}
           , factors{r.factors}
           , extra{r.extra} {};

         /**
          * @brief Move constructor.
          * @param r The LempelZiv object to move from.
          */
         LempelZiv(LempelZiv&& r) { *this = std::move(r); };

         /**
          * @brief Destructor.
          */
         ~LempelZiv() {}

         /**
          * @brief Copy assignment operator.
          */
         LempelZiv& operator=(LempelZiv& lz) = default;

         /**
          * @brief Move assignment operator.
          */
         LempelZiv& operator=(LempelZiv&& lz) = default;

         /**
          * @brief Equality comparison based on factorization.
          *
          * Two LempelZiv results are equal if they have identical factor boundaries,
          * regardless of other computed values.
          *
          * @param lhs First LempelZiv object.
          * @param rhs Second LempelZiv object.
          * @return true if factorizations are identical.
          */
         friend bool operator==(const LempelZiv& lhs, const LempelZiv& rhs) { return lhs.factors == rhs.factors; };

         /**
          * @brief Inequality comparison based on factorization.
          * @param lhs First LempelZiv object.
          * @param rhs Second LempelZiv object.
          * @return true if factorizations differ.
          */
         friend bool operator!=(const LempelZiv& lhs, const LempelZiv& rhs) { return !(lhs == rhs); };

         /**
          * @brief Returns the LZ complexity (number of factors).
          * @return The complexity value.
          */
         auto getComplexity(void) const { return complexity; };

         /**
          * @brief Returns the normalized entropy density.
          * @return The entropy density value.
          */
         auto getEntropyDensity(void) const { return entropy_density; };

         /**
          * @brief Returns the paired shuffle complexity results.
          * @return The LZ_Shuffle structure with paired shuffle analysis.
          */
         auto getPairedShuffleComplexity(void) const { return paired_shuffle_complexity; };

         /**
          * @brief Returns the random shuffle complexity results.
          * @return The LZ_Shuffle structure with random shuffle analysis.
          */
         auto getRandomShuffleComplexity(void) const { return random_shuffle_complexity; };

         /**
          * @brief Returns the normal distribution error estimate.
          * @return The error value.
          */
         auto getNormalError(void) const { return lz_normal_errors; };

         /**
          * @brief Returns the Poisson distribution error estimate.
          * @return The error value.
          */
         auto getPoisonError(void) const { return lz_poison_errors; };

         /**
          * @brief Returns additional computed quantities.
          * @return The LZ_Extra structure.
          */
         auto getExtras(void) const { return extra; };

         /**
          * @brief Returns the standard deviation of factor lengths.
          * @return The standard deviation value.
          */
         auto getFactorsStddev(void) const { return factors_stddev; };

         /**
          * @brief Returns the factor boundaries.
          * @return Vector of starting indices for each factor.
          */
         auto getFactors(void) const { return factors; };
      };

   }  // namespace utils
}  // namespace lz
