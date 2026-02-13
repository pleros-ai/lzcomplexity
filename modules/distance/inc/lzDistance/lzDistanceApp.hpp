/**
 * @file lzDistanceApp.hpp
 * @brief LZ-based distance computation functions for sequence comparison.
 *
 * Provides functions to compute pairwise distance matrices between sequences
 * using Lempel-Ziv complexity measures including information distance,
 * shuffle distance, and directed graph representations.
 *
 * @note All functions use parallel computation for optimal performance.
 * @note Thread-safe for concurrent calls with different flag/output instances.
 */

#pragma once

#include <lz/lempelziv.h>

#include "flags.hpp"
#include "lz/general.h"

namespace lz {

  /**
   * @brief Computes the LZ76 information distance matrix between sequences.
   *
   * Calculates pairwise information distances for all sequence pairs in the input.
   * Supports multiple modes: default, binary, DNA/RNA, trajectory, and reverse.
   *
   * @param[in] flags Configuration flags and input sequences.
   * @param[out] lz_dist Output container for the distance matrix (will be resized).
   * @return EXIT_SUCCESS on success, non-zero on failure.
   *
   * @complexity O(n*m) distance computations, parallelized across rows.
   * @note The output matrix dimensions are [first_input.size() x second_input.size()].
   */
  auto lz76DistanceMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) noexcept -> lz_int;

  /**
   * @brief Computes the LZ76 shuffle-based distance matrix between sequences.
   *
   * Similar to lz76DistanceMatrix but uses random shuffle distance metric.
   *
   * @param[in] flags Configuration flags and input sequences.
   * @param[out] lz_dist Output container for the distance matrix (will be resized).
   * @return EXIT_SUCCESS on success, non-zero on failure.
   *
   * @complexity O(n*m) distance computations, parallelized across rows.
   */
  auto lz76ShuffleDistanceMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) noexcept -> lz_int;

  /**
   * @brief Computes the LZ76 directed graph matrix between sequences.
   *
   * For sequences x1 and x2, computes concatenations A=x1+x2 and B=x2+x1.
   * The edge direction is determined by comparing Clz(A) vs Clz(B):
   * - Clz(A) < Clz(B): edge from x2 to x1 (positive weight)
   * - Clz(A) > Clz(B): edge from x1 to x2 (negative weight)
   * - Clz(A) = Clz(B): bidirectional edge (weight = 1)
   *
   * @param[in] flags Configuration flags and input sequences.
   * @param[out] lz_dist Output container for the directed matrix (will be resized).
   * @return EXIT_SUCCESS on success, non-zero on failure.
   *
   * @complexity O(n*m) for asymmetric, O(n*(n-1)/2) for symmetric (same input).
   */
  auto lz76DirectedMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) noexcept -> lz_int;

}  // namespace lz
