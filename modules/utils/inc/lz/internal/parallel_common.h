#pragma once

// =============================================================================
// parallel_common.h - Backend-agnostic parallel utilities
// =============================================================================
// This header provides common types and utilities used by all parallel backends.
// It does NOT depend on any specific parallel library (TBB, OpenMP, Cilk).
// =============================================================================

#include "../general.h"  // For lz_size and other common types

#if defined (LZ_PARALLEL_TBB)
  #include <tbb/blocked_range.h>
#endif

namespace lz {

   namespace internal {

      // -------------------------------------------------------------------------
      // LZ_BlockedRange - A simple range class compatible with all backends
      // -------------------------------------------------------------------------
      // This is a lightweight alternative to tbb::blocked_range that works with
      // any parallel backend. For TBB, we can still use tbb::blocked_range
      // internally, but this provides a common interface.
      // -------------------------------------------------------------------------
#if defined(LZ_PARALLEL_TBB)
      template<typename T>
      class LZ_BlockedRange : public tbb::blocked_range<T> {
         using tbb::blocked_range<T>::blocked_range;
      };
#else
      template<typename T>
      class LZ_BlockedRange {
       public:
         using value_type = T;
         using size_type  = std::size_t;

         LZ_BlockedRange() : begin_(0), end_(0), grainsize_(1) {}

         LZ_BlockedRange(T begin, T end, size_type grainsize = 1)
           : begin_(begin), end_(end), grainsize_(grainsize) {}

         // Copy constructor for splitting (used by TBB-style reduction)
         LZ_BlockedRange(LZ_BlockedRange& r, [[maybe_unused]] int split_tag)
           : begin_(r.begin_ + (r.end_ - r.begin_) / 2), end_(r.end_), grainsize_(r.grainsize_) {
            r.end_ = begin_;
         }

         T begin() const { return begin_; }
         T end() const { return end_; }

         size_type size() const { return static_cast<size_type>(end_ - begin_); }
         size_type grainsize() const { return grainsize_; }

         bool empty() const { return begin_ >= end_; }
         bool is_divisible() const { return size() > grainsize_; }

       private:
         T         begin_;
         T         end_;
         size_type grainsize_;
      };
#endif

      // -------------------------------------------------------------------------
      // CPU_Bandwidth - Get the number of available CPU cores
      // -------------------------------------------------------------------------
      int CPU_Bandwidth();

      // -------------------------------------------------------------------------
      // Split tag for range splitting (TBB compatibility)
      // -------------------------------------------------------------------------
      struct split {};

   }  // namespace internal

}  // namespace lz
