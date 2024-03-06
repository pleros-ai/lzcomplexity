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

#include <lz/utils.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
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
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace lz {
   namespace suffixarray {

      // The Suffix Array (SA) and the Longest Common Prefix (LCP) array constructor
      // class for some given sequence.
      class CaPS_SA {
        private:
         const char* T_;                //!> The input text.
         lz_int n_;                     //!> Length of the input text.
         lz_int* SA_;                   //!> The suffix array.
         lz_int* LCP_;                  //!> The LCP array.
         lz_int* SA_w;                  //!> Working space for the SA construction.
         lz_int* LCP_w;                 //!> Working space for the LCP construction.
         const lz_int p_;               //!> Count of subproblems used in construction.
         lz_int max_context;            //!> Maximum prefix-context length for comparing suffixes.
         lz_int* pivot_;                //!> Pivots for the global suffix array.
         const lz_int pivot_per_part_;  //!> Number of pivots to sample per sub-array.
         lz_int* part_size_scan_;       //!> Inclusive scan (prefix sum) of the sizes of the pivoted final
                                        //! partitions containing appropriate sorted sub-subarrays.
         lz_int* part_ruler_;           //!> "Ruler" for the partitions—contains the indices of each
                                        //! sub-sub-array in each partition.
         std::atomic_uint64_t solved_;  //!> Progress tracker—number of subproblems solved in some step.
         lz_int c;                      //!> constant for select the number of pivots by partitions

         static constexpr lz_int default_subproblem_count = 8192;  //!> Default subproblem-count to use in construction.
         static constexpr lz_int nested_par_grain_size = (100);    //!> Granularity for nested parallelism to kick in.

         // Fields for profiling time.
         typedef std::chrono::high_resolution_clock::time_point time_point_t;
         constexpr static auto now = std::chrono::high_resolution_clock::now;
         constexpr static auto duration = [](const std::chrono::nanoseconds& d) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(d).count();
         };

         // Returns the LCP length of `x` and `y`, where `min_len` is the length of
         // the shorter of `x` and `y`.
         static lz_int lcp(const char* x, const char* y, lz_int min_len);

         // Returns the LCP length of `x` and `y`, where `min_len` is the length of
         // the shorter of `x` and `y`. Optimized with some poor man's vectorization.
         static lz_int lcp_opt(const char* x, const char* y, lz_int min_len);

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
         // Returns the LCP length of `x` and `y`, where `min_len` is the length of
         // the shorter of `x` and `y`. Optimized with some poor man's vectorization.
         static lz_int lcp_opt_avx(const char* x, const char* y, lz_int min_len);
#endif

         // Merges the sorted collections of suffixes, `X` and `Y`, with lengths
         // `len_x` and `len_y` and LCP arrays `LCP_x` and `LCP_y` respectively, into
         // `Z`. Also constructs `Z`'s LCP array in `LCP_z`.
         void merge(const lz_int* X, lz_int len_x, const lz_int* Y, lz_int len_y, const lz_int* LCP_x,
                    const lz_int* LCP_y, lz_int* Z, lz_int* LCP_z) const;

         // Merge-sorts the suffix collection `X` of length `n` into `Y`. Also
         // constructs the LCP array of `X` in `LCP`, using `W` as working space.
         // A necessary precondition is that `Y` must be equal to `X`.  `X` may
         // not remain the same after the sort.
         void merge_sort(lz_int* X, lz_int* Y, lz_int n, lz_int* LCP, lz_int* W) const;

         // Initializes internal data structures for the construction algorithm.
         void initialize();

         // Sorts uniform-sized subarrays independently.
         void sort_subarrays();

         // Samples `m` pivots from the sorted suffix collection `X` of size `n`
         // into `P`.
         static void sample_pivots(const lz_int* X, lz_int n, lz_int m, lz_int* P);

         // Selects pivots for parallel merging of the sorted subarrays.
         void select_pivots();

         // Locates the positions (upper-bounds) of the selected pivots in the sorted
         // subarrays and flattens them in `P`. Besides these pivots, two flanking
         // pivots, `0` and `|X_i|`, for each subarray `X_i` are also placed.
         void locate_pivots(lz_int* P) const;

         // Returns the first index `idx` into the sorted suffix collection `X` of
         // length `n` such that `X[idx]` is strictly greater than the query pattern
         // `P` of length `P_len`.
         lz_int upper_bound(const lz_int* X, lz_int n, const char* P, lz_int P_len) const;

         // Collates the sub-subarrays delineated by the pivot locations in each
         // sorted subarray, present in `P`, into appropriate partitions.
         void partition_sub_subarrays(const lz_int* P);

         // Merges the sorted sub-subarrays laid flat together in each partition.
         void merge_sub_subarrays();

         // Computes the LCPs at the partition boundaries, specifically at the
         // starting index of each partition in their flat collection.
         void compute_partition_boundary_lcp();

         // Merge-sorts the collection `X` that contains `n` sorted arrays of
         // suffixes laid flat together, into `Y`. `S` contains the delimiter indices
         // of the `n` arrays in `X`. The LCP array of sorted `X` is constructed in
         // `LCP_y`; `LCP_x` contains the LCP arrays of the `n` arrays of `X`.
         // A necessary precondition is that `Y` must be equal to `X`, and `LCP_y` to
         // `LCP_x`. `X` and `LCP_x` may not remain the same after the sort.
         void sort_partition(lz_int* X, lz_int* Y, lz_int n, const lz_int* S, lz_int* LCP_x, lz_int* LCP_y);

         // Cleans up after the construction algorithm.
         void clean_up();
         void refresh();

         // Returns pointer to a memory-allocation for `size` elements of type `T_`.
         // static lz_int* allocate(lz_int size) { return static_cast<lz_int*>(std::malloc(size *
         // sizeof(T_))); }

         // Returns true iff `X` is a valid (partial) suffix array with size `n`.
         bool is_sorted(const lz_int* X, lz_int n) const;

        public:
         bool debug;
         // Constructs a suffix array object for the input text `T` of size
         // `n`. Optionally, the number of subproblems to decompose the original
         // construction problem into can be provided with `subproblem_count`, and
         // the maximum prefix-context length for the suffixes can be bounded by
         // `max_context`.
         // CaPS_SA() :CaPS_SA("", 1, 1) {};
         CaPS_SA(utils::SA_Args);
         CaPS_SA(lz_int subproblem_count = 0, lz_int max_context = 0);
         CaPS_SA(const char* T, lz_int n, lz_int subproblem_count = 0, lz_int max_context = 0);

         // Copy constructs the suffix array object from `other`.
         CaPS_SA(const CaPS_SA& other);

         // Move constructs the suffix array object from `other`.
         CaPS_SA(CaPS_SA&& other) noexcept;

         ~CaPS_SA();

         auto operator()(std::string str) -> utils::LZ_SuffixArray { return construct(str); }
         auto operator()(const char* str, lz_int n) -> utils::LZ_SuffixArray { return construct(str, n); }

         // Copy assignment
         const CaPS_SA& operator=(const CaPS_SA& rhs) {
            if (this != &rhs) {
               this->~CaPS_SA();
               new (this) CaPS_SA(rhs);
            }
            return *this;
         };

         // Move assignment
         const CaPS_SA& operator=(CaPS_SA&& rhs) {
            if (this != &rhs) {
               this->~CaPS_SA();
               new (this) CaPS_SA(rhs);
            }
            return *this;
         };

         friend void swap(CaPS_SA& first, CaPS_SA& second);

         friend constexpr bool operator==(const CaPS_SA& lhs, const CaPS_SA& rhs) {
            return lhs.n_ == rhs.n_ && lhs.p_ == rhs.p_ && lhs.max_context == rhs.max_context &&
                   std::equal(lhs.T_, lhs.T_ + lhs.n_, rhs.T_);
         }

         friend constexpr bool operator!=(const CaPS_SA& lhs, const CaPS_SA& rhs) { return !operator==(lhs, rhs); }

         // Returns the text.
         constexpr auto T() const { return T_; }

         // Returns the length of the text.
         constexpr auto n() const { return n_; }

         // Returns the suffix array.
         constexpr auto SA() const { return SA_; }

         // Returns the LCP array.
         constexpr auto LCP() const { return LCP_; }

         // Constructs the suffix array and the LCP array.
         utils::LZ_SuffixArray construct();
         utils::LZ_SuffixArray construct(const std::string&);
         utils::LZ_SuffixArray construct(const char*, lz_int);

         // Dumps the suffix array and the LCP array into the stream `output`.
         void dump(std::ofstream& output);
         void dump_plain(std::ofstream& output);
      };

      inline lz_int CaPS_SA::lcp(const char* const x, const char* const y, const lz_int min_len) {
         lz_int l = 0;
         while (l < min_len && x[l] == y[l]) l++;

         return l;
      }

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
      inline lz_int CaPS_SA::lcp_opt_avx(const char* str1, const char* str2, const lz_int len_in) {
         int64_t i = 0;
         int64_t len = static_cast<int64_t>(len_in);
         if (len >= 32) {
            for (; i <= len - 32; i += 32) {
               __m256i v1 = _mm256_loadu_si256((__m256i*)(str1 + i));
               __m256i v2 = _mm256_loadu_si256((__m256i*)(str2 + i));
               __m256i cmp = _mm256_cmpeq_epi8(v1, v2);
               int mask = _mm256_movemask_epi8(cmp);
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
         auto const X = reinterpret_cast<const uint64_t*>(x);
         auto const Y = reinterpret_cast<const uint64_t*>(y);
         const auto word_count = (min_len >> 3);

         lz_int i = 0;
         while (i < word_count && X[i] == Y[i]) i++;

         return (i << 3) + lcp(x + (i << 3), y + (i << 3), min_len - (i << 3));
      };

   };  // namespace suffixarray

   // template struct utils::LZ_FLAGS<suffixarray::CaPS_SA>;
};  // namespace lz
