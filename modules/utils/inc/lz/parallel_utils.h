#pragma once

#include <tbb/blocked_range.h>

#include "general.h"
#include "lz_arena.h"
#include "types.h"

namespace lz {

   namespace internal {
      void parallel_for_impl(lz_size start, lz_size end, const std::function<void(lz_size)>& fun, long granularity = 0);

      void parallel_for_impl_2(lz_size                                                 start,
                               lz_size                                                 end,
                               const std::function<void(tbb::blocked_range<lz_size>)>& fun,
                               long                                                    granularity = 0);

      template<typename ReturnType>
      ReturnType
         parallel_reduce_impl(lz_size    init,
                              lz_size    end,
                              ReturnType init_value,
                              const std::function<ReturnType(internal::LZ_BlockedRange<lz_size>, ReturnType)>& fun,
                              const std::function<ReturnType(ReturnType, ReturnType)>& reduce_fun);

      void parallel_do_impl(const std::vector<std::function<void()>>& funcs);
   }  // namespace internal

   namespace utils {
      void EnabledMT(lz_uint numthreads);
      void DisabledMT();

      lz_size num_workers();

      lz_size worker_id();

      template<typename Fun>
#ifdef __cpp_concepts
         requires(std::is_invocable_v<Fun &&, lz_size> ||
                  std::is_invocable_v<Fun &&, internal::LZ_BlockedRange<lz_size>>)

#endif
      inline void parallel_for(lz_size start, lz_size end, Fun&& fun, long granularity = 0) {
         // Use TBB's automatic granularity partitioner (tbb::auto_partitioner)
         internal::parallel_for_impl(start, end, std::forward<Fun>(fun), granularity);
      }

      template<typename T, typename Acc, typename ReduceFun>
#ifdef __cpp_concepts
         requires(std::is_invocable_v<ReduceFun, T, T> &&
                  std::is_invocable_v<Acc, internal::LZ_BlockedRange<lz_size>, T>)
#endif
      inline auto parallel_reduce(lz_size init, lz_size end, T init_value, Acc&& acc_fun, ReduceFun&& reduce_fun) {
         return internal::parallel_reduce_impl<T>(
            init, end, init_value, std::forward<Acc>(acc_fun), std::forward<ReduceFun>(reduce_fun));
      }

      template<typename T, typename Fun>
#ifdef __cpp_concepts
         requires(std::is_invocable_v<Fun &&, T>)
#endif
      inline auto for_each(std::vector<T> vec, Fun&& f, long granularity = 0) {
         auto lambda = [&vec, &f](unsigned idx) { f(vec[idx]); };
         internal::parallel_for_impl(0ul, vec.size(), lambda, granularity);
      }

      template<typename T, typename Fun>
#ifdef __cpp_concepts
         requires(std::is_invocable_v<Fun &&, T>)
#endif
      inline auto map(std::vector<T> vec, Fun&& fun, long granularity = 0)
         -> std::vector<utils::invoke_result_t<Fun, T>> {
         using resType = decltype(fun(*vec.begin()));
         std::vector<resType> res(vec.size());
         auto                 lambda = [&vec, &fun, &res](unsigned idx) { res[idx] = fun(vec[idx]); };
         internal::parallel_for_impl(0ul, vec.size(), lambda, granularity);

         return res;
      }

      template<typename... Fun>
#ifdef __cpp_concepts
         requires(std::is_invocable_v<Fun &&> && ...)
#endif
      inline void par_do(Fun&&... fun) {
         internal::parallel_do_impl({std::forward<Fun>(fun)...});
         // tbb::parallel_invoke(std::forward<Fun>(fun)...);
      }

   }  // namespace utils

}  // namespace lz