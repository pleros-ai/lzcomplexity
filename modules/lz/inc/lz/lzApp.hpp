/***************************************************************************

    _    ____  ____ __            lzApp.hpp  -  description
   | |  |_  /_|__  / /          -----------------------------
   | |__ / /___|/ / _ \
   |____/___|  /_/\___/    begin                : 29 Oct 2023
                           email                : efrenaragon96@gmail.com

 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013-2023 by Efren Aragon Perez      						 *
 *   efrenaragon96@gmail.com *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 *
 * @file lzApp.hpp
 * @brief High-level batch processing API for LZ76 complexity analysis.
 *
 * This header provides functions for analyzing multiple sequences using
 * Lempel-Ziv 76 complexity measures. All functions operate on LZ_Flags
 * (input configuration) and LZ_Output (results container) structures,
 * enabling efficient batch processing of sequence collections.
 *
 * @author Efren Aragon Perez
 * @date 29 Oct 2023
 * @copyright Copyright (C) 2013-2023 by Efren Aragon Perez. Licensed under GPL v2+.
 *
 * @see flags.hpp for LZ_Flags and LZ_Output structure definitions.
 * @see lempelziv.h for single-sequence analysis functions.
 */

#pragma once

#include <lz/lempelziv.h>

#include "flags.hpp"

namespace lz {

  /**
   * @brief Performs complete LZ76 analysis on all input sequences.
   *
   * Computes LZ76 factorization, entropy density, and related measures
   * for all sequences in the input flags.
   *
   * @param flags Input configuration containing sequences and algorithm parameters.
   * @param output Output container where results will be stored.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // LZ76 Factorization
  //=========================================================================

  /**
   * @brief Computes LZ76 factorization for all input sequences.
   *
   * The LZ76 complexity c(S) is the number of factors in the minimal
   * factorization where each factor is either a new symbol or the longest
   * substring that appeared earlier in the sequence.
   *
   * @param flags Input configuration containing sequences and algorithm parameters.
   * @param output Output container where complexity values will be stored in `data[i].complexity`.
   * @return 0 on success, non-zero error code on failure.
   *
   * @note (Optional) This function must be called before entropy density or other
   *       measures that depend on the factorization for cache the factorization of
   *       the processed sequences.
   */
  auto lz76Factorization(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Entropy Density
  //=========================================================================

  /**
   * @brief Computes normalized entropy density for all input sequences.
   *
   * The entropy density h is estimated as: h ≈ c(S) · log_k(n) / n,
   * where c(S) is the LZ76 complexity, k is the alphabet size, and n
   * is the sequence length. This converges to the true entropy rate
   * for ergodic sources as n → ∞.
   *
   * @param flags Input configuration containing sequences and algorithm parameters.
   * @param output Output container where entropy values will be stored in `data[i].entropy_density`.
   * @return 0 on success, non-zero error code on failure.
   *
   * @pre lz76Factorization() should be called first, or complexity will be computed automatically.
   */
  auto lz76EntropyDensity(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Excess Entropy by Shuffling
  //=========================================================================

  /**
   * @brief Computes excess entropy using random block shuffling.
   *
   * Estimates the excess entropy by comparing the original sequence complexity
   * with the expected complexity after random permutation of blocks. This
   * quantifies the statistical dependencies in the sequence structure.
   *
   * @param flags Input configuration containing sequences and shuffle parameters.
   * @param output Output container where shuffle complexity will be stored in
   * `data[i].random_shuffle_complexity`.
   * @return 0 on success, non-zero error code on failure.
   *
   * @note Uses parallel processing for efficiency on large sequences.
   * @see lz76RandomShuffleComplexitySequential() for single-threaded version.
   */
  auto lz76RandomShuffleComplexity(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Computes excess entropy using paired shuffle method.
   *
   * Similar to random shuffle complexity but uses a paired shuffling strategy
   * that generates the shuffled sequence from both halves of the original.
   *
   * @param flags Input configuration containing sequences and shuffle parameters.
   * @param output Output container where paired shuffle complexity will be stored.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76PairedShuffleComplexity(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Sequential version of random shuffle complexity computation.
   *
   * Single-threaded implementation for environments where parallel
   * processing is not available or desired.
   *
   * @param flags Input configuration containing sequences and shuffle parameters.
   * @param output Output container where shuffle complexity will be stored.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76RandomShuffleComplexitySequential(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Effective Complexity (Mutual Information)
  //=========================================================================

  /**
   * @brief Computes effective complexity as mutual information between sequence halves.
   *
   * Calculates excess entropy using the formula: E = C(X) + C(Y) - C(XY),
   * where X is the first half, Y is the second half, and XY is the
   * concatenation. This measures the statistical dependence between halves.
   *
   * @param flags Input configuration containing sequences and algorithm parameters.
   * @param output Output container where effective complexity will be stored in `lz_effective_complexity`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76EffectiveComplexity(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Computes excess entropy using normalized distance method.
   *
   * Calculates excess entropy as: E = [1 - d(X,Y)] × max(C(X), C(Y)),
   * where d(X,Y) is the normalized information distance between the
   * first half (X) and second half (Y) of each sequence.
   *
   * @param flags Input configuration containing sequences and algorithm parameters.
   * @param output Output container where excess entropy will be stored in `excess_entropy_dist`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76ExcessEntropyDistance(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Additional Measures
  //=========================================================================

  /**
   * @brief Computes additional information-theoretic measures.
   *
   * Calculates supplementary metrics for each sequence:
   * - Rajski distance between sequence halves
   * - First half uncertainty (conditional entropy)
   * - Second half uncertainty (conditional entropy)
   * - Redundancy measure
   * - Pearson correlation coefficient
   *
   * @param flags Input configuration containing sequences and algorithm parameters.
   * @param output Output container where extra measures will be stored in `extra`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76ExtraMeasures(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Inter-Sequence Distance Measures
  //=========================================================================

  /**
   * @brief Computes normalized information distance between consecutive sequences.
   *
   * For each pair of consecutive sequences (S_i, S_{i+1}), calculates the
   * normalized information distance: d = [C(S_i S_{i+1}) - min(C(S_i), C(S_{i+1}))] / max(C(S_i),
   * C(S_{i+1})).
   *
   * @param flags Input configuration containing sequences (must have at least 2).
   * @param output Output container where distances will be stored in `info_distance`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76InformationDistance(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Computes shuffle-based information distance between consecutive sequences.
   *
   * Similar to lz76InformationDistance() but uses mutual information estimated
   * via random shuffling for more robust distance estimation.
   *
   * @param flags Input configuration containing sequences (must have at least 2).
   * @param output Output container where distances will be stored in `random_shuffle_distance`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76RandomShuffleDistance(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Intra-Sequence Distance Measures
  //=========================================================================

  /**
   * @brief Computes mutual information within each sequence (between halves).
   *
   * For each sequence, calculates the mutual information between its
   * first and second halves: I(X;Y) = C(X) + C(Y) - C(XY).
   *
   * @param flags Input configuration containing sequences.
   * @param output Output container where mutual information will be stored in `mutual_information`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76MutualInformationBySequence(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Computes normalized information distance within each sequence.
   *
   * For each sequence, calculates the normalized distance between its
   * first and second halves.
   *
   * @param flags Input configuration containing sequences.
   * @param output Output container where distances will be stored in `sequence_info_distance`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76InformationDistanceBySequence(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Computes shuffle-based distance within each sequence.
   *
   * For each sequence, calculates the shuffle-based information distance
   * between its first and second halves.
   *
   * @param flags Input configuration containing sequences.
   * @param output Output container where distances will be stored.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76RandomShuffleDistanceBySequence(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Error Estimation
  //=========================================================================

  /**
   * @brief Estimates entropy error assuming normal distribution of factor sizes.
   *
   * Computes the uncertainty in entropy density estimation based on the
   * assumption that LZ76 factor sizes follow a normal distribution.
   *
   * @param flags Input configuration containing sequences.
   * @param output Output container where normal errors will be stored.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76NormalError(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  /**
   * @brief Estimates entropy error assuming Poisson distribution of factor sizes.
   *
   * Computes the uncertainty in entropy density estimation based on the
   * assumption that LZ76 factor sizes follow a Poisson distribution.
   *
   * @param flags Input configuration containing sequences.
   * @param output Output container where Poisson errors will be stored.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76PoisonError(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

  //=========================================================================
  // Mixed Entropy
  //=========================================================================

  /**
   * @brief Computes mixed entropy density for consecutive sequence pairs.
   *
   * For each pair of consecutive sequences (S_i, S_{i+1}), calculates the
   * entropy density of their concatenation. This measures the joint
   * complexity of adjacent sequences.
   *
   * @param flags Input configuration containing sequences (must have at least 2).
   * @param output Output container where mixed entropy will be stored in `mixed_entropy_density`.
   * @return 0 on success, non-zero error code on failure.
   */
  auto lz76MixedEntropyDensity(utils::LZ_Flags& flags, utils::LZ_Output& output) -> lz_int;

}  // namespace lz
