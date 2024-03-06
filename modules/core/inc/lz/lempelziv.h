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
#include <lz/sais_lite.h>
#include <lz/utils.h>

#include <typeinfo>
#include <vector>

#include "lpf.h"
#include "lz76.h"
#include "sequence.h"
#include "structures.h"
#include "types.h"

namespace lz {
   //.........................................................................
   // Lempel-Ziv 76 factorization
   //.........................................................................
   auto LempelZivFactorization(const sequence&) -> lz_int;
   auto LempelZivFactors(const sequence&) -> lz76::LZ_Result;
   auto LempelZivFactorization(const sequence&, utils::LZ_Args) -> lz_int;
   auto LempelZivFactors(const sequence&, utils::LZ_Args) -> lz76::LZ_Result;

   //.........................................................................
   // Entropy density
   //.........................................................................
   auto EntropyDensity(const sequence&) -> lz_double;
   auto EntropyDensity(const sequence&, utils::LZ_Args) -> lz_double;

   //.........................................................................
   // Excess entropy as mutual information: E = (C(X) + C(Y) - C(XY))
   //.........................................................................
   auto ExcessEntropyMi(const sequence&) -> lz_int;
   auto ExcessEntropyMiNormalized(const sequence&) -> lz_double;
   auto ExcessEntropyMi(const sequence&, utils::LZ_Args) -> lz_int;
   auto ExcessEntropyMiNormalized(const sequence&, utils::LZ_Args) -> lz_double;

   //.........................................................................
   // Excess entropy by shuffling.
   //.........................................................................
   auto ExcessEntropyShuffle(const sequence&) -> utils::LZ_ExcessInfo;
   auto ExcessEntropyShuffle(const sequence&, utils::LZ_Args) -> utils::LZ_ExcessInfo;
   auto ExcessEntropyShuffleSequential(const sequence&, utils::LZ_Args) -> utils::LZ_ExcessInfo;

   //.........................................................................
   // Excess entropy by distance: E = [1 - d(X,Y)] * max(C(X), C(Y))
   // X -> first half, Y -> second half
   //.........................................................................
   auto ExcessEntropyDistance(const sequence&) -> lz_double;
   auto ExcessEntropyDistance(const sequence&, utils::LZ_Args) -> lz_double;

   //.........................................................................
   // Information distance between two consecutive sequences
   //.........................................................................
   auto InformationDistance(const sequence&, const sequence&) -> lz_double;
   auto InformationDistance(const sequence&, const lz76::LZ_Result&, const sequence&, const lz76::LZ_Result&)
       -> lz_double;
   auto InformationDistance(const sequence&, const sequence&, utils::LZ_Args) -> lz_double;

}  // namespace lz