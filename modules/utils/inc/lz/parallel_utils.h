#pragma once

#include "general.h"
#include "lz_arena.h"

namespace lz {

   namespace internal {
      void parallel_for_impl(lz_size start, lz_size end, const std::function<void(lz_size)>& fun, long granularity = 0);

      double parallel_reduce_impl(lz_size init, lz_size end, double init_value,
                                  const std::function<double(internal::LZ_BlockedRange<lz_size>, double)>& fun,
                                  const std::function<double(double, double)>& reduce_fun);
      float parallel_reduce_impl(lz_size init, lz_size end, float init_value,
                                 const std::function<float(internal::LZ_BlockedRange<lz_size>, float)>& fun,
                                 const std::function<float(float, float)>& reduce_fun);

      void parallel_do_impl(const std::vector<std::function<void()>>& funcs);
   }  // namespace internal

   namespace utils {
      void EnabledMT(lz_uint numthreads);
      void DisabledMT();

      lz_size num_workers();

      lz_size worker_id();

      template <typename Fun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<Fun&&, lz_size>)
#endif
          inline void parallel_for(lz_size start, lz_size end, Fun&& fun, long granularity = 0) {
         // Use TBB's automatic granularity partitioner (tbb::auto_partitioner)
         internal::parallel_for_impl(start, end, std::forward<Fun>(fun), granularity);
      }

      template <typename T, typename Acc, typename ReduceFun>
#ifdef __cpp_concepts
      requires(
          std::is_invocable_v<ReduceFun&&, T, T>&& std::is_invocable_v<Acc&&, internal::LZ_BlockedRange<lz_size>, T>)
#endif
          inline auto parallel_reduce(lz_size init, lz_size end, T init_value, Acc&& acc_fun, ReduceFun&& reduce_fun) {

         return internal::parallel_reduce_impl(init, end, init_value, std::forward<Acc>(acc_fun),
                                               std::forward<ReduceFun>(reduce_fun));
      }

      template <typename... Fun>
#ifdef __cpp_concepts
      requires(std::is_invocable_v<Fun&&>&&...)
#endif
          inline void par_do(Fun&&... fun) {
         internal::parallel_do_impl({std::forward<Fun>(fun)...});
         // tbb::parallel_invoke(std::forward<Fun>(fun)...);
      }

   }  // namespace utils

}  // namespace lz
