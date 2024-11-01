#include <lzDistance/flags.hpp>

namespace lz {
   namespace dist {

      // void LZ_Output::checkCapacity(lz_size idx) {
      //    if (!data.capacity() || data.capacity() < idx) [[unlikely]] {
      //       std::vector<LempelZiv> new_data(idx);
      //       data.insert(data.end(), new_data.begin(), new_data.end());
      //    }
      // }

      // auto LZ_Output::setComplexity(lz_size idx, lz_uint cpx) -> void {
      //    checkCapacity(idx);
      //    data[idx].complexity = cpx;
      // };

      auto LZ_Output::setDistance(lz_size idx, lz_double dist) -> void {
         info_distance[idx].push_back(dist);
      };

      auto LZ_Output::setDistance(lz_size idx, std::vector<lz_double> distances) -> void {
         info_distance[idx] = distances;
      };

      auto LZ_Output::setDistance(lz_size i, lz_size j, lz_double dist) -> void {
         if (i > info_distance.size() || j > info_distance[i].size()) {
            throw lz::LZOutOfBounds();
         }

         info_distance[i][j] = dist;
      };

      auto LZ_Output::setDistanceShuffle(lz_size idx, lz_double dist) -> void {
         shuffle_distance[idx].push_back(dist);
      };

      auto LZ_Output::setDistanceShuffle(lz_size idx, std::vector<lz_double> distances) -> void {
         shuffle_distance[idx] = distances;
      };

      auto LZ_Output::setDistanceShuffle(lz_size i, lz_size j, lz_double dist) -> void {
         if (i > info_distance.size() || j > info_distance[i].size()) {
            throw lz::LZOutOfBounds();
         }

         info_distance[i][j] = dist;
      };

   }  // namespace dist
}  // namespace lz
