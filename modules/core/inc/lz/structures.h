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
         /* Excess entropy by shuffling parameters */
         lz_int block_size = 0;    //?> Max length of the block for excess of
                                   //?  entropy by shuffle.
         lz_int excess_line = -1;  //?> Line where get excess entropy by terms
                                   //?  (valid for excess of entropy by shuffling).

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
            std::swap(this->excess_line, rhs.excess_line);
            return *this;
         };

         friend bool operator==(LZ_Args sa1, LZ_Args sa2) {
            return sa1.chunks == sa2.chunks && sa1.max_context == sa2.max_context && sa1.block_size == sa2.block_size &&
                   sa1.excess_line == sa2.excess_line;
         };
         friend bool operator!=(LZ_Args sa1, LZ_Args sa2) { return !(sa1 == sa2); };

         friend void swap(LZ_Args& sa1, LZ_Args& sa2) {
            std::swap(sa1.chunks, sa2.chunks);
            std::swap(sa1.max_context, sa2.max_context);
            std::swap(sa1.block_size, sa2.block_size);
            std::swap(sa1.excess_line, sa2.excess_line);
         };
      };
   }  // namespace utils
}  // namespace lz