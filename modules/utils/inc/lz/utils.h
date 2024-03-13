#pragma once
// Local includes
#include "general.h"
#include "lzexceptions.h"

namespace lz {
   namespace utils {

      enum MSG_TYPE { ERROR, WARRING, INFO };
      /** @deprecated */
      enum INPUT_FLAG { text, path };

      inline void cmp_arrays(const int* arr1, const uint64_t* arr2, lz_size n) {
         int diff = 0;

         for (lz_size i = 0; i < n; i++) {
            diff += arr1[i] - arr2[i];
         }

         std::cout << "\n==> Difference: " << diff << std::endl;
      }

      inline void cmp_arrays(const uint64_t* arr1, const int* arr2, lz_size n) {
         int diff = 0;

         for (lz_size i = 0; i < n; i++) {
            diff += arr1[i] - arr2[i];
         }

         std::cout << "\n==> Difference: " << diff << std::endl;
      }

      //*****************************************************
      //*               auxiliary functions
      //*****************************************************
      /** @brief
       *  Given an integer size it calculates the maximum value for any entropy iteration
       *  calculation before fluctuations becomes a pain in the neck
       *  @param size
       *  @return std::vector<char>::lz_sizeype
       */
      inline lz_size max_block_size(lz_size size) {
         lz_size M = 10;
         lz_size Mold = 1;
         unsigned int count = 0;

         //? The next iteration is solving the non linear equation N=M 2^M for M. This estimate is
         //? termed naive and given in Melchert and Hartmann PRE 91, 023306 (2015).
         //? Perhaps not the most efficient way to solve the NLE but enough for our purposes
         //? Note that a binary logarithm is taken regardless of the alphabet cardinality

         while (count < 100) {
            M = std::lround(std::log(((double)size) / ((double)Mold)) / std::log(2));
            count++;
            if (M == Mold) break;

            Mold = M;
         }

         return M;
      }

      // Returns pointer to a memory-allocation for `size` elements of type `T_`.
      template <typename T>
      inline T* allocate(unsigned int size) {
         return static_cast<T*>(std::malloc(size * sizeof(T)));
      }
   }  // namespace utils
}  // namespace lz
