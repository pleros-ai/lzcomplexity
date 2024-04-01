#pragma once

#include <memory>

namespace lz {
   namespace internal {
      template <typename T>
      class LZ_BlockedRange;

      class LZ_Arena;

      ////////////////////////////////////////////////////////////////////////////////
      /// Same idea behind LogicalCPUBandwithControl function of oneTbb
      ////////////////////////////////////////////////////////////////////////////////
      int CPU_Bandwidth();
   }  // namespace internal

   namespace utils {

      // Inspired in the ROOT lib from CERN
      class LZArenaWrapper {
        public:
         ~LZArenaWrapper();                // necessary to set size back to zero
         static unsigned TaskArenaSize();  // A static getter lets us check for LZArenaWrapper's existence
         lz::internal::LZ_Arena &Access();

        private:
         LZArenaWrapper(unsigned maxConcurrency = 0);
         friend std::shared_ptr<lz::utils::LZArenaWrapper> GetGlobalTaskArena(unsigned maxConcurrency);
         std::unique_ptr<lz::internal::LZ_Arena> fTBBArena;
         static unsigned fNWorkers;
      };

      std::shared_ptr<lz::utils::LZArenaWrapper> GetGlobalTaskArena(unsigned maxConcurrency = 0);
   }  // namespace utils

}  // namespace lz
