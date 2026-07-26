/**
 * @file rng_parity_test.cpp
 * @brief Locks the shuffle RNG to the Rust backend's stream, bit for bit.
 *
 * `emc` is a function of the surrogates it draws, so the two backends only report
 * the same number while they draw the same shuffles. `lz/rng.h` reproduces the
 * generator the Rust backend gets from `rand` 0.8.6 and `rand_chacha` 0.3.1, and
 * the vectors below were dumped from that Rust implementation directly. If this
 * test fails, C++ and Rust `emc` values have silently diverged -- most likely
 * because the Rust side bumped one of those two dependencies, whose *internal*
 * algorithms this header deliberately mirrors.
 *
 * Regenerating the vectors: add a temporary `#[cfg(test)]` block to
 * `crates/lzcomplexity-core/src/sequence.rs` that prints `ChaCha8Rng::seed_from_u64`
 * streams and `gen_range(0..=k)` sequences, then run
 * `cargo test -p lzcomplexity-core rngdump -- --nocapture`.
 */

#include <lz/rng.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

  int failures = 0;

  void check(const std::string& what, const std::string& got, const std::string& want) {
    if (got == want) {
      std::printf("  ok    %s\n", what.c_str());
      return;
    }
    ++failures;
    std::printf("  FAIL  %s\n         got  %s\n         want %s\n", what.c_str(), got.c_str(), want.c_str());
  }

  std::string hex64(std::uint64_t v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%016llx", static_cast<unsigned long long>(v));
    return b;
  }

  std::string hex32(std::uint32_t v) {
    char b[16];
    std::snprintf(b, sizeof(b), "%08x", v);
    return b;
  }

  struct RawCase {
    std::uint64_t seed;
    const char*   want;
  };

  struct RangeCase {
    std::uint64_t seed;
    std::uint64_t high;
    const char*   want;
  };

}  // namespace

int main() {
  std::printf("ChaCha8 u32 stream\n");
  {
    lz::rng::ChaCha8 r(0);
    std::string      got;
    for (int i = 0; i < 24; ++i) {
      if (i) got += " ";
      got += hex32(r.next_u32());
    }
    check("seed 0, 24 words",
          got,
          "a79a3b6c b585f767 bad8c037 7746a55f 81e2a6e6 b2fb0d32 8f9b887c 0f6760a4 32024679 "
          "e10d6667 947eb0bd 8cae14cb 6a2e923c d438539d d2d368ba ef781c7d 7e88e660 cdc4a23a "
          "e6b31e08 277fa208 7f1bcc44 e17653a3 eb1e2f69 c54302f2");
  }

  std::printf("ChaCha8 u64 stream (seed_from_u64 expansion)\n");
  {
    const RawCase cases[] = {
      {0ull,
       "b585f767a79a3b6c 7746a55fbad8c037 b2fb0d3281e2a6e6 0f6760a48f9b887c e10d666732024679 "
       "8cae14cb947eb0bd d438539d6a2e923c ef781c7dd2d368ba cdc4a23a7e88e660 277fa208e6b31e08 "
       "e17653a37f1bcc44 c54302f2eb1e2f69"},
      {1ull,
       "67094cea8ca40db1 149406d8fc0e8e6b 98b82b0336070665 3825a7dc63080d42 489ff253d51bcbe5 "
       "b5ea8d36e9e22058 7609dfe07eff8a17 27143f42b5c5488e dff1a71b0ab16013 b4e550b02c806908 "
       "7bb6bdb2070c4371 93c3fa5ef11049e3"},
      {0x9E3779B97F4A7C15ull,
       "a6ab11812ab1c509 0c4c52a015bd83b9 179e6bd15ba937eb 7dd12826b1a29cc0 e8cb77352de671d8 "
       "e0831b55f639a4e7 427b1bedce1a11af 3697e3101de43bb4 5f7219685477fbad 6cd1c69eae1f5b42 "
       "fded2bd2afa873ad e8a5674c1d263fc1"},
      {14695981039346656037ull,
       "0cab52e47e476df3 15392a2d0d76db90 e8b80fa8a8bfd9fd eafd4b1584c41000 e9cd9110b0dab8f1 "
       "0f09dd9e9e60fce2 fe956aafa7921dbf 38a64a83215509b6 6e9565e26ac4077a a85ae240a1cd96df "
       "c55ad25232358515 658223a0b66c5e32"},
    };
    for (const auto& c : cases) {
      lz::rng::ChaCha8 r(c.seed);
      std::string      got;
      for (int i = 0; i < 12; ++i) {
        if (i) got += " ";
        got += hex64(r.next_u64());
      }
      check("seed " + hex64(c.seed), got, c.want);
    }
  }

  std::printf("gen_range(0..=high) -- rand 0.8.6 rejection zone\n");
  {
    const RangeCase cases[] = {
      {0ull, 1ull, "1 1 0 1 0 1 1 0 1 1 1 1"},
      {0ull, 2ull, "2 1 2 0 2 1 2 2 0 2 2 1"},
      {0ull, 7ull, "0 7 4 7 6 1 7 6 4 6 5 3"},
      {0ull, 10ull, "5 0 9 6 9 10 8 8 7 2 6 9"},
      {0ull, 100ull, "71 47 70 6 55 83 94 81 15 35 77 19"},
      {0ull, 1249ull, "886 582 75 1036 1169 963 655 438 803 237 538 1110"},
      {0ull, 65535ull, "45819 3943 57613 36014 54328 61304 57718 50499 34344 22989 50105 28234"},
      {42ull, 1ull, "1 1 0 0 1 1 1 0 1 0 0 0"},
      {42ull, 2ull, "2 1 0 2 2 0 1 2 2 0 1 0"},
      {42ull, 7ull, "5 3 5 2 1 2 6 6 4 7 1 1"},
      {42ull, 10ull, "7 10 3 1 3 8 2 5 10 6 8 6"},
      {42ull, 100ull, "43 63 29 15 31 81 24 51 91 96 73 18"},
      {42ull, 1249ull, "852 534 784 187 385 964 298 633 1127 1196 741 719"},
      {42ull, 65535ull, "62277 18913 33218 62717 38861 29101 5006 10129 5138 2520 6543 50161"},
      {0x9E3779B97F4A7C15ull, 1ull, "1 0 0 0 1 0 0 1 1 0 1 0"},
      {0x9E3779B97F4A7C15ull, 2ull, "0 0 1 2 2 0 1 1 2 2 0 0"},
      {0x9E3779B97F4A7C15ull, 7ull, "5 0 7 7 2 3 7 5 3 5 6 2"},
      {0x9E3779B97F4A7C15ull, 10ull, "7 0 1 5 10 9 2 4 4 7 2 2"},
      {0x9E3779B97F4A7C15ull, 100ull, "65 9 49 88 26 21 37 100 91 68 21 21"},
      {0x9E3779B97F4A7C15ull, 1249ull, "60 115 614 1096 266 466 531 847 262 261 539 428"},
      {0x9E3779B97F4A7C15ull, 65535ull,
       "42667 3148 6046 32209 59595 57475 17019 24434 65005 59557 44419 13757"},
    };
    for (const auto& c : cases) {
      lz::rng::ChaCha8 r(c.seed);
      std::string      got;
      for (int i = 0; i < 12; ++i) {
        if (i) got += " ";
        got += std::to_string(r.gen_range_inclusive(c.high));
      }
      check("seed " + hex64(c.seed) + " high " + std::to_string(c.high), got, c.want);
    }
  }

  std::printf("seed derivation (FNV-1a of the sequence, mixed with the block index)\n");
  {
    const std::string s = "0110100110010110";
    std::vector<char> v(s.begin(), s.end());
    const auto        base = lz::rng::fnv1a(v);
    check("fnv1a(\"0110100110010110\")", hex64(base), "ff4a9f08ae9bc1c5");
    check("block 1", hex64(lz::rng::seed_from_base(base, 1)), "617de6b1d1d1bdd0");
    check("block 2", hex64(lz::rng::seed_from_base(base, 2)), "c3246c7a500f39ef");
    check("block 3", hex64(lz::rng::seed_from_base(base, 3)), "25ecf224d344b5fa");
    check("block 4", hex64(lz::rng::seed_from_base(base, 4)), "879779ed53b23191");
  }

  std::printf("\n%s\n", failures == 0 ? "RNG parity with the Rust backend holds."
                                      : "RNG PARITY BROKEN -- C++ and Rust emc values have diverged.");
  return failures == 0 ? 0 : 1;
}
