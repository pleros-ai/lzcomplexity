/**
 * @file sequence.cpp
 * @brief Implementation of non-inline sequence methods.
 */

#include <lz/parallel_utils.h>
#include <lz/sequence.h>

namespace lz {

  // ═══════════════════════════════════════════════════════════════════════════════
  // Constructors
  // ═══════════════════════════════════════════════════════════════════════════════

  sequence::sequence(const std::string& str)
    : seq(str.begin(), str.end()), alphabet_size(details::ALPHABET_SIZE) {
    setAlphabetSize();
  }

  sequence::sequence(std::string_view& str)
    : seq(str.begin(), str.end()), alphabet_size(details::ALPHABET_SIZE) {
    setAlphabetSize();
  }

  sequence::sequence(const std::vector<char>& vec)
    : seq(vec), alphabet_size(details::ALPHABET_SIZE) {
    setAlphabetSize();
  }

  sequence::sequence(const std::initializer_list<char>& vec)
    : seq(vec), alphabet_size(details::ALPHABET_SIZE) {
    setAlphabetSize();
  }

  sequence::sequence(const char* vec) {
    int idx = 0;
    while (vec[idx] != '\0') seq.push_back(vec[idx++]);
    setAlphabetSize();
  }

#if __cplusplus >= 202002L || __has_include(<span>)
  sequence::sequence(const std::span<char>& vec)
    : seq({vec.begin(), vec.end()}), alphabet_size(details::ALPHABET_SIZE) {
    setAlphabetSize();
  }
#endif

  sequence& sequence::reverse(void) {
#ifdef __cpp_lib_ranges
    std::ranges::reverse(seq);
#else
    std::reverse(seq.begin(), seq.end());
#endif

    return *this;
  }

  sequence sequence::reverseCopy() {
    // Construct directly from reverse iterators (avoids copy + reverse)
    sequence result;
    result.seq.assign(seq.rbegin(), seq.rend());
    result.alphabet_size = alphabet_size;
    return result;
  }

  sequence sequence::reverseCopy() const {
    sequence result;
    result.seq.assign(seq.rbegin(), seq.rend());
    result.alphabet_size = alphabet_size;
    return result;
  }

  sequence& sequence::rightShift(lz_uint ls) {
    if (seq.empty()) return *this;
    const auto shift = ls % seq.size();
    if (shift == 0) return *this;
#ifdef __cpp_lib_ranges
    std::ranges::rotate(seq.begin(), seq.begin() + static_cast<std::ptrdiff_t>(shift), seq.end());
#else
    std::rotate(seq.begin(), seq.begin() + static_cast<std::ptrdiff_t>(shift), seq.end());
#endif
    return *this;
  }

  sequence& sequence::leftShift(lz_uint ls) {
    if (seq.empty()) return *this;
    const auto shift = seq.size() - (ls % seq.size());
    if (shift == seq.size()) return *this;
#ifdef __cpp_lib_ranges
    std::ranges::rotate(seq.begin(), seq.begin() + static_cast<std::ptrdiff_t>(shift), seq.end());
#else
    std::rotate(seq.begin(), seq.begin() + static_cast<std::ptrdiff_t>(shift), seq.end());
#endif
    return *this;
  }

  // ═══════════════════════════════════════════════════════════════════════════════
  // Subsequence Operations
  // ═══════════════════════════════════════════════════════════════════════════════

  sequence sequence::Drop(lz_size l) const {
    if (l >= seq.size()) [[unlikely]] {
      return sequence("", alphabet_size);
    }
    return sequence(std::vector<char>{seq.begin() + static_cast<std::ptrdiff_t>(l), seq.end()},
                    alphabet_size);
  }

  std::pair<sequence, sequence> sequence::Split(lz_size l) const {
    const auto split_pos = seq.begin() + static_cast<std::ptrdiff_t>(std::min(l, seq.size()));
    return {sequence(std::vector<char>{seq.begin(), split_pos}, alphabet_size),
            sequence(std::vector<char>{split_pos, seq.end()}, alphabet_size)};
  }

  sequence sequence::Granularity(lz_uint gr) const {
    if (gr == 0) return sequence();

    std::string ns;
    ns.reserve(seq.size() / gr);
    std::array<bool, 256> seen{};
    char                  temp = 0;
    lz_uint               count = 0;

    for (const auto c: seq) {
      temp += c;
      if (++count == gr) {
        ns.push_back(temp);
        seen[static_cast<unsigned char>(temp)] = true;
        temp = 0;
        count = 0;
      }
    }

    // Count unique values
    lz_uint unique_count = 0;
    for (const auto s: seen) {
      unique_count += s;
    }

    return sequence(ns, unique_count);
  }

  // ═══════════════════════════════════════════════════════════════════════════════
  // Shuffle Operations
  // ═══════════════════════════════════════════════════════════════════════════════

  void Shuffle(sequence& s, lz_uint block_size, rng::ChaCha8& gen) {
    const lz_size seq_size = s.size();
    const lz_size bs = block_size;
    if (bs == 0 || seq_size <= bs + 1) return;  // Guard against underflow

    const lz_size max_block_idx = (seq_size - bs - 1) / bs;
    if (max_block_idx == 0) return;  // Only one eligible block: nothing to transpose

    // Find the first valid block index.
    lz_size op1 = bs * gen.gen_range_inclusive(max_block_idx);
    while (op1 + bs > seq_size - 1) {
      op1 = bs * gen.gen_range_inclusive(max_block_idx);
    }

    if (seq_size <= 10) return;  // Too small for a meaningful shuffle

    // Find a second block that neither overlaps op1 nor runs off the end. Note the
    // boundaries: op2 == op1 + bs is accepted, and op2 + bs == seq_size - 1 is in
    // bounds. Both match the Rust backend; tightening either desynchronises them.
    for (;;) {
      const lz_size op2 = bs * gen.gen_range_inclusive(max_block_idx);
      const bool    no_overlap = (op2 + bs <= op1) || (op2 >= op1 + bs);
      const bool    in_bounds = (op2 + bs <= seq_size - 1);
      if (no_overlap && in_bounds) {
        swap(s, op1, op2, bs);
        return;
      }
    }
  }

  sequence Shuffle(const sequence& s, lz_uint block_size, lz_uint times, std::uint64_t seed) {
    sequence     result(s);
    rng::ChaCha8 gen(seed);  // one generator across all iterations, as in Rust
    for (lz_uint i = 0; i < times; ++i) {
      Shuffle(result, block_size, gen);
    }
    return result;
  }

  void Shuffle(sequence& s, lz_uint block_size) {
    static std::random_device rd_seed;
    static std::mt19937_64    random_engine(rd_seed());

    rng::ChaCha8 gen(random_engine());
    Shuffle(s, block_size, gen);
  }

  sequence Shuffle(const sequence& s, lz_uint block_size, lz_uint times) {
    static std::random_device rd_seed;
    static std::mt19937_64    random_engine(rd_seed());

    return Shuffle(s, block_size, times, random_engine());
  }

}  // namespace lz
