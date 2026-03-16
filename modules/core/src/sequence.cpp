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

  void Shuffle(sequence& s, lz_uint block_size) {
    static std::random_device rd_seed;
    static std::mt19937       random_engine(rd_seed());

    const auto seq_size = s.size();
    if (seq_size <= block_size + 1) return;  // Guard against underflow

    const auto                             max_block_idx = (seq_size - block_size - 1) / block_size;
    std::uniform_int_distribution<lz_uint> dis(0, max_block_idx);

    // Find first valid block index
    lz_uint op1 = block_size * dis(random_engine);
    while (op1 > seq_size - block_size - 1) {
      op1 = block_size * dis(random_engine);
    }

    // Find second non-overlapping block index
    if (seq_size <= 10) return;  // Too small for meaningful shuffle

    lz_uint op2 = block_size * dis(random_engine);
    while (true) {
      op2 = block_size * dis(random_engine);
      const bool no_overlap = (op2 < op1 || op2 > op1 + block_size);
      const bool in_bounds = (op2 < seq_size - block_size - 1);
      if (no_overlap && in_bounds) break;
    }

    swap(s, op1, op2, block_size);
  }

  sequence Shuffle(const sequence& s, lz_uint block_size, lz_uint times) {
    sequence result(s);
    for (lz_uint i = 0; i < times; ++i) {
      Shuffle(result, block_size);
    }
    return result;
  }

}  // namespace lz
