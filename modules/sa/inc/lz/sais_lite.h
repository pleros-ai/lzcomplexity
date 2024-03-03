/***************************************************************************
                          sais_lite.h  -  description
                             -------------------
    begin                : 16 Nov 2013
    last modified        : 26 Oct 2023
    email                : estevez@imre.oc.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013 by Ernesto Estevez Rams   						   *
 *   estevez@imre.oc.uh.cu   											   *
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

/********************************************************************************
 * Sources:                                                                     *
 *        1) Computing Longest Previous Factor in linear time and applications. *
 *           M. Crochemore and L. Ilie, Inf. Process. Lett., 106(2008), 75-80   *
 * 	   2) "Inducing the LCP-Array". Johannes Fischer,						 *
 *           http://arxiv.org/abs/1101.3448									 *
 *        3) Two Efficient Algorithms for Linear Suffix Array Construction,     *
 *           Ge Nong, Sen Zhang and Wai Hong Chan, DCC 2009.                    *
 *                                                                              *
 *                                                                              *
 * ******************************************************************************/

#pragma once

#include <lz/utils.h>

#include <iostream>
#include <limits>
#include <vector>

#include "sa_structures.h"

#ifndef CHAR_SIZE
#define CHAR_SIZE std::numeric_limits<char>::max()
#endif

#ifndef UCHAR_SIZE
#define UCHAR_SIZE std::numeric_limits<unsigned char>::max()
#endif

namespace lz {
   namespace suffixarray {

      class SAIS {
        protected:
         const char* T_;  // The input text.
         lz_int* SA_;     // The suffix array.
         lz_int* LCP_;    // The LCP array.
         lz_int n_;       // Length of the input text.

        public:
         SAIS()
             : SAIS("", 1){};
         SAIS(const char*, lz_int);
         SAIS(const char*, lz_int*, lz_int*, lz_int);
         // Copy constructs the suffix array object from `other`.
         SAIS(const SAIS&);
         // Move constructs the suffix array object from `other`.
         SAIS(SAIS&&) noexcept;

         ~SAIS();

         void refresh() {
            std::free(SA_);
            std::free(LCP_);

            SA_ = static_cast<lz_int*>(std::malloc(n_ * sizeof(lz_int)));
            LCP_ = static_cast<lz_int*>(std::malloc(n_ * sizeof(lz_int)));
         }

         auto operator()(std::string str) -> utils::LZ_SuffixArray { return construct(str); }
         auto operator()(const char* str, lz_int n) -> utils::LZ_SuffixArray { return construct(str, n); }
         // Copy assignment
         const SAIS& operator=(const SAIS& rhs) {
            if (this != &rhs) {
               this->~SAIS();
               new (this) SAIS(rhs);
            }
            return *this;
         };

         // Move assignment
         const SAIS& operator=(SAIS&& rhs) {
            if (this != &rhs) {
               this->~SAIS();
               new (this) SAIS(rhs);
            }
            return *this;
         };

         friend void swap(SAIS& first, SAIS& second);

         friend constexpr bool operator==(const SAIS& lhs, const SAIS& rhs) {
            return (lhs.n_ == rhs.n_) && std::equal(lhs.T_, lhs.T_ + lhs.n_, rhs.T_);
         }

         friend constexpr bool operator!=(const SAIS& lhs, const SAIS& rhs) { return !operator==(lhs, rhs); }

         // Returns the length of the text.
         constexpr auto n() const { return n_; }

         // Returns the text.
         constexpr auto T() const { return T_; }

         // Returns the suffix array.
         constexpr auto SA() const { return SA_; }

         // Returns the LCP array.
         constexpr auto LCP() const { return LCP_; }
         //*****************************************************
         //               Main funtion
         //*****************************************************
         utils::LZ_SuffixArray construct();
         utils::LZ_SuffixArray construct(const std::string&);
         utils::LZ_SuffixArray construct(const char*, lz_int);
      };

   }  // namespace suffixarray

   // template struct utils::LZ_FLAGS<suffixarray::SAIS>;
}  // namespace lz