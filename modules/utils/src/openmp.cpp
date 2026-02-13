// =============================================================================
// openmp.cpp - OpenMP Backend Implementation
// =============================================================================

#include <lz/internal/openmp.hpp>

#ifdef _OPENMP
#include <omp.h>
#else
// Fallback stubs when OpenMP is not available
inline int  omp_get_max_threads() { return 1; }
inline int  omp_get_thread_num() { return 0; }
inline void omp_set_num_threads(int) {}
#endif

namespace lz {

  namespace utils {

    // Thread count management
    static int g_max_threads = 0;

    void EnabledMT(lz_uint numthreads) {
      if (numthreads > 0) {
        g_max_threads = static_cast<int>(numthreads);
        omp_set_num_threads(g_max_threads);
      } else {
        g_max_threads = omp_get_max_threads();
      }
    }

    void DisabledMT() {
      g_max_threads = 1;
      omp_set_num_threads(1);
    }

    lz_size num_workers() { return static_cast<lz_size>(omp_get_max_threads()); }

    lz_size worker_id() { return static_cast<lz_size>(omp_get_thread_num()); }

  }  // namespace utils

  namespace internal {

    void parallel_for_impl(lz_size                             start,
                           lz_size                             end,
                           const std::function<void(lz_size)>& fun,
                           [[maybe_unused]] long               granularity) {
      // OpenMP handles granularity automatically via schedule
      // granularity parameter is ignored but could be used with schedule(static, granularity)

      const auto count = static_cast<long long>(end - start);

#pragma omp parallel for schedule(dynamic)
      for (long long i = 0; i < count; ++i) {
        fun(start + static_cast<lz_size>(i));
      }
    }

    void parallel_do_impl(const std::vector<std::function<void()>>& funcs) {
      const auto count = static_cast<long long>(funcs.size());

#pragma omp parallel for schedule(dynamic)
      for (long long i = 0; i < count; ++i) {
        funcs[static_cast<size_t>(i)]();
      }
    }

    // Template specialization for parallel_reduce_impl
    // OpenMP reduction is more complex - we use a manual reduction pattern
    template<typename ReturnType>
    ReturnType parallel_reduce_impl(
      lz_size                                                                          init,
      lz_size                                                                          end,
      ReturnType                                                                       init_value,
      const std::function<ReturnType(internal::LZ_BlockedRange<lz_size>, ReturnType)>& fun,
      const std::function<ReturnType(ReturnType, ReturnType)>&                         reduce_fun) {
      const int               num_threads = omp_get_max_threads();
      std::vector<ReturnType> partial_results(static_cast<size_t>(num_threads), init_value);

      const lz_size range_size = end - init;
      const lz_size chunk_size
        = (range_size + static_cast<lz_size>(num_threads) - 1) / static_cast<lz_size>(num_threads);

#pragma omp parallel
      {
        const int     tid = omp_get_thread_num();
        const lz_size thread_start = init + static_cast<lz_size>(tid) * chunk_size;
        const lz_size thread_end = std::min(thread_start + chunk_size, end);

        if (thread_start < end) {
          LZ_BlockedRange<lz_size> range(thread_start, thread_end);
          partial_results[static_cast<size_t>(tid)] = fun(range, init_value);
        }
      }

      // Reduce partial results
      ReturnType result = init_value;
      for (const auto& partial: partial_results) {
        result = reduce_fun(result, partial);
      }

      return result;
    }

  }  // namespace internal

}  // namespace lz

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
  const std::function<std::vector<lz::sequence>(lz::internal::LZ_BlockedRange<lz_size>,
                                                std::vector<lz::sequence>)>&,
  const std::function<std::vector<lz::sequence>(std::vector<lz::sequence>, std::vector<lz::sequence>)>&);
