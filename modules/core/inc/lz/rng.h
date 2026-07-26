/**
 * @file rng.h
 * @brief Deterministic RNG for the block shuffle, bit-compatible with the Rust backend.
 *
 * The EMC estimator's value is a function of the surrogates it draws, so the two
 * backends can only report the same number if they draw the same shuffles. This
 * header reproduces, exactly, the generator the Rust backend uses:
 *
 *   - `rand_core` 0.6.4 `SeedableRng::seed_from_u64` -- a PCG32 that expands one
 *     u64 into the 32-byte ChaCha key.
 *   - `rand_chacha` 0.3.1 `ChaCha8Rng` -- ChaCha with 8 rounds (4 double rounds),
 *     a 64-bit counter and a 64-bit stream, buffered four blocks (64 words) at a
 *     time, which is what fixes the word order of the output stream.
 *   - `rand` 0.8.6 `Rng::gen_range(0..=high)` for `usize` -- Lemire's multiply-shift
 *     with the library's own rejection zone `(range << range.leading_zeros()) - 1`.
 *     The rejection zone is *not* the textbook one and rejects far more often, so
 *     getting it wrong silently desynchronises the two backends.
 *
 * Verified against ground-truth vectors dumped from the Rust implementation itself
 * (raw u32 and u64 streams for four seeds, and `gen_range(0..=k)` sequences for
 * twenty-one seed/range pairs), and then end-to-end: 188 of 188 differential
 * comparisons of `lz76RandomShuffleComplexity` between the two backends came back
 * bit-identical in `value`, `multi_information` and every summand.
 *
 * Two warnings. First, do not "optimise" any of this -- every constant and every
 * draw is load-bearing, and a single extra or missing draw silently desynchronises
 * the backends rather than failing loudly. Second, this repository has no test
 * suite, so nothing here is guarded by CI: the parity above was established by
 * hand and can regress unnoticed, including if the Rust side ever bumps its `rand`
 * dependency, since these are that dependency's internal algorithms.
 */

#ifndef LZ_RNG_H
#define LZ_RNG_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lz::rng {

  namespace detail {

    inline constexpr std::uint32_t rotl32(std::uint32_t x, unsigned n) noexcept {
      return static_cast<std::uint32_t>((x << n) | (x >> (32u - n)));
    }

    /// Rotate right, tolerating n == 0 (a 32-bit shift would be undefined).
    inline constexpr std::uint32_t rotr32(std::uint32_t x, unsigned n) noexcept {
      n &= 31u;
      return n == 0u ? x : static_cast<std::uint32_t>((x >> n) | (x << (32u - n)));
    }

    /// Widening 64x64 -> 128 multiply, returning (high, low). Portable: no __int128.
    inline constexpr void wmul64(std::uint64_t a,
                                 std::uint64_t b,
                                 std::uint64_t& hi,
                                 std::uint64_t& lo) noexcept {
      const std::uint64_t a_lo = a & 0xFFFFFFFFull;
      const std::uint64_t a_hi = a >> 32;
      const std::uint64_t b_lo = b & 0xFFFFFFFFull;
      const std::uint64_t b_hi = b >> 32;

      const std::uint64_t p0 = a_lo * b_lo;
      const std::uint64_t p1 = a_lo * b_hi;
      const std::uint64_t p2 = a_hi * b_lo;
      const std::uint64_t p3 = a_hi * b_hi;

      const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFFFFFFull) + (p2 & 0xFFFFFFFFull);
      lo = (mid << 32) | (p0 & 0xFFFFFFFFull);
      hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
    }

    /// Number of leading zero bits in a 64-bit value (64 for zero).
    inline constexpr unsigned leading_zeros64(std::uint64_t x) noexcept {
      if (x == 0ull) return 64u;
      unsigned n = 0u;
      for (int shift = 63; shift >= 0; --shift) {
        if ((x >> static_cast<unsigned>(shift)) & 1ull) break;
        ++n;
      }
      return n;
    }

  }  // namespace detail

  /**
   * @brief ChaCha8, seeded from a single u64, matching rand_chacha 0.3.1 exactly.
   */
  class ChaCha8 {
  public:
    /// Mirrors `ChaCha8Rng::seed_from_u64(seed)`.
    explicit ChaCha8(std::uint64_t seed) noexcept {
      // rand_core's PCG32 seed expansion. Because the u32 outputs are written
      // little-endian and read back little-endian, each output *is* a key word.
      std::uint64_t state = seed;
      for (std::size_t i = 0; i < 8; ++i) {
        state = state * 6364136223846793005ull + 11634580027462260723ull;
        const std::uint64_t s = state;
        const std::uint32_t xorshifted = static_cast<std::uint32_t>(((s >> 18) ^ s) >> 27);
        const std::uint32_t rot = static_cast<std::uint32_t>(s >> 59);
        key_[i] = detail::rotr32(xorshifted, rot);
      }
      index_ = BUF_WORDS;  // force a refill on first use
    }

    std::uint32_t next_u32() noexcept {
      if (index_ >= BUF_WORDS) refill();
      return buf_[index_++];
    }

    /// Mirrors `BlockRng::next_u64`: two consecutive words, low word first.
    std::uint64_t next_u64() noexcept {
      if (index_ >= BUF_WORDS) refill();
      if (index_ + 1 < BUF_WORDS) {
        const std::uint64_t lo = buf_[index_];
        const std::uint64_t hi = buf_[index_ + 1];
        index_ += 2;
        return (hi << 32) | lo;
      }
      // The pair straddles a refill.
      const std::uint64_t lo = buf_[BUF_WORDS - 1];
      refill();
      const std::uint64_t hi = buf_[0];
      index_ = 1;
      return (hi << 32) | lo;
    }

    /**
     * @brief Mirrors `rng.gen_range(0..=high)` for `usize` in rand 0.8.6.
     *
     * The rejection zone is `(range << range.leading_zeros()) - 1`, rand's own
     * "conservative but fast approximation" -- deliberately coarser than the
     * textbook Lemire threshold, so it rejects often. Reproducing it is what keeps
     * the two backends' draw sequences aligned.
     */
    std::uint64_t gen_range_inclusive(std::uint64_t high) noexcept {
      const std::uint64_t range = high + 1ull;  // low is always 0 here
      if (range == 0ull) return next_u64();     // range was 2^64, any value will do
      const std::uint64_t zone = (range << detail::leading_zeros64(range)) - 1ull;
      for (;;) {
        std::uint64_t hi = 0;
        std::uint64_t lo = 0;
        detail::wmul64(next_u64(), range, hi, lo);
        if (lo <= zone) return hi;
      }
    }

  private:
    static constexpr std::size_t BUF_WORDS = 64;  // rand_chacha buffers 4 blocks

    void refill() noexcept {
      static constexpr std::uint32_t CONST[4] = {0x61707865u, 0x3320646Eu, 0x79622D32u, 0x6B206574u};
      for (std::size_t b = 0; b < 4; ++b) {
        const std::uint64_t counter = block_ + b;
        std::uint32_t       s[16];
        s[0] = CONST[0];
        s[1] = CONST[1];
        s[2] = CONST[2];
        s[3] = CONST[3];
        for (std::size_t i = 0; i < 8; ++i) s[4 + i] = key_[i];
        s[12] = static_cast<std::uint32_t>(counter & 0xFFFFFFFFull);
        s[13] = static_cast<std::uint32_t>(counter >> 32);
        s[14] = static_cast<std::uint32_t>(stream_ & 0xFFFFFFFFull);
        s[15] = static_cast<std::uint32_t>(stream_ >> 32);

        std::uint32_t x[16];
        for (std::size_t i = 0; i < 16; ++i) x[i] = s[i];
        for (std::size_t round = 0; round < 4; ++round) {  // 4 double rounds = 8 rounds
          quarter(x, 0, 4, 8, 12);
          quarter(x, 1, 5, 9, 13);
          quarter(x, 2, 6, 10, 14);
          quarter(x, 3, 7, 11, 15);
          quarter(x, 0, 5, 10, 15);
          quarter(x, 1, 6, 11, 12);
          quarter(x, 2, 7, 8, 13);
          quarter(x, 3, 4, 9, 14);
        }
        for (std::size_t i = 0; i < 16; ++i) buf_[b * 16 + i] = x[i] + s[i];
      }
      block_ += 4;
      index_ = 0;
    }

    static void quarter(std::uint32_t* x, std::size_t a, std::size_t b, std::size_t c, std::size_t d) noexcept {
      x[a] += x[b];
      x[d] = detail::rotl32(x[d] ^ x[a], 16);
      x[c] += x[d];
      x[b] = detail::rotl32(x[b] ^ x[c], 12);
      x[a] += x[b];
      x[d] = detail::rotl32(x[d] ^ x[a], 8);
      x[c] += x[d];
      x[b] = detail::rotl32(x[b] ^ x[c], 7);
    }

    std::array<std::uint32_t, 8>          key_{};
    std::array<std::uint32_t, BUF_WORDS>  buf_{};
    std::uint64_t                         block_ = 0;
    std::uint64_t                         stream_ = 0;
    std::size_t                           index_ = BUF_WORDS;
  };

  /// FNV-1a over the sequence bytes -- the content-dependent part of the seed.
  inline std::uint64_t fnv1a(const std::vector<char>& bytes) noexcept {
    std::uint64_t h = 14695981039346656037ull;  // FNV offset basis (64-bit)
    for (const char c : bytes) {
      h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
      h *= 1099511628211ull;
    }
    return h;
  }

  /// Derive the per-block-size seed from the once-computed content hash.
  inline constexpr std::uint64_t seed_from_base(std::uint64_t base, std::uint64_t idx) noexcept {
    return base ^ (idx * 0x9E3779B97F4A7C15ull);
  }

}  // namespace lz::rng

#endif  // LZ_RNG_H
