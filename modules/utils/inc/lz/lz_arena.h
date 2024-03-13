#pragma once

#include <memory>

namespace lz {
   namespace internal {
      template <typename T>
      class LZ_BlockedRange;

      class LZ_Arena;

      ////////////////////////////////////////////////////////////////////////////////
      /// Returns the available number of logical cores.
      ///
      ///  - Checks if there is CFS bandwidth control in place (linux, via cgroups,
      ///    assuming standard paths)
      ///  - Otherwise, returns the number of logical cores provided by
      ///    std::thread::hardware_concurrency()
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

      ////////////////////////////////////////////////////////////////////////////////
      /// Factory function returning a shared pointer to the instance of the global
      /// LZArenaWrapper.
      ///
      /// Allows for reinstantiation of the global LZArenaWrapper once all the
      /// references to the previous one are gone and the object destroyed.
      ////////////////////////////////////////////////////////////////////////////////
      std::shared_ptr<lz::utils::LZArenaWrapper> GetGlobalTaskArena(unsigned maxConcurrency = 0);
   }  // namespace utils

}  // namespace lz
