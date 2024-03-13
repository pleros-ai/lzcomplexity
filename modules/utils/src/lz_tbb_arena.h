#include <tbb/blocked_range.h>
#include <tbb/task_arena.h>

namespace lz {
   namespace internal {

      template <typename T>
      class LZ_BlockedRange : public tbb::blocked_range<T> {
         using tbb::blocked_range<T>::blocked_range;
      };

      class LZ_Arena : public tbb::task_arena {};
   }  // namespace internal
}  // namespace lz
