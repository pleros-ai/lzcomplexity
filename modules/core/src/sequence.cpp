#include <lz/parallel_utils.h>
#include <lz/sequence.h>

namespace lz {
   sequence::sequence(const std::string str) {
      seq = std::vector<char>(str.begin(), str.end());
      setAlphabetSize();
   }

   sequence::sequence(const std::string_view str) {
      seq = std::vector<char>(str.begin(), str.end());
      setAlphabetSize();
   }

   sequence::sequence(const std::vector<char> vec) {
      seq = vec;
      setAlphabetSize();
   }

   sequence& sequence::reverse(void) {
#ifdef __cpp_lib_ranges
      std::ranges::reverse(seq);
#else
      std::reverse(seq.begin(), seq.end());
#endif

      return *this;
   }

   sequence sequence::reverseCopy(void) {
      auto res = seq;

#ifdef __cpp_lib_ranges
      std::ranges::reverse(res);
#else
      std::reverse(res.begin(), res.end());
#endif

      return sequence(res, alphabet_size);
   }

   const sequence sequence::reverseCopy(void) const {
      auto res = seq;

#ifdef __cpp_lib_ranges
      std::ranges::reverse(res);
#else
      std::reverse(res.begin(), res.end());
#endif

      return sequence(res, alphabet_size);
   }

   sequence& sequence::rightShift(lz_uint ls) {
#ifdef __cpp_lib_ranges
      std::ranges::rotate(seq.begin(), seq.begin() + (ls % seq.size()), seq.end());
#else
      std::rotate(seq.begin(), seq.begin() + (ls % seq.size()), seq.end());
#endif

      return *this;
   }

   sequence& sequence::leftShift(lz_uint ls) {
#ifdef __cpp_lib_ranges
      std::ranges::rotate(seq.begin(), seq.begin() + seq.size() - (ls % seq.size()), seq.end());
#else
      std::rotate(seq.begin(), seq.begin() + seq.size() - (ls % seq.size()), seq.end());
#endif

      return *this;
   }

   sequence sequence::Drop(lz_size l) const {
      if (l >= seq.size()) {
         return sequence(std::vector<char>(), alphabet_size);
      }
      return sequence(std::vector<char>(seq.begin() + l, seq.end()), alphabet_size);
   }

   std::pair<sequence, sequence> sequence::Split(lz_size l) const {
      std::vector<char> lhs{seq.begin(), seq.begin() + l};
      std::vector<char> rhs{seq.begin() + l, seq.end()};

      // std::ranges::split(seq, l);

      return std::make_pair(lhs, rhs);
   }

   sequence sequence::Granularity(lz_uint gr) const {
      char temp = 0;
      std::vector<char> ns;
      std::unordered_set<char> alphabet;
      lz_uint count = 0;

      for (auto c: seq) {
         if (count == gr - 1) {
            ns.push_back(temp);
            alphabet.insert(temp);
            temp  = 0;
            count = 0;
         }
         temp += c;
         count++;
      }

      return sequence(ns, alphabet.size());
   }

   sequence sequence::map(std::function<lz_char(lz_char)> fn) {
      sequence transformed_sequence;

      transformed_sequence.seq.resize(seq.size());

      std::transform(seq.begin(), seq.end(), transformed_sequence.seq.begin(), fn);

      transformed_sequence.alphabet_size = alphabet_size;

      return transformed_sequence;
   }

   const sequence sequence::map(std::function<lz_char(lz_char)> fn) const {
      sequence transformed_sequence;

      transformed_sequence.seq.resize(seq.size());

      std::transform(seq.begin(), seq.end(), transformed_sequence.seq.begin(), fn);

      transformed_sequence.alphabet_size = alphabet_size;

      return transformed_sequence;
   }

   sequence map(std::function<lz_char(lz_char)> fn, const sequence& s) {
      sequence transformed_sequence;

      transformed_sequence.seq.resize(s.seq.size());

      std::transform(s.seq.begin(), s.seq.end(), transformed_sequence.seq.begin(), fn);

      transformed_sequence.alphabet_size = s.alphabet_size;

      return transformed_sequence;
   }

   sequence map(const sequence& s, std::function<lz_char(lz_char)> fn) {
      sequence transformed_sequence;

      transformed_sequence.seq.resize(s.seq.size());

      std::transform(s.seq.begin(), s.seq.end(), transformed_sequence.seq.begin(), fn);

      transformed_sequence.alphabet_size = s.alphabet_size;

      return transformed_sequence;
   }

   void Shuffle(sequence& s, lz_uint block_size) {
      static std::random_device rd_seed;
      static std::mt19937       random_engine(rd_seed());

      std::uniform_int_distribution<> dis(0, (s.size() - block_size - 0x01) / block_size);
      lz_uint                         op1 = s.size() + 0x03, op2 = s.size() + 0x03;

      while (op1 > s.size() - block_size - 0x01)  // this goes on until we get a valid index
         op1 = block_size * dis(random_engine);   // the index for the first block

      op2 = block_size * dis(random_engine);
      while (true && s.size() > 10) {            // this goes on until we get a valid index
         op2 = block_size * dis(random_engine);  // the index for the second block
         if ((op2 < op1 || op2 > op1 + block_size) &&
             op2 < s.size() - block_size - 0x01)  // it does not overlap with the previous block and
                                                  // is not to large the block size can not be feed
            break;
      }

      swap(s, op1, op2, block_size);
   }

   sequence Shuffle(const sequence& s, lz_uint block_size, lz_uint times) {
      sequence seq(s);

      for (lz_uint i = 0; i < times; i++)
         Shuffle(seq, block_size);

      return seq;
   }

}  // namespace lz
