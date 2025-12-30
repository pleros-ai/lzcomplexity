/***************************************************************************
                          caps.h  -  description
                             -------------------
    begin                : 16 Nov 2023
    email                : efrenaragon96@gmail.com
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2023 by Efren Aragon Perez * efrenaragon96@gmail.com
 **
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/********************************************************************************
 * Sources:                                                                     *
 *        1) Fast, Parallel, and Cache-Friendly Suffix Array Construction       *
 *           J. Khan, T. Rubel, L. Dhulipala, E. Molloy, R. Patro (2023)        *
 *                                                                              *
 * ******************************************************************************/

#pragma once

#include <lz/general.h>
#include <lz/parallel_utils.h>
#include <lz/utils.h>

#include <atomic>
#include <cassert>
#include <new>

#include "sa_structures.h"

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

// ============================================================================= //

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size  = 64;
#endif

/**
 * @file caps.h
 * @brief CaPS (Cache-friendly Parallel Suffix array) algorithm implementation.
 *
 * Implements a fast, parallel, and cache-friendly suffix array construction
 * algorithm based on the paper by Khan et al. (2023).
 */
namespace lz {
   namespace suffixarray {

      /**
       * @brief Cache-friendly Parallel Suffix array (CaPS) construction algorithm.
       *
       * Implements parallel suffix array and LCP array construction using a
       * divide-and-conquer approach optimized for cache efficiency. The algorithm
       * divides the problem into p subproblems, sorts them independently in parallel,
       * then merges using pivot-based partitioning.
       *
       * Key features:
       * - Parallel construction using OneTBB
       * - Cache-friendly memory access patterns
       * - Configurable subproblem count for load balancing
       * - Optional AVX2 optimization for LCP computation
       *
       * @note Preferred over SAIS for large inputs on multi-core systems.
       */
      class CaPS_SA {
     protected:
         std::vector<char> T_;       ///< The input text.
         lz_int n_;                  ///< Length of the input text.
         std::vector<lz_int> SA_;    ///< The computed suffix array.
         std::vector<lz_int> LCP_;   ///< The computed LCP array.
         lz_int* SA_w;               ///< Working space for SA construction.
         lz_int* LCP_w;              ///< Working space for LCP construction.

         lz_int p_;                  ///< Number of subproblems for parallel construction.
         lz_int max_context;         ///< Maximum prefix length for suffix comparison.
         lz_int* pivot_;             ///< Pivot values for partitioning.
         lz_int pivot_per_part_;     ///< Number of pivots sampled per subarray.
         lz_int* part_size_scan_;    ///< Prefix sum of partition sizes.
         std::vector<lz_int> part_ruler_;  ///< Indices of sub-subarrays in each partition.
         std::atomic_uint64_t solved_;     ///< Progress counter for solved subproblems.
         lz_int c;                   ///< Constant for pivot count selection.

         static constexpr lz_int default_subproblem_count = 8192;  ///< Default number of subproblems.
         static constexpr lz_int nested_par_grain_size = 100;      ///< Granularity threshold for nested parallelism.

         /// @brief Type alias for time points.
         typedef std::chrono::high_resolution_clock::time_point time_point_t;
         constexpr static auto now = std::chrono::high_resolution_clock::now;
         constexpr static auto duration = [](const std::chrono::nanoseconds& d) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(d).count();
         };

         /**
          * @brief Computes LCP length between two strings.
          * @param x First string.
          * @param y Second string.
          * @param min_len Length of the shorter string.
          * @return The length of the longest common prefix.
          */
         static lz_int lcp(const char* x, const char* y, lz_int min_len);

         /**
          * @brief Optimized LCP computation using word-level comparison.
          * @param x First string.
          * @param y Second string.
          * @param min_len Length of the shorter string.
          * @return The length of the longest common prefix.
          */
         static lz_int lcp_opt(const char* x, const char* y, lz_int min_len);

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
         /**
          * @brief AVX2-optimized LCP computation.
          * @param x First string.
          * @param y Second string.
          * @param min_len Length of the shorter string.
          * @return The length of the longest common prefix.
          */
         static lz_int lcp_opt_avx(const char* x, const char* y, lz_int min_len);
#endif

         /**
          * @brief Merges two sorted suffix collections with their LCP arrays.
          * @param X First sorted suffix collection.
          * @param len_x Length of X.
          * @param Y Second sorted suffix collection.
          * @param len_y Length of Y.
          * @param LCP_x LCP array for X.
          * @param LCP_y LCP array for Y.
          * @param Z Output merged collection.
          * @param LCP_z Output LCP array for Z.
          */
         void merge(const lz_int* X,
                    lz_int        len_x,
                    const lz_int* Y,
                    lz_int        len_y,
                    const lz_int* LCP_x,
                    const lz_int* LCP_y,
                    lz_int*       Z,
                    lz_int*       LCP_z) const;

         /**
          * @brief Merge-sorts a suffix collection and constructs its LCP array.
          * @param X Input suffix collection (may be modified).
          * @param Y Output sorted collection (must initially equal X).
          * @param n Length of the collection.
          * @param LCP Output LCP array.
          * @param W Working space.
          */
         void merge_sort(lz_int* X, lz_int* Y, lz_int n, lz_int* LCP, lz_int* W) const;

         /**
          * @brief Initializes internal data structures for construction.
          */
         void initialize();

         /**
          * @brief Sorts uniform-sized subarrays independently in parallel.
          */
         void sort_subarrays();

         /**
          * @brief Samples m pivots from a sorted suffix collection.
          * @param X Sorted suffix collection.
          * @param n Size of X.
          * @param m Number of pivots to sample.
          * @param P Output pivot array.
          */
         static void sample_pivots(const lz_int* X, lz_int n, lz_int m, lz_int* P);

         /**
          * @brief Selects pivots for parallel merging of sorted subarrays.
          */
         void select_pivots();

         /**
          * @brief Locates pivot positions in sorted subarrays.
          * @param P Output array for flattened pivot positions.
          */
         void locate_pivots(lz_int* P) const;

         /**
          * @brief Finds upper bound for a pattern in a sorted suffix collection.
          * @param X Sorted suffix collection.
          * @param n Size of X.
          * @param P Query pattern.
          * @param P_len Length of the pattern.
          * @return First index where X[idx] > P.
          */
         lz_int upper_bound(const lz_int* X, lz_int n, const char* P, lz_int P_len) const;

         /**
          * @brief Partitions sub-subarrays based on pivot locations.
          * @param P Pivot location array.
          */
         void partition_sub_subarrays(const lz_int* P);

         /**
          * @brief Merges sorted sub-subarrays within each partition.
          */
         void merge_sub_subarrays();

         /**
          * @brief Computes LCP values at partition boundaries.
          */
         void compute_partition_boundary_lcp();

         /**
          * @brief Sorts a partition containing multiple sorted sub-subarrays.
          * @param X Input collection (may be modified).
          * @param Y Output sorted collection.
          * @param n Number of sub-subarrays.
          * @param S Delimiter indices of sub-subarrays.
          * @param LCP_x Input LCP arrays.
          * @param LCP_y Output LCP array.
          */
         void sort_partition(lz_int* X, lz_int* Y, lz_int n, const lz_int* S, lz_int* LCP_x, lz_int* LCP_y);

         /**
          * @brief Cleans up temporary data after construction.
          */
         void clean_up();

         /**
          * @brief Refreshes internal state for reuse.
          */
         void refresh();

         /**
          * @brief Validates that a suffix array is correctly sorted.
          * @param X Suffix array to validate.
          * @param n Size of X.
          * @return true if X is a valid sorted suffix array.
          */
         bool is_sorted(const lz_int* X, lz_int n) const;

     public:
         bool debug;  ///< Enable debug output.

         /**
          * @brief Constructs CaPS_SA from configuration arguments.
          * @param args SA_Args containing chunk count and max context.
          */
         CaPS_SA(utils::SA_Args);

         /**
          * @brief Constructs CaPS_SA with optional parameters.
          * @param subproblem_count Number of subproblems (0 = auto).
          * @param max_context Maximum prefix length for comparison (0 = unlimited).
          */
         CaPS_SA(lz_int subproblem_count = 0, lz_int max_context = 0);

         /**
          * @brief Constructs CaPS_SA with input text.
          * @param T The input text.
          * @param n Length of the text.
          * @param subproblem_count Number of subproblems (0 = auto).
          * @param max_context Maximum prefix length (0 = unlimited).
          */
         CaPS_SA(std::vector<lz_char> T, lz_int n, lz_int subproblem_count = 0, lz_int max_context = 0);

         /**
          * @brief Copy constructor.
          * @param other The CaPS_SA object to copy.
          */
         CaPS_SA(const CaPS_SA& other);

         /**
          * @brief Move constructor.
          * @param other The CaPS_SA object to move from.
          */
         CaPS_SA(CaPS_SA&& other) noexcept;

         /**
          * @brief Destructor. Frees allocated memory.
          */
         ~CaPS_SA();

         /**
          * @brief Copy assignment operator.
          * @param rhs The object to copy from.
          * @return Reference to this object.
          */
         const CaPS_SA& operator=(const CaPS_SA& rhs) {
            if (this != &rhs) {
               this->~CaPS_SA();
               new (this) CaPS_SA(rhs);
            }
            return *this;
         };

         /**
          * @brief Move assignment operator.
          * @param rhs The object to move from.
          * @return Reference to this object.
          */
         const CaPS_SA& operator=(CaPS_SA&& rhs) {
            if (this != &rhs) {
               T_              = std::move(rhs.T_);
               n_              = std::exchange(rhs.n_, std::numeric_limits<lz_int>::max());
               SA_             = std::move(rhs.SA_);
               LCP_            = std::move(rhs.LCP_);
               p_              = std::exchange(rhs.p_, std::numeric_limits<lz_int>::max());
               c               = std::exchange(rhs.c, std::numeric_limits<lz_int>::max());
               max_context     = std::exchange(rhs.max_context, std::numeric_limits<lz_int>::max());
               pivot_per_part_ = std::exchange(rhs.pivot_per_part_, std::numeric_limits<lz_int>::max());
            }
            return *this;
         };

         /**
          * @brief Swaps the contents of two CaPS_SA objects.
          * @param first First object.
          * @param second Second object.
          */
         friend void swap(CaPS_SA& first, CaPS_SA& second);

         /**
          * @brief Equality comparison based on configuration and input.
          * @return true if objects have same configuration and text.
          */
         friend bool operator==(const CaPS_SA& lhs, const CaPS_SA& rhs) {
            return lhs.n_ == rhs.n_ && lhs.p_ == rhs.p_ && lhs.max_context == rhs.max_context && lhs.T_ == rhs.T_;
         }

         /**
          * @brief Inequality comparison.
          * @return true if objects differ.
          */
         friend bool operator!=(const CaPS_SA& lhs, const CaPS_SA& rhs) { return !operator==(lhs, rhs); }

         /**
          * @brief Returns the input text.
          * @return The text vector.
          */
         auto T() const { return T_; }

         /**
          * @brief Returns the length of the input text.
          * @return The text length.
          */
         auto n() const { return n_; }

         /**
          * @brief Returns the computed suffix array.
          * @return The SA vector.
          */
         auto SA() const { return SA_; }

         /**
          * @brief Returns the computed LCP array.
          * @return The LCP vector.
          */
         auto LCP() const { return LCP_; }

         /**
          * @brief Constructs SA and LCP for the stored text.
          * @return LZ_SuffixArray containing the computed arrays.
          */
         utils::LZ_SuffixArray construct();

         /**
          * @brief Constructs SA and LCP for a given string.
          * @param str The input string.
          * @return LZ_SuffixArray containing the computed arrays.
          */
         utils::LZ_SuffixArray construct(const std::string&);

         /**
          * @brief Constructs SA and LCP for a given character vector.
          * @param T The input text.
          * @param n Length of the text.
          * @return LZ_SuffixArray containing the computed arrays.
          */
         utils::LZ_SuffixArray construct(std::vector<char>, lz_int);

         /**
          * @brief Dumps SA and LCP to a binary output stream.
          * @param output The output file stream.
          */
         void dump(std::ofstream& output);

         /**
          * @brief Dumps SA and LCP to a plain text output stream.
          * @param output The output file stream.
          */
         void dump_plain(std::ofstream& output);
      };

      inline lz_int CaPS_SA::lcp(const char* const x, const char* const y, const lz_int min_len) {
         lz_int l = 0;
         while (l < min_len && x[l] == y[l])
            l++;

         return l;
      }

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
      inline lz_int CaPS_SA::lcp_opt_avx(const char* str1, const char* str2, const lz_int len_in) {
         int64_t i   = 0;
         int64_t len = static_cast<int64_t>(len_in);
         if (len >= 32) {
            for (; i <= len - 32; i += 32) {
               __m256i v1   = _mm256_loadu_si256((__m256i*)(str1 + i));
               __m256i v2   = _mm256_loadu_si256((__m256i*)(str2 + i));
               __m256i cmp  = _mm256_cmpeq_epi8(v1, v2);
               int     mask = _mm256_movemask_epi8(cmp);
               if (mask != 0xFFFFFFFF) {
                  int j = __builtin_ctz(~mask) + i;
                  return static_cast<lz_int>(j);
               }
            }
         }
         for (; i < len; i++) {
            if (str1[i] != str2[i]) {
               break;
            }
         }
         return static_cast<lz_int>(i);
      }
#endif

      inline lz_int CaPS_SA::lcp_opt(const char* const x, const char* const y, const lz_int min_len) {
         auto const X          = reinterpret_cast<const uint64_t*>(x);
         auto const Y          = reinterpret_cast<const uint64_t*>(y);
         const auto word_count = (min_len >> 3);

         lz_int i = 0;
         while (i < word_count && X[i] == Y[i])
            i++;

         return (i << 3) + lcp(x + (i << 3), y + (i << 3), min_len - (i << 3));
      };

   };  // namespace suffixarray

   // template struct utils::LZ_FLAGS<suffixarray::CaPS_SA>;
};  // namespace lz
