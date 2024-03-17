#pragma once

#include <lz/sa_structures.h>
#include <lz/utils.h>

#include <vector>

namespace lz {
   namespace utils {
      struct LZ_ExcessInfo {
         lz_int max_block_size;                   //? The value used in excess of entropy by shuffling
         lz_double excess_value;                  //? The excess of entropy value
         lz_double multi_information;             //? The multi information value
         std::vector<lz_double> excess_by_terms;  //? The vector of excess fo entropy for each term (size == mm_value)
      };

      struct LZ_Args : public SA_Args {
         const static lz_int ALL_LINES = -2;
         const static lz_int UNDEFINED_LINES = -1;

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
         LZ_Args(const LZ_Args& sa) = default;
         LZ_Args(LZ_Args&& sa) = default;

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
   }  // namespace utils
}  // namespace lz