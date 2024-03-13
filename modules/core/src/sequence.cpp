#include <lz/parallel_utils.h>
#include <lz/sequence.h>
#include <tbb/concurrent_set.h>

namespace lz {
   sequence::sequence(const std::string str) {
      seq = std::vector<char>(str.begin(), str.end());
      tbb::concurrent_set<char> aph;

      auto fun = [&](auto i) { aph.insert(str[i]); };
      utils::parallel_for(0, str.size(), fun);

      alphabet_size = aph.size();
   }

   sequence::sequence(const std::vector<char> vec) {
      seq = vec;
      tbb::concurrent_set<char> aph;

      auto fun = [&](auto i) { aph.insert(vec[i]); };
      utils::parallel_for(0, vec.size(), fun);

      alphabet_size = aph.size();
   }

}  // namespace lz
