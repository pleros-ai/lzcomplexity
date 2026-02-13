// =============================================================================
// generic.cpp - Sequential Fallback Implementation
// =============================================================================
// This backend provides sequential (single-threaded) implementations of all
// parallel primitives. Used when no parallel library is available.
// =============================================================================

#include <lz/sequence.h>

#include <lz/internal/generic.hpp>
#include <thread>

namespace lz {

  namespace internal {

    int CPU_Bandwidth() { return static_cast<int>(std::thread::hardware_concurrency()); }

  }  // namespace internal

  namespace utils {

    void EnabledMT([[maybe_unused]] lz_uint numthreads) {
      // No-op for sequential backend
    }

    void DisabledMT() {
      // No-op for sequential backend
    }

    lz_size num_workers() {
      // Always 1 for sequential backend
      return 1;
    }

    lz_size worker_id() {
      // Always 0 for sequential backend
      return 0;
    }

  }  // namespace utils

  namespace internal {

    void parallel_for_impl(lz_size                             start,
                           lz_size                             end,
                           const std::function<void(lz_size)>& fun,
                           [[maybe_unused]] long               granularity) {
      // Sequential implementation - just a simple for loop
      for (lz_size i = start; i < end; ++i) {
        fun(i);
      }
    }

    void parallel_do_impl(const std::vector<std::function<void()>>& funcs) {
      // Sequential implementation - execute each function in order
      for (const auto& f: funcs) {
        f();
      }
    }

    // Template specialization for parallel_reduce_impl
    template<typename ReturnType>
    ReturnType parallel_reduce_impl(
      lz_size                                                                          init,
      lz_size                                                                          end,
      ReturnType                                                                       init_value,
      const std::function<ReturnType(internal::LZ_BlockedRange<lz_size>, ReturnType)>& fun,
      [[maybe_unused]] const std::function<ReturnType(ReturnType, ReturnType)>&        reduce_fun) {
      // Sequential implementation - process entire range at once
      LZ_BlockedRange<lz_size> range(init, end);
      return fun(range, init_value);
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
  const std::function<std::vector<lz::sequence>(lz::internal::LZ_BlockedRange<lz_size>,
                                                std::vector<lz::sequence>)>&,
  const std::function<std::vector<lz::sequence>(std::vector<lz::sequence>, std::vector<lz::sequence>)>&);
