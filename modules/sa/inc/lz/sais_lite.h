/***************************************************************************
                          sais_lite.h  -  description
                             -------------------
    begin                : 16 Nov 2013
    last modified        : 26 Oct 2023
    email                : estevez@imre.oc.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013 by Ernesto Estevez Rams   						   *
 *   estevez@imre.oc.uh.cu 	   *
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
 *           http://arxiv.org/abs/1101.3448 * 3) Two Efficient Algorithms for Linear Suffix Array
 * Construction,     * Ge Nong, Sen Zhang and Wai Hong Chan, DCC 2009.                    *
 *                                                                              *
 *                                                                              *
 * ******************************************************************************/

#pragma once

#include <lz/exceptions.h>
#include <lz/general.h>
#include <lz/utils.h>

#include <limits>

#include "sa_structures.h"

#ifndef CHAR_SIZE
#define CHAR_SIZE std::numeric_limits<char>::max()
#endif

#ifndef UCHAR_SIZE
#define UCHAR_SIZE std::numeric_limits<unsigned char>::max()
#endif

/**
 * @file sais_lite.h
 * @brief SA-IS (Suffix Array by Induced Sorting) algorithm implementation.
 *
 * Implements the linear-time suffix array construction algorithm based on
 * induced sorting, along with LCP array computation.
 */
namespace lz {
  namespace suffixarray {

    /**
     * @brief SA-IS suffix array construction algorithm.
     *
     * Implements the Suffix Array by Induced Sorting (SA-IS) algorithm
     * for linear-time suffix array construction. Also computes the
     * Longest Common Prefix (LCP) array.
     *
     * @note This is a sequential algorithm suitable for smaller inputs
     *       or when parallelism is not available. For parallel construction,
     *       consider using CaPS_SA instead.
     */
    class SAIS {
  protected:
      const char* T_;    ///< Pointer to the input text.
      lz_int*     SA_;   ///< Pointer to the suffix array.
      lz_int*     LCP_;  ///< Pointer to the LCP array.
      lz_int      n_;    ///< Length of the input text.

  public:
      /**
       * @brief Default constructor. Creates an empty SAIS object.
       */
      SAIS()
        : SAIS("", 1) {};

      /**
       * @brief Constructs SAIS with input text.
       * @param T Pointer to the input text.
       * @param n Length of the input text.
       */
      SAIS(const char*, lz_int);

      /**
       * @brief Constructs SAIS with pre-allocated arrays.
       * @param T Pointer to the input text.
       * @param SA Pointer to pre-allocated suffix array.
       * @param LCP Pointer to pre-allocated LCP array.
       * @param n Length of the input text.
       */
      SAIS(const char*, lz_int*, lz_int*, lz_int);

      /**
       * @brief Copy constructor.
       * @param other The SAIS object to copy.
       */
      SAIS(const SAIS&);

      /**
       * @brief Move constructor.
       * @param other The SAIS object to move from.
       */
      SAIS(SAIS&&) noexcept;

      /**
       * @brief Destructor. Frees allocated memory.
       */
      ~SAIS();

      /**
       * @brief Reallocates internal arrays.
       *
       * Frees existing SA and LCP arrays and allocates new ones
       * of size n_.
       */
      void refresh() {
        std::free(SA_);
        std::free(LCP_);

        SA_ = static_cast<lz_int*>(std::malloc(n_ * sizeof(lz_int)));
        LCP_ = static_cast<lz_int*>(std::malloc(n_ * sizeof(lz_int)));
      }

      /**
       * @brief Function call operator for string input.
       * @param str The input string.
       * @return LZ_SuffixArray containing the computed SA and LCP.
       */
      auto operator()(std::string str) -> utils::LZ_SuffixArray { return construct(str); }

      /**
       * @brief Function call operator for C-string input.
       * @param str Pointer to the input text.
       * @param n Length of the input text.
       * @return LZ_SuffixArray containing the computed SA and LCP.
       */
      auto operator()(const char* str, lz_int n) -> utils::LZ_SuffixArray { return construct(str, n); }

      /**
       * @brief Copy assignment operator.
       * @param rhs The object to copy from.
       * @return Reference to this object.
       */
      const SAIS& operator=(const SAIS& rhs) {
        if (this != &rhs) {
          this->~SAIS();
          new (this) SAIS(rhs);
        }
        return *this;
      };

      /**
       * @brief Move assignment operator.
       * @param rhs The object to move from.
       * @return Reference to this object.
       */
      const SAIS& operator=(SAIS&& rhs) {
        if (this != &rhs) {
          this->~SAIS();
          new (this) SAIS(rhs);
        }
        return *this;
      };

      /**
       * @brief Swaps the contents of two SAIS objects.
       * @param first First object.
       * @param second Second object.
       */
      friend void swap(SAIS& first, SAIS& second);

      /**
       * @brief Equality comparison based on input text.
       * @return true if both have same length and identical text.
       */
      friend constexpr bool operator==(const SAIS& lhs, const SAIS& rhs) {
        return (lhs.n_ == rhs.n_) && std::equal(lhs.T_, lhs.T_ + lhs.n_, rhs.T_);
      }

      /**
       * @brief Inequality comparison.
       * @return true if objects differ.
       */
      friend constexpr bool operator!=(const SAIS& lhs, const SAIS& rhs) { return !operator==(lhs, rhs); }

      /**
       * @brief Returns the length of the input text.
       * @return The text length.
       */
      constexpr auto n() const { return n_; }

      /**
       * @brief Returns a pointer to the input text.
       * @return Pointer to the text.
       */
      constexpr auto T() const { return T_; }

      /**
       * @brief Returns a pointer to the suffix array.
       * @return Pointer to the SA.
       */
      constexpr auto SA() const { return SA_; }

      /**
       * @brief Returns a pointer to the LCP array.
       * @return Pointer to the LCP array.
       */
      constexpr auto LCP() const { return LCP_; }

      /**
       * @brief Constructs SA and LCP for the stored text.
       * @return LZ_SuffixArray containing the computed arrays.
       */
      utils::LZ_SuffixArray construct();

      /**
       * @brief Constructs SA and LCP for a given string.
       * @param str The input string.
       * @return LZ_SuffixArray containing the computed arrays.
       */
      utils::LZ_SuffixArray construct(const std::string&);

      /**
       * @brief Constructs SA and LCP for a given C-string.
       * @param str Pointer to the input text.
       * @param n Length of the input text.
       * @return LZ_SuffixArray containing the computed arrays.
       */
      utils::LZ_SuffixArray construct(const char*, lz_int);
    };

  }  // namespace suffixarray

  // template struct utils::LZ_FLAGS<suffixarray::SAIS>;
}  // namespace lz
