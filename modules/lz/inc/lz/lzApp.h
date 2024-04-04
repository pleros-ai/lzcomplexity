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
#include <lz/lempelziv.h>

#include <typeinfo>
#include <vector>

#include "flags.h"

namespace lz {

   auto lz76(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   // using T = std::variant<suffixarray::CaPS_SA, suffixarray::SAIS>;
   //.........................................................................
   // Lempel-Ziv 76 factorization
   //.........................................................................
   auto lz76Factorization(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Entropy density
   //.........................................................................
   auto lz76EntropyDensity(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Here comes the excess entropy by shuffling.
   //.........................................................................
   auto lz76AllRandomShuffleComplexity(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   auto lz76RandomShuffleComplexity(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   auto lz76RandomShuffleComplexitySequential(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Excess entropy as mutual information: E = (C(X) + C(Y) - C(XY))
   //.........................................................................
   auto lz76EffectiveComplexity(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   //.........................................................................
   // Here comes the excess entropy by distance: E = [1 - d(X,Y)] * max(C(X), C(Y))
   // X -> first half, Y -> second half
   //.........................................................................
   auto lz76ExcessEntropyDistance(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Extra measures
   // - rajski distance
   // - first half uncertainty
   // - last half uncertainty
   // - redundancy
   // - pearson coefficient
   //.........................................................................
   auto lz76ExtraMeasures(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Information distance between two consecutive sequences
   //.........................................................................
   auto lz76InformationDistance(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   auto lz76RandomShuffleDistance(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Information distance into the sequences
   //.........................................................................
   auto lz76MutualInformationBySequence(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   auto lz76InformationDistanceBySequence(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   auto lz76RandomShuffleDistanceBySequence(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Errors assuming the the stddev of factors size has normal or poison distribution
   //.........................................................................
   auto lz76NormalError(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;
   auto lz76PoisonError(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

}  // namespace lz