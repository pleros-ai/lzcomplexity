#pragma once
// Language includes
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstddef>
#ifdef __cpp_lib_ranges
#include <ranges>
#endif
#include <algorithm>
#include <random>
#include <cstdarg>
#include <utility>
#include <concepts>
#include <type_traits>
// External includes
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/tbb_allocator.h>
#include <tbb/global_control.h>
#include <csv.h>
// Local includes
#include "lzexceptions.h"

namespace lz {
  using lz_int = int;
  using lz_uint = unsigned int;
  using lz_size = std::size_t;
  using lz_double = double;
  using lz_bool = bool;
  using lz_str = std::string;

  const lz_str RED_COLOR = "\033[1;31m";
  const lz_str GREEN_COLOR = "\033[1;32m";
  const lz_str YELLOW_COLOR = "\033[1;33m";
  const lz_str BLUE_COLOR = "\033[1;34m";
  const lz_str MAGENTA_COLOR = "\033[1;35m";
  const lz_str END_COLOR = "\033[0m";
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

    inline lz_size num_workers() { return tbb::this_task_arena::max_concurrency(); }

    inline lz_size worker_id() {
      auto id = tbb::this_task_arena::current_thread_index();
      return id == tbb::task_arena::not_initialized ? 0 : id;
    }

    template <typename Fun>
      requires (std::is_invocable_v<Fun&&, lz_size>)
    inline void parallel_for(lz_size start, lz_size end, Fun&& fun, long granularity = 0) {
      // static_assert(std::is_invocable_v<F&, lz_size>);
      // Use TBB's automatic granularity partitioner (tbb::auto_partitioner)
      if (granularity == 0) {
        tbb::parallel_for(tbb::blocked_range<lz_size>(start, end), [&](const tbb::blocked_range<lz_size>& r) {
          for (auto i = r.begin(); i != r.end(); ++i) {
            fun(i);
          }
          }, tbb::auto_partitioner{});
      }
      // Otherwise, use the granularity specified by the user (tbb::simple_partitioner)
      else {
        tbb::parallel_for(tbb::blocked_range<lz_size>(start, end, granularity), [&](const tbb::blocked_range<lz_size>& r) {
          for (auto i = r.begin(); i != r.end(); ++i) {
            fun(i);
          }
          }, tbb::simple_partitioner{});
      }
    }

    template <typename... Fun>
      requires (std::is_invocable_v<Fun&&> &&...)
    inline void par_do(Fun&&... fun) {
      tbb::parallel_invoke(std::forward<Fun>(fun)...);
    }

    // template <typename... Fs>
    // void execute_with_scheduler(Fs...) {
    //   struct Illegal {};
    //   static_assert((std::is_same_v<Illegal, Fs> && ...), "parlay::execute_with_scheduler is only available in the Parlay scheduler and is not compatible with TBB");
    // }

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
        if (M == Mold)
          break;

        Mold = M;
      }

      return 	M;
    }

    // Returns pointer to a memory-allocation for `size` elements of type `T_`.
    template<typename T>
    inline T* allocate(unsigned int size) { return static_cast<T*>(std::malloc(size * sizeof(T))); }
  }
}
