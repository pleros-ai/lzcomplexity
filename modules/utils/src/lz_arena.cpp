#include <lz/lz_arena.h>
#include <lz/parallel_utils.h>
#include <tbb/task_arena.h>
#define TBB_PREVIEW_GLOBAL_CONTROL 1  // required for TBB versions preceding 2019_U4
#include <tbb/global_control.h>

#include <thread>

#include "lz_tbb_arena.h"

namespace lz {

   namespace internal {

      // To honor cgroup quotas if set: see https://github.com/oneapi-src/oneTBB/issues/190
      int CPU_Bandwidth() {
#ifdef R__LINUX
         // Check for CFS bandwith control
         std::ifstream f("/sys/fs/cgroup/cpuacct/cpu.cfs_quota_us");  // quota file
         if (f) {
            float cfs_quota;
            f >> cfs_quota;
            f.close();
            if (cfs_quota > 0) {
               f.open("/sys/fs/cgroup/cpuacct/cpu.cfs_period_us");  // period file
               float cfs_period;
               f >> cfs_period;
               f.close();
               return static_cast<int>(std::ceil(cfs_quota / cfs_period));
            }
         }
#endif
         return std::thread::hardware_concurrency();
      }
   }  // namespace internal

   namespace utils {

      LZArenaWrapper::LZArenaWrapper(unsigned maxConcurrency)
          : fTBBArena(new internal::LZ_Arena{}) {
         const unsigned tbbDefaultNumberThreads = fTBBArena->max_concurrency();  // not initialized, automatic state
         maxConcurrency =
             maxConcurrency > 0 ? std::min(maxConcurrency, tbbDefaultNumberThreads) : tbbDefaultNumberThreads;
         const unsigned bcCpus = internal::CPU_Bandwidth();
         if (maxConcurrency > bcCpus) {
            std::cout << YELLOW_COLOR << "CPU Bandwith Control Active. Proceeding with " << bcCpus
                      << " threads accordingly" << END_COLOR << std::endl;
            maxConcurrency = bcCpus;
         }
         if (maxConcurrency > tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)) {
            std::cout << YELLOW_COLOR
                      << "tbb::global_control is active, limiting the number of parallel workers from this task arena "
                         "available for execution."
                      << END_COLOR << std::endl;
         }
         fTBBArena->initialize(maxConcurrency);
         fNWorkers = maxConcurrency;
      }

      LZArenaWrapper::~LZArenaWrapper() { fNWorkers = 0u; }

      unsigned LZArenaWrapper::fNWorkers = 0u;

      internal::LZ_Arena& LZArenaWrapper::Access() { return *fTBBArena; }

      unsigned LZArenaWrapper::TaskArenaSize() { return fNWorkers; }

      std::shared_ptr<lz::utils::LZArenaWrapper> GetGlobalTaskArena(unsigned maxConcurrency) {
         static std::weak_ptr<lz::utils::LZArenaWrapper> weak_GTAWrapper;

         static std::mutex m;
         const std::lock_guard<std::mutex> lock{m};
         if (auto sp = weak_GTAWrapper.lock()) {
            if (maxConcurrency && (sp->TaskArenaSize() != maxConcurrency)) {
               std::cout << "There's already an active task arena. Proceeding with the current " << sp->TaskArenaSize()
                         << " threads\n";
            }
            return sp;
         }
         std::shared_ptr<lz::utils::LZArenaWrapper> sp(new lz::utils::LZArenaWrapper(maxConcurrency));
         weak_GTAWrapper = sp;
         return sp;
      }
   }  // namespace utils
}  // namespace lz
