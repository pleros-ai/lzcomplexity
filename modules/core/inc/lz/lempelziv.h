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

#include <assert.h>
#include <lz/caps.h>
#include <lz/parallel_utils.h>
#include <lz/sais_lite.h>
#include <lz/types.h>

#include <typeinfo>
#include <vector>

#include "lpf.h"
#include "lz76.h"
#include "sequence.h"
#include "structures.h"

namespace lz {

   namespace internal {

      struct LZ_Factors {
         lz_uint factorization;
         lz_int half_factorization;
      };

      auto getDefaultArgs(const sequence&) -> utils::LZ_Args;

      auto LempelZivFactorization(const sequence&) -> LZ_Factors;
      auto LempelZivFactorization(const sequence&, utils::LZ_Args) -> LZ_Factors;
      auto MergeSequences(sequence s1, sequence s2) -> sequence;
   }  // namespace internal
   //.........................................................................
   // Lempel-Ziv 76 factorization
   //.........................................................................
   auto LempelZivFactorization(const sequence&, utils::LZ_Args) -> lz_uint;

   auto LempelZivFactorization(const sequence& text) -> lz_uint {
      return LempelZivFactorization(text, internal::getDefaultArgs(text));
   };
   auto LempelZivFactorization(const std::string& text) -> lz_uint { return LempelZivFactorization(sequence{text}); };
   auto LempelZivFactorization(const std::string& text, utils::LZ_Args args) -> lz_uint {
      return LempelZivFactorization(sequence{text}, args);
   };

   auto LempelZivFactors(const sequence&, utils::LZ_Args) -> lz76::LZ_Result;

   auto LempelZivFactors(const sequence& text) -> lz76::LZ_Result {
      return LempelZivFactors(text, internal::getDefaultArgs(text));
   };
   auto LempelZivFactors(const std::string& text) -> lz76::LZ_Result { return LempelZivFactors(sequence{text}); };
   auto LempelZivFactors(const std::string& text, utils::LZ_Args args) -> lz76::LZ_Result {
      return LempelZivFactors(sequence{text}, args);
   };

   //.........................................................................
   // Entropy density
   //.........................................................................
   auto EntropyDensity(const sequence&, utils::LZ_Args) -> lz_double;

   auto EntropyDensity(const sequence& text) -> lz_double {
      return EntropyDensity(text, internal::getDefaultArgs(text));
   };
   auto EntropyDensity(const std::string& text) -> lz_double { return EntropyDensity(sequence{text}); };
   auto EntropyDensity(const std::string& text, utils::LZ_Args args) -> lz_double {
      return EntropyDensity(sequence{text}, args);
   };

   //.........................................................................
   // LZ effective complexity (Excess entropy as mutual information): E = (C(X) + C(Y) - C(XY))
   //.........................................................................
   // Main functions
   auto LZEffectiveComplexity(const sequence&, utils::LZ_Args) -> lz_int;
   auto LZEffectiveComplexityNormalized(const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto LZEffectiveComplexity(const sequence& text) -> lz_int {
      return LZEffectiveComplexity(text, internal::getDefaultArgs(text));
   };
   auto LZEffectiveComplexity(const std::string& text) -> lz_int { return LZEffectiveComplexity(sequence{text}); };
   auto LZEffectiveComplexityNormalized(const sequence& text) -> lz_double {
      return LZEffectiveComplexityNormalized(text, internal::getDefaultArgs(text));
   };
   auto LZEffectiveComplexityNormalized(const std::string& text) -> lz_double {
      return LZEffectiveComplexityNormalized(sequence{text});
   };

   auto LZEffectiveComplexity(const std::string& text, utils::LZ_Args args) -> lz_int {
      return LZEffectiveComplexity(sequence{text}, args);
   };
   auto LZEffectiveComplexityNormalized(const std::string& text, utils::LZ_Args args) -> lz_double {
      return LZEffectiveComplexityNormalized(sequence{text}, args);
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
   auto WholeRandomShuffleComplexity(const sequence&, utils::LZ_Args) -> utils::LZ_ExcessInfo;
   auto RandomShuffleComplexity(const sequence&, utils::LZ_Args) -> utils::LZ_ExcessInfo;
   auto RandomShuffleComplexitySequential(const sequence&, utils::LZ_Args) -> utils::LZ_ExcessInfo;
   // Variants
   auto WholeRandomShuffleComplexity(const sequence& str) -> utils::LZ_ExcessInfo {
      return WholeRandomShuffleComplexity(str, internal::getDefaultArgs(str));
   };
   auto WholeRandomShuffleComplexity(const std::string& str) -> utils::LZ_ExcessInfo {
      return WholeRandomShuffleComplexity(sequence{str});
   };
   auto WholeRandomShuffleComplexity(const std::string& str, utils::LZ_Args args) -> utils::LZ_ExcessInfo {
      return WholeRandomShuffleComplexity(sequence{str}, args);
   };

   auto RandomShuffleComplexity(const sequence& str) -> utils::LZ_ExcessInfo {
      return RandomShuffleComplexity(str, internal::getDefaultArgs(str));
   };
   auto RandomShuffleComplexity(const std::string& str) -> utils::LZ_ExcessInfo {
      return RandomShuffleComplexity(sequence{str});
   };
   auto RandomShuffleComplexity(const std::string& str, utils::LZ_Args args) -> utils::LZ_ExcessInfo {
      return RandomShuffleComplexity(sequence{str}, args);
   };
   //.........................................................................
   auto ShuffleEntropyCalculation(const sequence&, const lz_int, const std::vector<lz_int>, lz_int, bool)
       -> utils::LZ_ExcessInfo;

   //.........................................................................
   // Excess entropy by distance: E = [1 - d(X,Y)] * max(C(X), C(Y))
   // X -> first half, Y -> second half
   //.........................................................................
   // Main function
   auto ExcessEntropyDistance(const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto ExcessEntropyDistance(const sequence& str) -> lz_double {
      return ExcessEntropyDistance(str, internal::getDefaultArgs(str));
   };
   auto ExcessEntropyDistance(const std::string& str) -> lz_double { return ExcessEntropyDistance(sequence{str}); };
   auto ExcessEntropyDistance(const std::string& str, utils::LZ_Args args) -> lz_double {
      return ExcessEntropyDistance(sequence{str}, args);
   };

   //.........................................................................
   // Distance between two sequences using information
   //.........................................................................
   // Main functions
   auto InformationDistance(const sequence&, const lz76::LZ_Result&, const sequence&, const lz76::LZ_Result&)
       -> lz_double;

   auto InformationDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto InformationDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return InformationDistance(T1, T2, internal::getDefaultArgs(T1));
   };
   auto InformationDistance(const std::string& T1, const sequence& T2) -> lz_double {
      return InformationDistance(sequence{T1}, T2);
   };
   auto InformationDistance(const sequence& T1, const std::string& T2) -> lz_double {
      return InformationDistance(T1, sequence{T2});
   };
   auto InformationDistance(const std::string& T1, const std::string& T2) -> lz_double {
      return InformationDistance(sequence{T1}, sequence{T2});
   };

   auto InformationDistance(const std::string& T1, const sequence& T2, utils::LZ_Args args) -> lz_double {
      return InformationDistance(sequence{T1}, T2, args);
   };
   auto InformationDistance(const sequence& T1, const std::string& T2, utils::LZ_Args args) -> lz_double {
      return InformationDistance(T1, sequence{T2}, args);
   };
   auto InformationDistance(const std::string& T1, const std::string& T2, utils::LZ_Args args) -> lz_double {
      return InformationDistance(sequence{T1}, sequence{T2}, args);
   };

   //.........................................................................
   // Distance between two sequences using shuffling over merged (Z) sequence
   //.........................................................................
   // Main functions
   auto MutualInformation(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   auto RandomShuffleDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;
   // Variants
   auto RandomShuffleDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return RandomShuffleDistance(T1, T2, internal::getDefaultArgs(T1));
   };
   auto RandomShuffleDistance(const std::string& T1, const sequence& T2) -> lz_double {
      return RandomShuffleDistance(sequence{T1}, T2);
   };
   auto RandomShuffleDistance(const sequence& T1, const std::string& T2) -> lz_double {
      return RandomShuffleDistance(T1, sequence{T2});
   };
   auto RandomShuffleDistance(const std::string& T1, const std::string& T2) -> lz_double {
      return RandomShuffleDistance(sequence{T1}, sequence{T2});
   };

   auto RandomShuffleDistance(const std::string& T1, const sequence& T2, utils::LZ_Args args) -> lz_double {
      return RandomShuffleDistance(sequence{T1}, T2, args);
   };
   auto RandomShuffleDistance(const sequence& T1, const std::string& T2, utils::LZ_Args args) -> lz_double {
      return RandomShuffleDistance(T1, sequence{T2}, args);
   };
   auto RandomShuffleDistance(const std::string& T1, const std::string& T2, utils::LZ_Args args) -> lz_double {
      return RandomShuffleDistance(sequence{T1}, sequence{T2}, args);
   };

   //.........................................................................
   // Other distances
   //.........................................................................
   auto LzRajskiDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

   auto LzRajskiDistance(const sequence& T1, const sequence& T2) -> lz_double {
      return LzRajskiDistance(T1, T2, internal::getDefaultArgs(T1));
   };
}  // namespace lz