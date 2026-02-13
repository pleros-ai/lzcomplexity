#pragma once

// =============================================================================
// cilk.hpp - Cilk Backend Implementation (OpenCilk / Intel Cilk Plus)
// =============================================================================

#include "parallel_common.h"

// Cilk runtime header - works with both OpenCilk and Intel Cilk Plus
#if defined(__cilk)
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#endif

namespace lz {

  namespace internal {

    // -------------------------------------------------------------------------
    // Cilk-specific parallel implementations (defined in cilk.cpp)
    // -------------------------------------------------------------------------

    void parallel_for_impl(lz_size                             start,
                           lz_size                             end,
                           const std::function<void(lz_size)>& fun,
                           long                                granularity = 0);

    void parallel_do_impl(const std::vector<std::function<void()>>& funcs);

    template<typename ReturnType>
    ReturnType parallel_reduce_impl(
      lz_size                                                                          init,
      lz_size                                                                          end,
      ReturnType                                                                       init_value,
      const std::function<ReturnType(internal::LZ_BlockedRange<lz_size>, ReturnType)>& fun,
      const std::function<ReturnType(ReturnType, ReturnType)>&                         reduce_fun);

  }  // namespace internal

  namespace utils {

    // -------------------------------------------------------------------------
    // Cilk-specific utility functions (defined in cilk.cpp)
    // -------------------------------------------------------------------------

    void EnabledMT(lz_uint numthreads);
    void DisabledMT();

    lz_size num_workers();
    lz_size worker_id();

  }  // namespace utils

}  // namespace lz
