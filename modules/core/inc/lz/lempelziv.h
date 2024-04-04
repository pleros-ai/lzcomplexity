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

#include <typeinfo>

#include "lz76.h"
#include "structures.h"

namespace lz {

   namespace internal {

      struct LZ_Data {
         lz_uint   factorization;
         lz_int    half_factorization;
         lz_double factors_stddev;
         lz_double epsilon;
      };

      auto getDefaultArgs(const sequence&) -> utils::LZ_Args;

      auto lz76Factorization(const sequence&) -> LZ_Data;
      auto lz76Factorization(const sequence&, utils::LZ_Args) -> LZ_Data;
      auto MergeSequences(sequence s1, sequence s2) -> sequence;
   }  // namespace internal

   auto lz76(const sequence&, utils::LZ_Args) -> utils::LempelZiv;

   //.........................................................................
   // Lempel-Ziv 76 factorization
   //.........................................................................
   auto lz76Factorization(const sequence&, utils::LZ_Args) -> lz_uint;

   auto lz76Factorization(const sequence& text) -> lz_uint {
      return lz76Factorization(text, internal::getDefaultArgs(text));
   };

   auto lz76Factors(const sequence&, utils::LZ_Args) -> internal::LZ_Result;

   auto lz76Factors(const sequence& text) -> internal::LZ_Result {
      return lz76Factors(text, internal::getDefaultArgs(text));
   };

   //.........................................................................
   // Entropy density
   //.........................................................................
   auto lz76EntropyDensity(const sequence&, utils::LZ_Args) -> lz_double;

   auto lz76EntropyDensity(const sequence& text) -> lz_double {
      return lz76EntropyDensity(text, internal::getDefaultArgs(text));
   };

   //.........................................................................
   // LZ effective complexity (Excess entropy as mutual information): E = (C(X) + C(Y) - C(XY))
   //.........................................................................
   // Main functions
   auto lz76EffectiveComplexity(const sequence&, utils::LZ_Args) -> lz_int;
   auto lz76EffectiveComplexityNormalized(const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto lz76EffectiveComplexity(const sequence& text) -> lz_int {
      return lz76EffectiveComplexity(text, internal::getDefaultArgs(text));
   };
   auto lz76EffectiveComplexityNormalized(const sequence& text) -> lz_double {
      return lz76EffectiveComplexityNormalized(text, internal::getDefaultArgs(text));
   };

   //.........................................................................
   // Calculate the lz76 factorization for all the shuffle sequences
   //.........................................................................
   // Main function
   auto ShuffleFactorization(const sequence&, utils::LZ_Args) -> std::pair<std::vector<lz_int>, lz_size>;
   // Variant
   auto ShuffleFactorization(const sequence& text) -> std::pair<std::vector<lz_int>, lz_size> {
      return ShuffleFactorization(text, internal::getDefaultArgs(text));
   };

   //.........................................................................
   // Shuffle entropy deficit (Excess entropy by shuffling.)
   // --> Random Shuffle Complexity use all the sequence
   // --> Z Random Shuffle Complexity use the Z sequence (the merge of both half)
   //.........................................................................
   // Main functions
   auto lz76AllRandomShuffleComplexity(const sequence&, utils::LZ_Args) -> utils::LZ_Shuffle;
   auto lz76RandomShuffleComplexity(const sequence&, utils::LZ_Args) -> utils::LZ_Shuffle;
   auto lz76RandomShuffleComplexitySequential(const sequence&, utils::LZ_Args) -> utils::LZ_Shuffle;
   // Variants
   auto lz76AllRandomShuffleComplexity(const sequence& str) -> utils::LZ_Shuffle {
      return lz76AllRandomShuffleComplexity(str, internal::getDefaultArgs(str));
   };
   auto lz76RandomShuffleComplexity(const sequence& str) -> utils::LZ_Shuffle {
      return lz76RandomShuffleComplexity(str, internal::getDefaultArgs(str));
   };
   //.........................................................................
   auto
      ShuffleEntropyCalculation(const sequence&, const utils::LZ_Args, const lz_int, const std::vector<lz_int>, lz_int)
         -> utils::LZ_Shuffle;

   //.........................................................................
   // Excess entropy by distance: E = [1 - d(X,Y)] * max(C(X), C(Y))
   // X -> first half, Y -> second half
   //.........................................................................
   // Main function
   auto lz76ExcessEntropyDistance(const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto lz76ExcessEntropyDistance(const sequence& str) -> lz_double {
      return lz76ExcessEntropyDistance(str, internal::getDefaultArgs(str));
   };

   //.........................................................................
   // Distance between two sequences using information
   //.........................................................................
   // Main functions
   auto
      lz76InformationDistance(const sequence&, const internal::LZ_Result&, const sequence&, const internal::LZ_Result&)
         -> lz_double;

   auto lz76InformationDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   auto lz76InformationDistanceZ(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto lz76InformationDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return lz76InformationDistance(T1, T2, internal::getDefaultArgs(T1));
   };
   auto lz76InformationDistanceZ(const sequence& T1, const sequence& T2) -> lz_double {
      return lz76InformationDistanceZ(T1, T2, internal::getDefaultArgs(T1));
   };

   //.........................................................................
   // Distance between two sequences using shuffling over merged (Z) sequence
   //.........................................................................
   // Main functions
   auto MutualInformation(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   auto lz76RandomShuffleDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto lz76RandomShuffleDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return lz76RandomShuffleDistance(T1, T2, internal::getDefaultArgs(T1));
   };

   //.........................................................................
   // Extras
   //.........................................................................
   auto lz76ExtraMeasures(const sequence&, utils::LZ_Args) -> utils::LZ_Extra;

   auto lz76ExtraMeasures(const sequence& seq) -> utils::LZ_Extra {
      return lz76ExtraMeasures(seq, internal::getDefaultArgs(seq.size()));
   };

   //.........................................................................
   // Errors
   //.........................................................................
   auto lz76NormalError(const sequence&, utils::LZ_Args) -> lz_double;

   auto lz76NormalError(const sequence& seq) -> lz_double {
      return lz76NormalError(seq, internal::getDefaultArgs(seq.size()));
   };

   auto lz76PoisonError(const sequence&, utils::LZ_Args) -> lz_double;

   auto lz76PoisonError(const sequence& seq) -> lz_double {
      return lz76PoisonError(seq, internal::getDefaultArgs(seq.size()));
   };
}  // namespace lz