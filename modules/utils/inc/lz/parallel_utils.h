#pragma once

// =============================================================================
// parallel_utils.h - Unified Parallel Computing API
// =============================================================================
// This header provides a unified API for parallel computing that works with
// multiple backends: TBB, OpenMP, Cilk, or sequential fallback.
//
// The backend is selected at compile time via preprocessor definitions:
//   - LZ_PARALLEL_TBB    : Intel oneTBB
//   - LZ_PARALLEL_OPENMP : OpenMP
//   - LZ_PARALLEL_CILK   : OpenCilk / Intel Cilk Plus
//   - (none)             : Sequential fallback
// =============================================================================

#include "types.h"

// Include the appropriate backend header
// Each backend header includes parallel_common.h and declares the internal functions
#if defined(LZ_PARALLEL_OPENMP)
#include "internal/openmp.hpp"

#elif defined(LZ_PARALLEL_TBB)
#include "internal/tbb.hpp"

#elif defined(LZ_PARALLEL_CILK)
#include "internal/cilk.hpp"

#else
#include "internal/generic.hpp"

#endif

namespace lz {

  namespace utils {

    // =========================================================================
    // Public Parallel API
    // =========================================================================
    // These template functions provide the user-facing API. They delegate to
    // the internal::*_impl functions which are implemented by each backend.
    // =========================================================================

    // -------------------------------------------------------------------------
    // parallel_for - Parallel iteration over a range
    // -------------------------------------------------------------------------
    // Executes fun(i) for each i in [start, end) in parallel.
    // The granularity parameter is a hint for work partitioning (0 = automatic).
    // -------------------------------------------------------------------------
    template<typename Fun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<Fun &&, lz_size>
               || std::is_invocable_v<Fun &&, internal::LZ_BlockedRange<lz_size>>)
#endif
    inline void parallel_for(lz_size start, lz_size end, Fun&& fun, long granularity = 0) {
      internal::parallel_for_impl(start, end, std::forward<Fun>(fun), granularity);
    }

    // -------------------------------------------------------------------------
    // parallel_reduce - Parallel reduction over a range
    // -------------------------------------------------------------------------
    // Applies acc_fun to each sub-range and combines results using reduce_fun.
    // -------------------------------------------------------------------------
    template<typename T, typename Acc, typename ReduceFun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<ReduceFun, T, T>
               && std::is_invocable_v<Acc, internal::LZ_BlockedRange<lz_size>, T>)
#endif
    inline auto parallel_reduce(lz_size     init,
                                lz_size     end,
                                T           init_value,
                                Acc&&       acc_fun,
                                ReduceFun&& reduce_fun) {
      return internal::parallel_reduce_impl<T>(
        init, end, init_value, std::forward<Acc>(acc_fun), std::forward<ReduceFun>(reduce_fun));
    }

    // -------------------------------------------------------------------------
    // for_each - Parallel for_each over a vector
    // -------------------------------------------------------------------------
    template<typename T, typename Fun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<Fun &&, T>)
#endif
    inline void for_each(std::vector<T>& vec, Fun&& f, long granularity = 0) {
      auto lambda = [&vec, &f](lz_size idx) { f(vec[idx]); };
      internal::parallel_for_impl(0ul, vec.size(), lambda, granularity);
    }

    // -------------------------------------------------------------------------
    // map - Parallel map over a vector
    // -------------------------------------------------------------------------
    template<typename T, typename Fun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<Fun &&, T>)
#endif
    inline auto map(const std::vector<T>& vec, Fun&& fun, long granularity = 0)
      -> std::vector<invoke_result_t<Fun, T>> {
      using resType = invoke_result_t<Fun, T>;
      std::vector<resType> res(vec.size());
      auto                 lambda = [&vec, &fun, &res](lz_size idx) { res[idx] = fun(vec[idx]); };
      internal::parallel_for_impl(0ul, vec.size(), lambda, granularity);
      return res;
    }

    // -------------------------------------------------------------------------
    // par_do - Execute multiple functions in parallel
    // -------------------------------------------------------------------------
    template<typename... Fun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<Fun &&> && ...)
#endif
    inline void par_do(Fun&&... fun) {
      internal::parallel_do_impl({std::forward<Fun>(fun)...});
    }

  }  // namespace utils

}  // namespace lz