#include <lz/flags.h>

namespace lz {
   namespace utils {

      void LZ_Output::checkCapacity(lz_size idx) {
         if (!data.capacity() || data.capacity() < idx) [[unlikely]] {
            std::vector<LempelZiv> new_data{idx};
            data.insert(data.end(), new_data.begin(), new_data.end());
            // data.reserve(10);
         }
      }

      auto LZ_Output::setComplexity(lz_size idx, lz_uint cpx) -> void {
         checkCapacity(idx);
         data[idx].complexity = cpx;
      };

      auto LZ_Output::setEntropyDensity(lz_size idx, lz_double entropy) -> void {
         checkCapacity(idx);
         data[idx].entropy_density = entropy;
      };

      auto LZ_Output::setFactors(lz_size idx, std::vector<lz_uint> lzf) -> void {
         checkCapacity(idx);
         data[idx].factors = lzf;
      };

      auto LZ_Output::setRandomShuffleComplexity(lz_size idx, LZ_Shuffle shuffle) -> void {
         checkCapacity(idx);
         data[idx].random_shuffle_complexity = shuffle;
      };

      auto LZ_Output::setWholeRandomShuffleComplexity(lz_size idx, LZ_Shuffle shuffle) -> void {
         checkCapacity(idx);
         data[idx].whole_random_shuffle_complexity = shuffle;
      };

      auto LZ_Output::setEpsilon(lz_size idx, lz_double eps) -> void {
         checkCapacity(idx);
         data[idx].epsilon = eps;
      };

      auto LZ_Output::setFactorsStddev(lz_size idx, lz_double stddev) -> void {
         checkCapacity(idx);
         data[idx].factors_stddev = stddev;
      };

      auto LZ_Output::setNormalError(lz_size idx, lz_double error) -> void {
         checkCapacity(idx);
         data[idx].lz_normal_errors = error;
      };

      auto LZ_Output::setPoisonError(lz_size idx, lz_double error) -> void {
         checkCapacity(idx);
         data[idx].lz_poison_errors = error;
      };

      auto LZ_Output::setExtras(lz_size idx, LZ_Extra extra) -> void {
         checkCapacity(idx);
         data[idx].extra = extra;
      };

   }  // namespace utils
}  // namespace lz
