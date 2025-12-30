/***************************************************************************

    _    ____  ____ __           lempelziv.h  -  description
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
 ***************************************************************************/

#pragma once

#include "lz76.h"
#include "structures.h"

/**
 * @file lempelziv.h
 * @brief Public API for Lempel-Ziv complexity analysis.
 *
 * Provides functions for computing LZ76 factorization, entropy density,
 * effective complexity, shuffle-based entropy measures, and information
 * distance between sequences.
 */
namespace lz {

   namespace internal {

      /**
       * @brief Internal data structure for LZ76 computation results.
       */
      struct LZ_Data {
         lz_uint factorization;      ///< LZ76 complexity.
         lz_int half_factorization;  ///< Complexity of half the sequence.
         lz_double factors_stddev;   ///< Standard deviation of factor lengths.
         lz_double epsilon;          ///< Epsilon value for entropy estimation.
      };

      /**
       * @brief Returns default LZ_Args based on sequence properties.
       * @param seq The input sequence.
       * @return Default configuration parameters.
       */
      auto getDefaultArgs(const sequence&) -> utils::LZ_Args;

      /**
       * @brief Computes LZ76 factorization data with default parameters.
       * @param seq The input sequence.
       * @return LZ_Data containing factorization results.
       */
      auto lz76Factorization(const sequence&) -> LZ_Data;

      /**
       * @brief Computes LZ76 factorization data with custom parameters.
       * @param seq The input sequence.
       * @param args Configuration parameters.
       * @return LZ_Data containing factorization results.
       */
      auto lz76Factorization(const sequence&, utils::LZ_Args) -> LZ_Data;

      /**
       * @brief Merges two sequences element-by-element into a new sequence.
       *
       * Creates a merged sequence where each position combines corresponding
       * elements from both input sequences, effectively creating a new alphabet.
       *
       * @param s1 First sequence.
       * @param s2 Second sequence.
       * @return Merged sequence with combined alphabet.
       */
      auto MergeSequences(const sequence& s1, const sequence& s2) -> sequence;

      /**
       * @brief Computes character frequency distribution in a sequence.
       * @param seq The input sequence.
       * @return Map of characters to their frequencies.
       */
      auto CheckCharDensity(const sequence&) -> std::map<char, lz_double>;

   }  // namespace internal

   /**
    * @brief Computes complete LZ76 analysis of a sequence.
    *
    * Performs full Lempel-Ziv 76 analysis including factorization,
    * entropy density, shuffle complexity, and error estimates.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return LempelZiv structure with all computed measures.
    */
   auto lz76(const sequence&, utils::LZ_Args) -> utils::LempelZiv;

   //*************************************************************************
   //                    LZ76 Factorization
   //*************************************************************************

   /**
    * @brief Computes the LZ76 complexity of a sequence.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The LZ76 complexity (number of factors).
    */
   auto lz76Factorization(const sequence&, utils::LZ_Args) -> lz_uint;

   /**
    * @brief Computes the LZ76 complexity with default parameters.
    * @param text The input sequence.
    * @return The LZ76 complexity (number of factors).
    */
   inline auto lz76Factorization(const sequence& text) -> lz_uint {
      return lz76Factorization(text, internal::getDefaultArgs(text));
   };

   /**
    * @brief Computes LZ76 factors with full result details.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return LZ_Result containing complexity, epsilon, and factor boundaries.
    */
   auto lz76Factors(const sequence&, utils::LZ_Args) -> internal::LZ_Result;

   /**
    * @brief Computes LZ76 factors with default parameters.
    * @param text The input sequence.
    * @return LZ_Result containing complexity, epsilon, and factor boundaries.
    */
   inline auto lz76Factors(const sequence& text) -> internal::LZ_Result {
      return lz76Factors(text, internal::getDefaultArgs(text));
   };

   //*************************************************************************
   //                    Entropy Density
   //*************************************************************************

   /**
    * @brief Computes the normalized entropy density using LZ76.
    *
    * Estimates the entropy rate of the sequence based on the LZ76
    * factorization complexity.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The entropy density value.
    */
   auto lz76EntropyDensity(const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes entropy density with default parameters.
    * @param text The input sequence.
    * @return The entropy density value.
    */
   inline auto lz76EntropyDensity(const sequence& text) -> lz_double {
      return lz76EntropyDensity(text, internal::getDefaultArgs(text));
   };

   //*************************************************************************
   //                    Effective Complexity
   //*************************************************************************

   /**
    * @brief Computes LZ76 effective complexity (excess entropy as mutual information).
    *
    * Calculates E = C(X) + C(Y) - C(XY), where X and Y are the two halves
    * of the sequence and XY is the concatenation.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The effective complexity value.
    */
   auto lz76EffectiveComplexity(const sequence&, utils::LZ_Args) -> lz_int;

   /**
    * @brief Computes normalized effective complexity.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The normalized effective complexity value.
    */
   auto lz76EffectiveComplexityNormalized(const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes effective complexity with default parameters.
    * @param text The input sequence.
    * @return The effective complexity value.
    */
   inline auto lz76EffectiveComplexity(const sequence& text) -> lz_int {
      return lz76EffectiveComplexity(text, internal::getDefaultArgs(text));
   };

   /**
    * @brief Computes normalized effective complexity with default parameters.
    * @param text The input sequence.
    * @return The normalized effective complexity value.
    */
   inline auto lz76EffectiveComplexityNormalized(const sequence& text) -> lz_double {
      return lz76EffectiveComplexityNormalized(text, internal::getDefaultArgs(text));
   };

   //*************************************************************************
   //                    Shuffle Factorization
   //*************************************************************************

   /**
    * @brief Computes LZ76 factorization for multiple shuffled versions.
    *
    * Generates shuffled sequences and computes their LZ76 complexities
    * for statistical comparison with the original sequence.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters (including block size).
    * @return Pair of (vector of complexities, sequence length).
    */
   auto ShuffleFactorization(const sequence&, utils::LZ_Args) -> std::pair<std::vector<lz_int>, lz_size>;

   /**
    * @brief Computes shuffle factorization with default parameters.
    * @param text The input sequence.
    * @return Pair of (vector of complexities, sequence length).
    */
   inline auto ShuffleFactorization(const sequence& text) -> std::pair<std::vector<lz_int>, lz_size> {
      return ShuffleFactorization(text, internal::getDefaultArgs(text));
   };

   //*************************************************************************
   //                    Shuffle Complexity
   //*************************************************************************

   /**
    * @brief Computes excess entropy by random shuffling of the full sequence.
    *
    * Measures the difference in complexity between the original sequence
    * and randomly shuffled versions to estimate excess entropy.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return LZ_Shuffle structure with entropy excess results.
    */
   auto lz76RandomShuffleComplexity(const sequence&, utils::LZ_Args) -> utils::LZ_Shuffle;

   /**
    * @brief Computes excess entropy by shuffling the paired (merged) sequence.
    *
    * Uses the merged sequence Z (interleaving of two halves) for
    * shuffle-based entropy estimation.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return LZ_Shuffle structure with entropy excess results.
    */
   auto lz76PairedShuffleComplexity(const sequence&, utils::LZ_Args) -> utils::LZ_Shuffle;

   /**
    * @brief Sequential (non-parallel) version of random shuffle complexity.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return LZ_Shuffle structure with entropy excess results.
    */
   auto lz76RandomShuffleComplexitySequential(const sequence&, utils::LZ_Args) -> utils::LZ_Shuffle;

   /**
    * @brief Computes random shuffle complexity with default parameters.
    * @param str The input sequence.
    * @return LZ_Shuffle structure with entropy excess results.
    */
   inline auto lz76RandomShuffleComplexity(const sequence& str) -> utils::LZ_Shuffle {
      return lz76RandomShuffleComplexity(str, internal::getDefaultArgs(str));
   };

   /**
    * @brief Computes paired shuffle complexity with default parameters.
    * @param str The input sequence.
    * @return LZ_Shuffle structure with entropy excess results.
    */
   inline auto lz76PairedShuffleComplexity(const sequence& str) -> utils::LZ_Shuffle {
      return lz76PairedShuffleComplexity(str, internal::getDefaultArgs(str));
   };

   /**
    * @brief Calculates entropy from pre-computed shuffle factorizations.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @param original_complexity Complexity of the original sequence.
    * @param shuffle_complexities Vector of shuffle complexities.
    * @param block_size Block size used for shuffling.
    * @return LZ_Shuffle structure with computed entropy values.
    */
   auto ShuffleEntropyCalculation(const sequence&, const utils::LZ_Args, const lz_int, const std::vector<lz_int>, lz_int)
      -> utils::LZ_Shuffle;

   /**
    * @brief Calculates entropy without pre-computed shuffle factorizations.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @param original_complexity Complexity of the original sequence.
    * @param block_size Block size used for shuffling.
    * @return LZ_Shuffle structure with computed entropy values.
    */
   auto ShuffleEntropyCalculation(const sequence&, const utils::LZ_Args, const lz_int, lz_int) -> utils::LZ_Shuffle;

   //*************************************************************************
   //                    Excess Entropy by Distance
   //*************************************************************************

   /**
    * @brief Computes excess entropy using information distance.
    *
    * Calculates E = [1 - d(X,Y)] * max(C(X), C(Y)), where X is the first
    * half and Y is the second half of the sequence.
    *
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The excess entropy value.
    */
   auto lz76ExcessEntropyDistance(const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes excess entropy by distance with default parameters.
    * @param str The input sequence.
    * @return The excess entropy value.
    */
   inline auto lz76ExcessEntropyDistance(const sequence& str) -> lz_double {
      return lz76ExcessEntropyDistance(str, internal::getDefaultArgs(str));
   };

   //*************************************************************************
   //                    Information Distance
   //*************************************************************************

   /**
    * @brief Computes information distance using pre-computed LZ results.
    *
    * Uses the normalized information distance metric based on LZ76 complexity.
    *
    * @param seq1 First sequence.
    * @param res1 Pre-computed LZ result for first sequence.
    * @param seq2 Second sequence.
    * @param res2 Pre-computed LZ result for second sequence.
    * @return The normalized information distance in [0, 1].
    */
   auto lz76InformationDistance(const sequence&, const internal::LZ_Result&, const sequence&, const internal::LZ_Result&)
      -> lz_double;

   /**
    * @brief Computes information distance between two sequences.
    * @param seq1 First sequence.
    * @param seq2 Second sequence.
    * @param args Configuration parameters.
    * @return The normalized information distance in [0, 1].
    */
   auto lz76InformationDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes information distance with default parameters.
    * @param T1 First sequence.
    * @param T2 Second sequence.
    * @return The normalized information distance in [0, 1].
    */
   inline auto lz76InformationDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return lz76InformationDistance(T1, T2, internal::getDefaultArgs(T1));
   };

   //*************************************************************************
   //                    Shuffle-based Distance (Concatenated)
   //*************************************************************************

   /**
    * @brief Computes mutual information between two sequences.
    *
    * Uses shuffle-based entropy estimation on the concatenated sequence.
    *
    * @param seq1 First sequence.
    * @param seq2 Second sequence.
    * @param args Configuration parameters.
    * @return The mutual information value.
    */
   auto MutualInformation(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes distance using random shuffling of concatenated sequences.
    * @param seq1 First sequence.
    * @param seq2 Second sequence.
    * @param args Configuration parameters.
    * @return The shuffle-based distance value.
    */
   auto lz76RandomShuffleDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes random shuffle distance with default parameters.
    * @param T1 First sequence.
    * @param T2 Second sequence.
    * @return The shuffle-based distance value.
    */
   inline auto lz76RandomShuffleDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return lz76RandomShuffleDistance(T1, T2, internal::getDefaultArgs(T1));
   };

   //*************************************************************************
   //                    Shuffle-based Distance (Merged Z)
   //*************************************************************************

   /**
    * @brief Computes mutual information using the merged (Z) sequence.
    *
    * Uses the element-wise merged sequence for entropy estimation.
    *
    * @param seq1 First sequence.
    * @param seq2 Second sequence.
    * @param args Configuration parameters.
    * @return The mutual information value.
    */
   auto MutualInformationZ(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes distance using random shuffling of the merged (Z) sequence.
    * @param seq1 First sequence.
    * @param seq2 Second sequence.
    * @param args Configuration parameters.
    * @return The shuffle-based distance value.
    */
   auto lz76RandomShuffleDistanceZ(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes random shuffle distance Z with default parameters.
    * @param T1 First sequence.
    * @param T2 Second sequence.
    * @return The shuffle-based distance value.
    */
   inline auto lz76RandomShuffleDistanceZ(const sequence& T1, const sequence& T2) -> lz_double {
      return lz76RandomShuffleDistanceZ(T1, T2, internal::getDefaultArgs(T1));
   };

   //*************************************************************************
   //                    Extra Measures
   //*************************************************************************

   /**
    * @brief Computes additional complexity measures.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return LZ_Extra structure with additional computed values.
    */
   auto lz76ExtraMeasures(const sequence&, utils::LZ_Args) -> utils::LZ_Extra;

   /**
    * @brief Computes extra measures with default parameters.
    * @param seq The input sequence.
    * @return LZ_Extra structure with additional computed values.
    */
   inline auto lz76ExtraMeasures(const sequence& seq) -> utils::LZ_Extra {
      return lz76ExtraMeasures(seq, internal::getDefaultArgs(seq.size()));
   };

   //*************************************************************************
   //                    Error Estimates
   //*************************************************************************

   /**
    * @brief Computes error estimate assuming normal distribution.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The error estimate value.
    */
   auto lz76NormalError(const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes normal error with default parameters.
    * @param seq The input sequence.
    * @return The error estimate value.
    */
   inline auto lz76NormalError(const sequence& seq) -> lz_double {
      return lz76NormalError(seq, internal::getDefaultArgs(seq.size()));
   };

   /**
    * @brief Computes error estimate assuming Poisson distribution.
    * @param seq The input sequence.
    * @param args Configuration parameters.
    * @return The error estimate value.
    */
   auto lz76PoisonError(const sequence&, utils::LZ_Args) -> lz_double;

   /**
    * @brief Computes Poisson error with default parameters.
    * @param seq The input sequence.
    * @return The error estimate value.
    */
   inline auto lz76PoisonError(const sequence& seq) -> lz_double {
      return lz76PoisonError(seq, internal::getDefaultArgs(seq.size()));
   };

}  // namespace lz
