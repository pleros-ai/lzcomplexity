#pragma once

#include <lz/sa_structures.h>
#include <lz/types.h>
#include <lz/utils.h>

namespace lz {
   namespace utils {
      struct LZ_Shuffle {
         lz_int                 max_block_size;     //? The value used in excess of entropy by shuffling
         lz_double              excess_value;       //? The excess of entropy value
         lz_double              multi_information;  //? The multi information value
         std::vector<lz_double> excess_by_terms;    //? The vector of excess fo entropy
                                                    //? for each term (size == mm_value)
      };

      struct LZ_Args : public SA_Args {
         const static lz_int ALL_LINES       = -2;
         const static lz_int UNDEFINED_LINES = -1;

         lz_bool calculate_lz = true;  //?> Flag for calculate the factorization of all sequence or not

         /* Excess entropy by shuffling parameters */
         lz_int block_size = -1;             //?> Max length of the block for excess of
                                             //?  entropy by shuffle (not calculate by default).
         lz_bool get_shuffle_terms = false;  //?> If true, the shuffle entropy deficit
                                             //?  will return the excess of entropy by terms.
         lz_uint alphabet = ALPHABET_SIZE;   //?> Size of the alphabet
         lz_uint log_base = ALPHABET_SIZE;   //?> base of the logarithm

         LZ_Args() = default;
         LZ_Args(lz_int chunks)
           : SA_Args(chunks){};
         LZ_Args(lz_int chunks, lz_int max_context)
           : SA_Args(chunks, max_context){};
         LZ_Args(lz_int chunks, lz_int max_context, lz_int block_size)
           : SA_Args(chunks, max_context), block_size(block_size){};
         LZ_Args(lz_int chunks, lz_int max_context, lz_int block_size, lz_uint alphabet)
           : SA_Args(chunks, max_context), block_size(block_size), alphabet(alphabet), log_base(alphabet){};
         LZ_Args(const LZ_Args& sa) = default;
         LZ_Args(LZ_Args&& sa)      = default;

         LZ_Args& operator=(LZ_Args rhs) {
            std::swap(this->chunks, rhs.chunks);
            std::swap(this->max_context, rhs.max_context);
            std::swap(this->block_size, rhs.block_size);
            std::swap(this->get_shuffle_terms, rhs.get_shuffle_terms);
            std::swap(this->alphabet, rhs.alphabet);
            std::swap(this->log_base, rhs.log_base);
            return *this;
         };

         friend bool operator==(LZ_Args sa1, LZ_Args sa2) {
            return sa1.chunks == sa2.chunks && sa1.max_context == sa2.max_context && sa1.block_size == sa2.block_size &&
                   sa1.get_shuffle_terms == sa2.get_shuffle_terms && sa1.alphabet == sa2.alphabet;
         };
         friend bool operator!=(LZ_Args sa1, LZ_Args sa2) { return !(sa1 == sa2); };

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

      class LempelZiv {
         friend LZ_Output;

     protected:
         lz_uint   complexity;       //!> complexity of the sequence
         lz_double entropy_density;  //!> entropy density of the sequence
         // lz_double lz_effective_complexity;             //!> excess of entropy by mutual information of the sequence
         // lz_double excess_entropy_dist;                 //!> excess of entropy by distance of the sequence
         LZ_Shuffle whole_random_shuffle_complexity;  //!> excess of entropy by shuffling of the sequence
         LZ_Shuffle random_shuffle_complexity;        //!> excess of entropy by shuffling
                                                      //!> of the merged sequence
         // lz_double mutual_information;                  //!> mutual information of two half of the sequences
         // lz_double info_distance;                       //!> information distance of the two consecutive sequences
         // lz_double sequence_info_distance;              //!> information distance of each sequences

         lz_double lz_normal_errors;
         lz_double lz_poison_errors;

         lz_double            epsilon;         //!> epsilon value
         lz_double            factors_stddev;  //!> standard deviation of factors length
         std::vector<lz_uint> factors;         //!> set of factors (each element is the initial index of the factor)

         LZ_Extra extra;

     public:
         LempelZiv() = default;
         LempelZiv(lz_uint              _complexity,
                   std::vector<lz_uint> lzf,
                   lz_double            _entropy_density,
                   LZ_Shuffle           _whole_random_shuffle_complexity,
                   LZ_Shuffle           _random_shuffle_complexity,
                   lz_double            _lz_normal_errors,
                   lz_double            _lz_poison_errors,
                   lz_double            eps,
                   lz_double            f_stddev,
                   LZ_Extra             _extra)
           : complexity{_complexity}
           , entropy_density{_entropy_density}
           , whole_random_shuffle_complexity{_whole_random_shuffle_complexity}
           , random_shuffle_complexity{_random_shuffle_complexity}
           , lz_normal_errors{_lz_normal_errors}
           , lz_poison_errors{_lz_poison_errors}
           , epsilon{eps}
           , factors_stddev{f_stddev}
           , factors{lzf}
           , extra{_extra} {};
         // Copy constructor
         LempelZiv(const LempelZiv& r)
           : complexity{r.complexity}
           , entropy_density{r.entropy_density}
           , whole_random_shuffle_complexity{r.whole_random_shuffle_complexity}
           , random_shuffle_complexity{r.random_shuffle_complexity}
           , lz_normal_errors{r.lz_normal_errors}
           , lz_poison_errors{r.lz_poison_errors}
           , epsilon{r.epsilon}
           , factors_stddev{r.factors_stddev}
           , factors{r.factors}
           , extra{r.extra} {};
         // Move constructor
         LempelZiv(LempelZiv&& r)
           : complexity{r.complexity}
           , entropy_density{r.entropy_density}
           , whole_random_shuffle_complexity{r.whole_random_shuffle_complexity}
           , random_shuffle_complexity{r.random_shuffle_complexity}
           , lz_normal_errors{r.lz_normal_errors}
           , lz_poison_errors{r.lz_poison_errors}
           , epsilon{r.epsilon}
           , factors_stddev{r.factors_stddev}
           , factors{std::move(r.factors)}
           , extra{r.extra} {};

         ~LempelZiv() {}

         LempelZiv& operator=(LempelZiv& lz)  = default;
         LempelZiv& operator=(LempelZiv&& lz) = default;
         // LempelZiv& operator=(const LempelZiv&);

         /// @brief
         /// logical equality operator. Compares factorizations, not complexities.
         /// @param lhs one of the source
         /// @param rhs another source
         /// @return *this
         friend bool operator==(const LempelZiv& lhs, const LempelZiv& rhs) { return lhs.factors == rhs.factors; };

         /// @brief
         /// logical non-equality operator. Compares factorizations, not complexities.
         /// @param lhs one of the source
         /// @param rhs another source
         /// @return *this
         friend bool operator!=(const LempelZiv& lhs, const LempelZiv& rhs) { return !(lhs == rhs); };

         // Gets
         auto getComplexity(void) const { return complexity; };
         auto getEntropyDensity(void) const { return entropy_density; };
         auto getRandomShuffleComplexity(void) const { return random_shuffle_complexity; };
         auto getWholeRandomShuffleComplexity(void) const { return whole_random_shuffle_complexity; };
         // auto getInformationDistance(void) const { return info_distance; };
         auto getNormalError(void) const { return lz_normal_errors; };
         auto getPoisonError(void) const { return lz_poison_errors; };
         auto getExtras(void) const { return extra; };
         auto getFactorsStddev(void) const { return factors_stddev; };
         auto getFactors(void) const { return factors; };
      };

   }  // namespace utils
}  // namespace lz