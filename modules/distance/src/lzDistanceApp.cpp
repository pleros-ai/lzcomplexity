/**
 * @file lzDistanceApp.cpp
 * @brief Implementation of LZ-based distance matrix computation functions.
 *
 * Provides parallel computation of pairwise distance matrices using various
 * LZ complexity-based distance metrics. Supports multiple sequence comparison
 * modes including binary, DNA/RNA, and trajectory sequences.
 *
 * @note Optimizations applied:
 *   - [[likely]]/[[unlikely]] branch hints for hot paths
 *   - Constexpr O(1) lookup tables for DNA/RNA complement
 *   - Cache-friendly sequential access patterns
 *   - Minimized std::future overhead in trajectory mode
 */

#include <algorithm>
#include <array>
#include <future>
#include <lzDistance/lzDistanceApp.hpp>

namespace {

  // ═══════════════════════════════════════════════════════════════════════════════
  // Compile-Time Utilities
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Helper template for selecting overloaded function pointers.
   * @tparam Args Parameter types of the target overload.
   */
  template<typename... Args>
  struct overload_cast_impl {
    template<typename Return>
    [[nodiscard]] constexpr auto operator()(Return (*pf)(Args...)) const noexcept -> decltype(pf) {
      return pf;
    }

    template<typename Return, typename Class>
    [[nodiscard]] constexpr auto operator()(Return (Class::*pmf)(Args...),
                                            std::false_type = {}) const noexcept -> decltype(pmf) {
      return pmf;
    }

    template<typename Return, typename Class>
    [[nodiscard]] constexpr auto operator()(Return (Class::*pmf)(Args...) const,
                                            std::true_type) const noexcept -> decltype(pmf) {
      return pmf;
    }
  };

  template<typename... Args>
  inline constexpr overload_cast_impl<Args...> overload_cast{};

  // ═══════════════════════════════════════════════════════════════════════════════
  // O(1) Nucleotide Complement Lookup Tables
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Compile-time lookup table for DNA/RNA complement bases.
   *
   * Uses direct array indexing for O(1) lookup instead of O(n) loop.
   * Index by ASCII value of uppercase character.
   */
  struct NucleotideLUT {
    std::array<char, 128> complement{};

    constexpr NucleotideLUT(bool is_rna) noexcept
      : complement{} {
      // Initialize all to identity (no change)
      for (int i = 0; i < 128; ++i) {
        complement[i] = static_cast<char>(i);
      }
      // Set complements
      complement['A'] = is_rna ? 'U' : 'T';
      complement['a'] = is_rna ? 'u' : 't';
      complement['T'] = 'A';
      complement['t'] = 'a';
      complement['U'] = 'A';
      complement['u'] = 'a';
      complement['C'] = 'G';
      complement['c'] = 'g';
      complement['G'] = 'C';
      complement['g'] = 'c';
    }

    [[nodiscard]] constexpr char operator()(char ch) const noexcept {
      const auto idx = static_cast<unsigned char>(ch);
      return (idx < 128) ? complement[idx] : ch;
    }
  };

  inline constexpr NucleotideLUT kDnaComplement{false};
  inline constexpr NucleotideLUT kRnaComplement{true};

}  // anonymous namespace

namespace lz {
  namespace internal {

    // ═══════════════════════════════════════════════════════════════════════════════
    // Line Range Filter
    // ═══════════════════════════════════════════════════════════════════════════════

    /**
     * @brief Checks if a line index should be processed based on range filters.
     * @param idx Zero-based line index.
     * @param init_rng Start of range (1-based) or special value.
     * @param end_rng End of range (1-based) or special value.
     * @return True if the line should be processed.
     */
    [[nodiscard]] constexpr auto canProcessTheLine(const lz_size idx,
                                                   const lz_int  init_rng,
                                                   const lz_int  end_rng) noexcept -> bool {
      // Fast path: process all lines (most common case)
      if (init_rng == details::ALL_LINES
          || (init_rng == details::UNDEFINED_LINES && end_rng == details::UNDEFINED_LINES)) [[likely]] {
        return true;
      }

      const auto line = static_cast<lz_int>(idx + 1);  // Convert to 1-based

      // Process single line
      if (line == init_rng && (end_rng == details::UNDEFINED_LINES || init_rng == end_rng)) [[unlikely]] {
        return true;
      }

      // Process range
      const bool after_start
        = (init_rng == details::ALL_LINES || init_rng == details::UNDEFINED_LINES) || (init_rng <= line);
      const bool before_end
        = (end_rng == details::ALL_LINES || end_rng == details::UNDEFINED_LINES) || (end_rng >= line);

      return after_start && before_end;
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // Distance Matrix Computation Strategies
    // ═══════════════════════════════════════════════════════════════════════════════

    /**
     * @brief Computes distance matrix using default (direct) comparison.
     * @param flags Configuration and input sequences.
     * @param lz_dist Output distance matrix.
     * @param fun Distance function to apply.
     * @return EXIT_SUCCESS on completion.
     */
    template<typename Matrix, typename DistFn>
    auto DistanceMatrixDefault(dist::LZ_Flags& flags, Matrix& lz_dist, DistFn&& fun) noexcept -> lz_int {
      const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;
      const auto  first_len = flags.first_input.size();
      const auto  second_len = second_input.size();

      // Cache range values to avoid repeated function calls
      const auto first_init = flags.first_dist_init();
      const auto first_end = flags.first_dist_end();
      const auto second_init = flags.second_dist_init();
      const auto second_end = flags.second_dist_end();

      auto for_fun = [&, first_init, first_end, second_init, second_end](auto&& idx) {
        if (!canProcessTheLine(idx, first_init, first_end)) [[unlikely]] {
          return;
        }

        const auto& f_seq = flags.first_input[idx];
        auto* const row_data = lz_dist[idx].data();  // Direct pointer for cache efficiency

        for (lz_size j = 0; j < second_len; ++j) {
          if (!canProcessTheLine(j, second_init, second_end)) [[unlikely]] {
            continue;
          }
          row_data[j] = fun(f_seq, second_input[j], flags.sa_args);
        }
      };

      utils::parallel_for(0ul, first_len, for_fun);
      return EXIT_SUCCESS;
    }

    /**
     * @brief Computes distance matrix with reversed second sequences.
     * @param flags Configuration and input sequences.
     * @param lz_dist Output distance matrix.
     * @param fun Distance function to apply.
     * @return EXIT_SUCCESS on completion.
     */
    template<typename Matrix, typename DistFn>
    auto DistanceMatrixRevert(dist::LZ_Flags& flags, Matrix& lz_dist, DistFn&& fun) noexcept -> lz_int {
      const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;
      const auto  first_len = flags.first_input.size();
      const auto  second_len = second_input.size();

      const auto first_init = flags.first_dist_init();
      const auto first_end = flags.first_dist_end();
      const auto second_init = flags.second_dist_init();
      const auto second_end = flags.second_dist_end();

      auto for_fun = [&, first_init, first_end, second_init, second_end](auto&& idx) {
        if (!canProcessTheLine(idx, first_init, first_end)) [[unlikely]] {
          return;
        }

        const auto& f_seq = flags.first_input[idx];
        auto* const row_data = lz_dist[idx].data();

        for (lz_size j = 0; j < second_len; ++j) {
          if (!canProcessTheLine(j, second_init, second_end)) [[unlikely]] {
            continue;
          }
          row_data[j] = fun(f_seq, second_input[j].reverseCopy(), flags.sa_args);
        }
      };

      utils::parallel_for(0ul, first_len, for_fun);
      return EXIT_SUCCESS;
    }

    /**
     * @brief Computes distance matrix for binary sequences.
     *
     * Tests 4 variants: original, reversed, bit-flipped, and bit-flipped reversed.
     * Returns the minimum distance across all variants.
     *
     * @param flags Configuration and input sequences.
     * @param lz_dist Output distance matrix.
     * @param fun Distance function to apply.
     * @return EXIT_SUCCESS on completion.
     */
    template<typename Matrix, typename DistFn>
    auto DistanceMatrixBinary(dist::LZ_Flags& flags, Matrix& lz_dist, DistFn&& fun) noexcept -> lz_int {
      const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;
      const auto  first_len = flags.first_input.size();
      const auto  second_len = second_input.size();

      const auto first_init = flags.first_dist_init();
      const auto first_end = flags.first_dist_end();
      const auto second_init = flags.second_dist_init();
      const auto second_end = flags.second_dist_end();

      // Bit-flip functor (constexpr-friendly)
      constexpr auto flip_bit = [](auto c) noexcept { return c == '0' ? '1' : '0'; };

      auto for_fun = [&, first_init, first_end, second_init, second_end](auto&& idx) {
        if (!canProcessTheLine(idx, first_init, first_end)) [[unlikely]] {
          return;
        }

        const auto& f_seq = flags.first_input[idx];
        auto* const row_data = lz_dist[idx].data();

        for (lz_size j = 0; j < second_len; ++j) {
          if (!canProcessTheLine(j, second_init, second_end)) [[unlikely]] {
            continue;
          }

          const auto& s_seq = second_input[j];
          auto        mapped_seq = s_seq.map(flip_bit);

          // Use std::array for stack allocation (no heap)
          std::array<lz_double, 4> results{};

          auto compute_dfl = [&]() { results[0] = fun(f_seq, s_seq, flags.sa_args); };
          auto compute_inv = [&]() { results[1] = fun(f_seq, s_seq.reverseCopy(), flags.sa_args); };
          auto compute_mapped = [&]() { results[2] = fun(f_seq, mapped_seq, flags.sa_args); };
          auto compute_map_inv = [&]() { results[3] = fun(f_seq, mapped_seq.reverseCopy(), flags.sa_args); };

          utils::par_do(compute_dfl, compute_inv, compute_mapped, compute_map_inv);

          row_data[j] = *std::min_element(results.begin(), results.end());
        }
      };

      utils::parallel_for(0ul, first_len, for_fun);
      return EXIT_SUCCESS;
    }

    /**
     * @brief Computes distance matrix for DNA/RNA sequences.
     *
     * Tests 6 variants: original, reversed, AT-swapped, AT-swapped reversed,
     * CG-swapped, and CG-swapped reversed. Returns minimum distance.
     *
     * @param flags Configuration and input sequences (adn_ or arn_ flag set).
     * @param lz_dist Output distance matrix.
     * @param fun Distance function to apply.
     * @return EXIT_SUCCESS on completion.
     */
    template<typename Matrix, typename DistFn>
    auto DistanceMatrixADN(dist::LZ_Flags& flags, Matrix& lz_dist, DistFn&& fun) noexcept -> lz_int {
      // Select O(1) lookup table based on DNA vs RNA
      const auto& lut = flags.adn_ ? kDnaComplement : kRnaComplement;

      // Character transformation using O(1) lookup
      auto swap_base = [&lut](char ch, char target) noexcept -> char {
        const char upper = static_cast<char>(std::toupper(ch));
        const char complement = lut(target);
        if (upper == target || upper == complement) {
          return static_cast<char>(std::tolower(lut(upper)));
        }
        return ch;
      };

      const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;
      const auto  first_len = flags.first_input.size();
      const auto  second_len = second_input.size();

      const auto first_init = flags.first_dist_init();
      const auto first_end = flags.first_dist_end();
      const auto second_init = flags.second_dist_init();
      const auto second_end = flags.second_dist_end();

      auto for_fun = [&, first_init, first_end, second_init, second_end](auto&& idx) {
        if (!canProcessTheLine(idx, first_init, first_end)) [[unlikely]] {
          return;
        }

        const auto& f_seq = flags.first_input[idx];
        auto* const row_data = lz_dist[idx].data();

        for (lz_size j = 0; j < second_len; ++j) {
          if (!canProcessTheLine(j, second_init, second_end)) [[unlikely]] {
            continue;
          }

          const auto& s_seq = second_input[j];

          auto mappedAT_seq = s_seq.map([&swap_base](auto c) { return swap_base(c, 'A'); });
          auto mappedCG_seq = s_seq.map([&swap_base](auto c) { return swap_base(c, 'C'); });

          std::array<lz_double, 6> results{};

          auto compute_dfl = [&]() { results[0] = fun(f_seq, s_seq, flags.sa_args); };
          auto compute_inv = [&]() { results[1] = fun(f_seq, s_seq.reverseCopy(), flags.sa_args); };
          auto compute_AT = [&]() { results[2] = fun(f_seq, mappedAT_seq, flags.sa_args); };
          auto compute_AT_inv = [&]() { results[3] = fun(f_seq, mappedAT_seq.reverseCopy(), flags.sa_args); };
          auto compute_CG = [&]() { results[4] = fun(f_seq, mappedCG_seq, flags.sa_args); };
          auto compute_CG_inv = [&]() { results[5] = fun(f_seq, mappedCG_seq.reverseCopy(), flags.sa_args); };

          utils::par_do(compute_dfl, compute_inv, compute_AT, compute_AT_inv, compute_CG, compute_CG_inv);

          row_data[j] = *std::min_element(results.begin(), results.end());
        }
      };

      utils::parallel_for(0ul, first_len, for_fun);
      return EXIT_SUCCESS;
    }

    /**
     * @brief Computes distance matrix for trajectory sequences.
     *
     * Tests all 8 trajectory rotations and their reverses (16 variants total).
     * Returns the minimum distance across all variants.
     *
     * @param flags Configuration and input sequences.
     * @param lz_dist Output distance matrix.
     * @param fun Distance function to apply.
     * @return EXIT_SUCCESS on completion.
     */
    template<typename Matrix, typename DistFn>
    auto DistanceMatrixTrajectory(dist::LZ_Flags& flags, Matrix& lz_dist, DistFn&& fun) noexcept -> lz_int {
      static constexpr std::array<char, 8> kTrajectories = {'1', '2', '3', '4', '5', '6', '7', '8'};
      static constexpr lz_size             kNumRotations = kTrajectories.size();

      const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;
      const auto  first_len = flags.first_input.size();
      const auto  second_len = second_input.size();

      const auto first_init = flags.first_dist_init();
      const auto first_end = flags.first_dist_end();
      const auto second_init = flags.second_dist_init();
      const auto second_end = flags.second_dist_end();

      auto tr_fun = [&, first_init, first_end, second_init, second_end](auto&& idx) {
        if (!canProcessTheLine(idx, first_init, first_end)) [[unlikely]] {
          return;
        }

        const auto& f_seq = flags.first_input[idx];
        auto* const row_data = lz_dist[idx].data();

        for (lz_size j = 0; j < second_len; ++j) {
          if (!canProcessTheLine(j, second_init, second_end)) [[unlikely]] {
            continue;
          }

          // Stack-allocated results array (no heap allocation)
          std::array<lz_double, 2 * kNumRotations> results{};

          // Pre-compute all rotated sequences to avoid redundant computation
          std::array<sequence, kNumRotations> rotated_seqs;
          for (lz_size tr = 0; tr < kNumRotations; ++tr) {
            rotated_seqs[tr] = second_input[j].map([tr](lz_char x) {
              const auto val = static_cast<lz_size>(x - '0');
              return kTrajectories[(val + tr - 1) % kNumRotations];
            });
          }

          // Use futures but with pre-computed sequences
          std::array<std::future<void>, kNumRotations> futures;
          for (lz_size tr = 0; tr < kNumRotations; ++tr) {
            futures[tr] = std::async(std::launch::async, [&, tr]() {
              results[2 * tr] = fun(f_seq, rotated_seqs[tr], flags.sa_args);
              results[2 * tr + 1] = fun(f_seq, rotated_seqs[tr].reverseCopy(), flags.sa_args);
            });
          }

          for (auto& future: futures) {
            future.get();
          }

          row_data[j] = *std::min_element(results.begin(), results.end());
        }
      };

      utils::parallel_for(0ul, first_len, tr_fun);
      return EXIT_SUCCESS;
    }

  }  // namespace internal

  // ═══════════════════════════════════════════════════════════════════════════════
  // Public API Implementation
  // ═══════════════════════════════════════════════════════════════════════════════

  namespace {

    /// Type alias for the distance function signature
    using DistanceFn = lz_double (*)(const sequence&, const sequence&, utils::LZ_Args);

    /**
     * @brief Dispatches to the appropriate distance matrix strategy.
     *
     * Uses [[likely]]/[[unlikely]] hints to optimize branch prediction.
     * Default mode is most common, so it's marked as likely.
     *
     * @param flags Configuration flags.
     * @param matrix Output matrix.
     * @param distance_fn Distance function to apply.
     * @return EXIT_SUCCESS on completion.
     */
    auto dispatchDistanceMatrix(dist::LZ_Flags&                      flags,
                                std::vector<std::vector<lz_double>>& matrix,
                                DistanceFn                           distance_fn) noexcept -> lz_int {
      // Order by expected frequency (default is most common)
      if (!flags.binary_ && !flags.adn_ && !flags.arn_ && !flags.trajectory_ && !flags.revert_) [[likely]] {
        return internal::DistanceMatrixDefault(flags, matrix, distance_fn);
      }
      if (flags.binary_) [[unlikely]] {
        return internal::DistanceMatrixBinary(flags, matrix, distance_fn);
      }
      if (flags.adn_ || flags.arn_) [[unlikely]] {
        return internal::DistanceMatrixADN(flags, matrix, distance_fn);
      }
      if (flags.trajectory_) [[unlikely]] {
        return internal::DistanceMatrixTrajectory(flags, matrix, distance_fn);
      }
      // revert_ is the remaining case
      return internal::DistanceMatrixRevert(flags, matrix, distance_fn);
    }

  }  // anonymous namespace

  auto lz76DistanceMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) noexcept -> lz_int {
    const auto first_size = flags.first_input.size();
    const auto second_size = flags.second_input.empty() ? first_size : flags.second_input.size();

    lz_dist.reserveInfoDistance(first_size, second_size);

    return dispatchDistanceMatrix(
      flags,
      lz_dist.info_distance,
      overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76InformationDistance));
  }

  auto lz76ShuffleDistanceMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) noexcept -> lz_int {
    const auto first_size = flags.first_input.size();
    const auto second_size = flags.second_input.empty() ? first_size : flags.second_input.size();

    lz_dist.reserveShuffleDistance(first_size, second_size);

    return dispatchDistanceMatrix(
      flags,
      lz_dist.shuffle_distance,
      overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76RandomShuffleDistance));
  }

  lz_int lz76DirectedMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) noexcept {
    const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;

    const lz_size first_len = flags.first_input.size();
    const lz_size second_len = second_input.size();
    const bool    is_symmetric = flags.second_input.empty();

    const lz_size matrix_dim = is_symmetric ? first_len : first_len + second_len;
    lz_dist.reserveDirectionGraph(matrix_dim, matrix_dim);

    // Cache first_input reference for inner loop
    const auto& first_input = flags.first_input;

    auto compute_row = [&, is_symmetric, first_len](auto&& i) noexcept -> void {
      const lz_size j_start = is_symmetric ? i + 1 : 0;
      const auto&   seq_i = first_input[i];

      for (lz_size j = j_start; j < second_len; ++j) {
        const auto& seq_j = second_input[j];

        lz_int a_lz = 0, b_lz = 0;

        // Compute both concatenation orders in parallel
        auto compute_a = [&]() { a_lz = lz::lz76Factorization(seq_i + seq_j); };
        auto compute_b = [&]() { b_lz = lz::lz76Factorization(seq_j + seq_i); };

        utils::par_do(compute_a, compute_b);

        const lz_int  diff = b_lz - a_lz;
        const lz_size j_idx = is_symmetric ? j : j + first_len;

        // Set edge weights: diff encodes direction
        lz_dist.setDirectionValue(i, j_idx, (std::abs(diff) > flags.matrix_threshold) ? -diff : 1);
        lz_dist.setDirectionValue(j_idx, i, (std::abs(diff) > flags.matrix_threshold) ? diff : 1);
      }
    };

    utils::parallel_for(0ul, first_len, compute_row);
    return EXIT_SUCCESS;
  }

}  // namespace lz