#pragma once

// =============================================================================
// lz_tbb_arena.h - TBB-specific arena and internal types
// =============================================================================
// This header provides TBB-specific implementations that are only used
// internally by tbb.cpp and lz_arena.cpp.
// =============================================================================

#include <tbb/blocked_range.h>
#include <tbb/task_arena.h>

namespace lz {
  namespace internal {

    class LZ_Arena : public tbb::task_arena {};

  }  // namespace internal
}  // namespace lz
