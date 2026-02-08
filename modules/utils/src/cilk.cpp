// =============================================================================
// cilk.cpp - Cilk Backend Implementation (OpenCilk / Intel Cilk Plus)
// =============================================================================

#include <lz/internal/cilk.hpp>
#include <lz/sequence.h>

#include <thread>

// Cilk runtime - conditionally included
#if defined(__cilk)
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#endif

namespace lz {

   namespace utils {

      void EnabledMT([[maybe_unused]] lz_uint numthreads) {
#if defined(__cilk)
         // Cilk runtime manages workers automatically
         // Can set via environment variable CILK_NWORKERS before program start
         // or use __cilkrts_set_param for some implementations
         if (numthreads > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", numthreads);
            __cilkrts_set_param("nworkers", buf);
         }
#endif
      }

      void DisabledMT() {
#if defined(__cilk)
         __cilkrts_set_param("nworkers", "1");
#endif
      }

      lz_size num_workers() {
#if defined(__cilk)
         return static_cast<lz_size>(__cilkrts_get_nworkers());
#else
         return static_cast<lz_size>(std::thread::hardware_concurrency());
#endif
      }

      lz_size worker_id() {
#if defined(__cilk)
         return static_cast<lz_size>(__cilkrts_get_worker_number());
#else
         return 0;
#endif
      }

   }  // namespace utils

   namespace internal {

      void parallel_for_impl(lz_size                             start,
                             lz_size                             end,
                             const std::function<void(lz_size)>& fun,
                             [[maybe_unused]] long               granularity) {
#if defined(__cilk)
         // cilk_for provides automatic work-stealing parallelism
         cilk_for (lz_size i = start; i < end; ++i) {
            fun(i);
         }
#else
         // Fallback to sequential if Cilk not available
         for (lz_size i = start; i < end; ++i) {
            fun(i);
         }
#endif
      }

      void parallel_do_impl(const std::vector<std::function<void()>>& funcs) {
#if defined(__cilk)
         cilk_for (size_t i = 0; i < funcs.size(); ++i) {
            funcs[i]();
         }
#else
         for (const auto& f : funcs) {
            f();
         }
#endif
      }

      // Template specialization for parallel_reduce_impl
      // Cilk uses divide-and-conquer pattern for reduction
      template<typename ReturnType>
      ReturnType parallel_reduce_impl(
         lz_size                                                                          init,
         lz_size                                                                          end,
         ReturnType                                                                       init_value,
         const std::function<ReturnType(internal::LZ_BlockedRange<lz_size>, ReturnType)>& fun,
         [[maybe_unused]] const std::function<ReturnType(ReturnType, ReturnType)>&        reduce_fun) {
         
         const lz_size range_size = end - init;
         const lz_size threshold = 1024;  // Granularity threshold
         
         if (range_size <= threshold) {
            // Base case: process sequentially
            LZ_BlockedRange<lz_size> range(init, end);
            return fun(range, init_value);
         }
         
#if defined(__cilk)
         // Divide and conquer
         const lz_size mid = init + range_size / 2;
         
         ReturnType left_result;
         ReturnType right_result;
         
         // Spawn left half
         left_result = cilk_spawn parallel_reduce_impl(init, mid, init_value, fun, reduce_fun);
         
         // Process right half
         right_result = parallel_reduce_impl(mid, end, init_value, fun, reduce_fun);
         
         // Sync and combine
         cilk_sync;
         
         return reduce_fun(left_result, right_result);
#else
         // Sequential fallback
         LZ_BlockedRange<lz_size> range(init, end);
         return fun(range, init_value);
#endif
      }

   }  // namespace internal

}  // namespace lz

// Explicit template instantiations for common types
template float lz::internal::parallel_reduce_impl<float>(
   lz_size,
   lz_size,
   float,
   const std::function<float(lz::internal::LZ_BlockedRange<lz_size>, float)>&,
   const std::function<float(float, float)>&);

template double lz::internal::parallel_reduce_impl<double>(
   lz_size,
   lz_size,
   double,
   const std::function<double(lz::internal::LZ_BlockedRange<lz_size>, double)>&,
   const std::function<double(double, double)>&);

template int lz::internal::parallel_reduce_impl<int>(
   lz_size,
   lz_size,
   int,
   const std::function<int(lz::internal::LZ_BlockedRange<lz_size>, int)>&,
   const std::function<int(int, int)>&);

template std::vector<lz::sequence> lz::internal::parallel_reduce_impl<std::vector<lz::sequence>>(
   lz_size,
   lz_size,
   std::vector<lz::sequence>,
   const std::function<std::vector<lz::sequence>(lz::internal::LZ_BlockedRange<lz_size>, std::vector<lz::sequence>)>&,
   const std::function<std::vector<lz::sequence>(std::vector<lz::sequence>, std::vector<lz::sequence>)>&);
