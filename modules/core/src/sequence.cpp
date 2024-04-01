#include <lz/parallel_utils.h>
#include <lz/sequence.h>
// #include <tbb/concurrent_set.h>

namespace lz {
   sequence::sequence(const std::string str) {
      seq = std::vector<char>(str.begin(), str.end());
      // tbb::concurrent_set<char> aph;

      // auto fun = [&](auto i) { aph.insert(str[i]); };
      // utils::parallel_for(0, str.size(), fun);

      DetermineAlphabet();
      alphabet_size = alphabet.size();
   }

   sequence::sequence(const std::vector<char> vec) {
      seq = vec;
      // tbb::concurrent_set<char> aph;

      // auto fun = [&](auto i) { aph.insert(vec[i]); };
      // utils::parallel_for(0, vec.size(), fun);

      DetermineAlphabet();
      alphabet_size = alphabet.size();
   }

}  // namespace lz
