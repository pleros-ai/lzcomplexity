#pragma once

// Local includes
#include "general.h"
#include "lzexceptions.h"

namespace lz {
   namespace utils {

      const static char newline_char    = '\n';
      const std::string SBIN_WHITESPACE = " \n\r\t";

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

      inline lz_double log(auto num, auto base = ALPHABET_SIZE) {
         return std::log(num) / std::log(base);
      };

#ifdef __cpp_lib_ranges
      constexpr inline bool is_space(char q) noexcept {
         const auto ws = {' ', '\t', '\n', '\v', '\r', '\f'};
         return std::ranges::any_of(ws, [q](auto p) { return p == q; });
      };
#endif
      // .............................................................................
      // Name: string_trim_left
      //
      // Synopsis: trim left white spaces
      //
      // Parameters:
      //			const std::string& s                   -----> operand
      //
      // Returns:
      //         std::string   -----> The trimmed string
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline std::string string_trim_left(const std::string& s) {
         size_t startpos = (s.size() > 0) ? s.find_first_not_of(SBIN_WHITESPACE) : std::string::npos;
         return (startpos == std::string::npos) ? "" : s.substr(startpos);
      }

      // .............................................................................
      // Name: string_trim_right
      //
      // Synopsis: trim right white spaces
      //
      // Parameters:
      //			const std::string& s                   -----> operand
      //
      // Returns:
      //         std::string   -----> The trimmed string
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline std::string string_trim_right(const std::string& s) {
         size_t endpos = (s.size() > 0) ? s.find_last_not_of(SBIN_WHITESPACE) : std::string::npos;
         return (endpos == std::string::npos) ? "" : s.substr(0, endpos + 1);
      }

      // .............................................................................
      // Name: string_trim
      //
      // Synopsis: trim starting and ending white spaces
      //
      // Parameters:
      //			const std::string& s                   -----> operand
      //
      // Returns:
      //         std::string   -----> The trimmed string
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline std::string string_trim(const std::string& s) {
#ifdef __cpp_lib_ranges
         auto view = s | std::views::drop_while(is_space) | std::views::reverse | std::views::drop_while(is_space) |
                     std::views::reverse;
         return {view.begin(), view.end()};
#else
         return string_trim_right(string_trim_left(s));
#endif
      }

      inline std::string to_lowercase(std::string& s) {
#ifdef __cpp_lib_ranges
         std::ranges::transform(s, s.begin(), [](unsigned char c) { return std::tolower(c); });
         return s;
#else
         std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
         return s;
#endif
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
         lz_size      M     = 10;
         lz_size      Mold  = 1;
         unsigned int count = 0;

         //? The next iteration is solving the non linear equation N=M 2^M for M. This estimate is
         //? termed naive and given in Melchert and Hartmann PRE 91, 023306 (2015).
         //? Perhaps not the most efficient way to solve the NLE but enough for our purposes
         //? Note that a binary logarithm is taken regardless of the alphabet cardinality

         while (count < 100) {
            M = std::lround(std::log(((double)size) / ((double)Mold)) / std::log(2));
            count++;
            if (M == Mold)
               break;

            Mold = M;
         }

         return M;
      }

      // Returns pointer to a memory-allocation for `size` elements of type `T_`.
      template<typename T>
      inline T* allocate(unsigned int size) {
         return static_cast<T*>(std::malloc(size * sizeof(T)));
      }

      constexpr uint32_t hash(std::string_view data) noexcept {
         uint32_t hash = 5381;

         for (const char c: data)
            hash = ((hash << 5) + hash) + (unsigned char)c;

         return hash;
      }
   }  // namespace utils
}  // namespace lz
