#pragma once

// =============================================================================
// generic.hpp - Sequential Fallback Implementation
// =============================================================================
// This backend provides sequential (single-threaded) implementations of all
// parallel primitives. Used when no parallel library is available.
// =============================================================================

#include "parallel_common.h"

namespace lz {

   namespace internal {

      // -------------------------------------------------------------------------
      // Sequential parallel implementations (defined in generic.cpp)
      // -------------------------------------------------------------------------

      void parallel_for_impl(lz_size                                   start,
                             lz_size                                   end,
                             const std::function<void(lz_size)>&       fun,
                             long                                      granularity = 0);

      void parallel_do_impl(const std::vector<std::function<void()>>& funcs);

      template<typename ReturnType>
      ReturnType parallel_reduce_impl(
         lz_size                                                                    init,
         lz_size                                                                    end,
         ReturnType                                                                 init_value,
         const std::function<ReturnType(internal::LZ_BlockedRange<lz_size>, ReturnType)>& fun,
         const std::function<ReturnType(ReturnType, ReturnType)>&                   reduce_fun);

   }  // namespace internal

   namespace utils {

      // -------------------------------------------------------------------------
      // Sequential utility functions (defined in generic.cpp)
      // -------------------------------------------------------------------------

      void EnabledMT(lz_uint numthreads);
      void DisabledMT();

      lz_size num_workers();
      lz_size worker_id();

   }  // namespace utils

}  // namespace lz