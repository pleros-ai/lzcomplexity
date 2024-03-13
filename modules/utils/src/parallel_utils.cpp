#include <lz/parallel_utils.h>

#include "lz_tbb_arena.h"

// External includes
// Require TBB without captured exceptions
#define TBB_USE_CAPTURED_EXCEPTION 0

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "tbb/tbb.h"
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif

namespace lz {
   namespace utils {

      static std::shared_ptr<utils::LZArenaWrapper>& __GetTaskArena() {
         static std::shared_ptr<utils::LZArenaWrapper> globalTaskArena;
         return globalTaskArena;
      }

      void EnabledMT(lz_uint numthreads) { __GetTaskArena() = utils::GetGlobalTaskArena(numthreads); };

      void DisabledMT() { __GetTaskArena().reset(); };

      lz_size num_workers() { return internal::CPU_Bandwidth(); }

      lz_size worker_id() {
         auto id = tbb::this_task_arena::current_thread_index();
         return id == tbb::task_arena::not_initialized ? 0 : id;
      }
   }  // namespace utils

   namespace internal {

      void parallel_for_impl(lz_size start, lz_size end, const std::function<void(lz_size)>& fun, long granularity) {
         auto arena = lz::utils::GetGlobalTaskArena();
         arena->Access().execute([&] {
            if (granularity == 0) {
               tbb::parallel_for(
                   tbb::blocked_range<lz_size>(start, end),
                   [&](const tbb::blocked_range<lz_size>& r) {
                      for (auto i = r.begin(); i != r.end(); ++i) {
                         fun(i);
                      }
                   },
                   tbb::auto_partitioner{});
            }
            // Otherwise, use the granularity specified by the user (tbb::simple_partitioner)
            else {
               tbb::parallel_for(
                   tbb::blocked_range<lz_size>(start, end, granularity),
                   [&](const tbb::blocked_range<lz_size>& r) {
                      for (auto i = r.begin(); i != r.end(); ++i) {
                         fun(i);
                      }
                   },
                   tbb::simple_partitioner{});
            }
         });
      }

      void parallel_do_impl(const std::vector<std::function<void()>>& funcs) {
         auto arena = lz::utils::GetGlobalTaskArena();
         arena->Access().execute([&] {
            tbb::this_task_arena::isolate([&] {
               tbb::parallel_for(tbb::blocked_range(0ul, funcs.size()), [&](const tbb::blocked_range<lz_size>& r) {
                  for (auto i = r.begin(); i < r.end(); i++) funcs[i]();
               });
            });
         });
      }

      float parallel_reduce_impl(lz_size init, lz_size end, float init_value,
                                 const std::function<float(internal::LZ_BlockedRange<lz_size>, float)>& fun,
                                 const std::function<float(float, float)>& reduce_fun) {
         float result = init_value;
         auto arena = lz::utils::GetGlobalTaskArena();
         arena->Access().execute([&] {
            tbb::this_task_arena::isolate([&] {
               result =
                   tbb::parallel_reduce(internal::LZ_BlockedRange<lz_size>(init, end), init_value, fun, reduce_fun);
            });
         });
         return result;
      }

      double parallel_reduce_impl(lz_size init, lz_size end, double init_value,
                                  const std::function<double(internal::LZ_BlockedRange<lz_size>, double)>& fun,
                                  const std::function<double(double, double)>& reduce_fun) {
         double result = init_value;
         auto arena = lz::utils::GetGlobalTaskArena();
         arena->Access().execute([&] {
            tbb::this_task_arena::isolate([&] {
               result =
                   tbb::parallel_reduce(internal::LZ_BlockedRange<lz_size>(init, end), init_value, fun, reduce_fun);
            });
         });
         return result;
      }
   }  // namespace internal

}  // namespace lz
