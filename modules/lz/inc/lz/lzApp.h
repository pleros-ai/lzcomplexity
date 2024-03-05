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

#include <concepts>
#include <typeinfo>
#include <vector>
#if __cplusplus >= 201703L
#include <variant>
#endif

#include <lz/lempelziv.h>

#include "flags.h"

namespace lz {
   // using T = std::variant<suffixarray::CaPS_SA, suffixarray::SAIS>;
   //.........................................................................
   // Lempel-Ziv 76 factorization
   //.........................................................................
   auto LempelZivFactorization(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Entropy density
   //.........................................................................
   auto EntropyDensity(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Excess entropy as mutual information: E = (C(X) + C(Y) - C(XY))
   //.........................................................................
   auto ExcessEntropyMi(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Here comes the excess entropy by shuffling.
   //.........................................................................
   auto ExcessEntropyShuffle(utils::LZ_Flags&, utils::LZ_Output&) -> utils::LZ_ExcessInfo;
   auto ExcessEntropyShuffleSequential(utils::LZ_Flags&, utils::LZ_Output&) -> utils::LZ_ExcessInfo;

   //.........................................................................
   // Here comes the excess entropy by distance: E = [1 - d(X,Y)] * max(C(X), C(Y))
   // X -> first half, Y -> second half
   //.........................................................................
   auto ExcessEntropyDistance(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Information distance between two consecutive sequences
   //.........................................................................
   auto InformationDistance(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

   //.........................................................................
   // Information distance into the sequences
   //.........................................................................
   auto InformationDistanceBySequence(utils::LZ_Flags&, utils::LZ_Output&) -> lz_int;

}  // namespace lz