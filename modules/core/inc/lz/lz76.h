/***************************************************************************

    _    ____  ____ __           lz76.h  -  description
   | |  |_  /_|__  / /          ------------------------
   | |__ / /___|/ / _ \
   |____/___|  /_/\___/    begin                : 31 Oct 2023
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

#include "lpf.h"
#include "sequence.h"
#include "structures.h"

/**
 * @file lz76.h
 * @brief Lempel-Ziv 76 factorization algorithm implementation.
 */
namespace lz {

   namespace internal {

      /**
       * @brief Result structure for LZ76 factorization.
       *
       * Contains the factorization complexity, epsilon value, and the
       * vector of factor starting positions.
       */
      struct LZ_Result {
         lz_uint factorization;        ///< LZ76 complexity (number of factors).
         lz_double epsilon;            ///< Epsilon value for entropy estimation.
         std::vector<lz_uint> lzf;     ///< Factor boundaries (starting index of each factor).
      };

      /**
       * @brief Computes Lempel-Ziv 76 factorization of sequences.
       *
       * Implements the LZ76 algorithm using suffix arrays (CaPS algorithm)
       * for efficient factorization. The LZ76 complexity is the number of
       * distinct factors in the factorization.
       */
      class LempelZiv76 {
     private:
         /**
          * @brief Resets the object to initial state.
          */
         void FreshStart(void);

     protected:
         lz_uint factorization;        ///< The computed LZ76 complexity.
         std::vector<lz_uint> lzf;     ///< Factor boundaries vector.
         lz_double epsilon;            ///< Epsilon value for entropy estimation.
         lz_double factors_stddev;     ///< Standard deviation of factor lengths.

     public:
         /**
          * @brief Default constructor. Initializes factorization to 0.
          */
         LempelZiv76()
           : factorization(0){};

         /**
          * @brief Constructs and computes factorization from a suffix array.
          * @param SA The pre-computed suffix array.
          */
         LempelZiv76(const utils::LZ_SuffixArray&);

         /**
          * @brief Copy constructor.
          * @param lz The LempelZiv76 object to copy.
          */
         LempelZiv76(const LempelZiv76&);

         /**
          * @brief Move constructor.
          * @param lz The LempelZiv76 object to move from.
          */
         LempelZiv76(LempelZiv76&&);

         /**
          * @brief Computes LZ76 factorization using the CaPS algorithm.
          * @param seq The input sequence to factorize.
          * @return LZ_Result containing complexity, epsilon, and factors.
          */
         LZ_Result Factorize(const sequence&);

         /**
          * @brief Computes LZ76 factorization from a raw character array.
          * @param str Pointer to the character array.
          * @param N Length of the array.
          * @return LZ_Result containing complexity, epsilon, and factors.
          */
         LZ_Result Factorize(const char*, int N);

         /**
          * @brief Computes LZ76 factorization with custom parameters.
          * @param seq The input sequence to factorize.
          * @param args Configuration parameters for the algorithm.
          * @return LZ_Result containing complexity, epsilon, and factors.
          */
         LZ_Result Factorize(const sequence&, utils::LZ_Args&);

         /**
          * @brief Computes LZ76 factorization from a pre-computed suffix array.
          * @param SA The suffix array structure.
          * @return LZ_Result containing complexity, epsilon, and factors.
          */
         LZ_Result Factorize(const utils::LZ_SuffixArray);

         /**
          * @brief Computes the standard deviation of factor lengths.
          * @return The standard deviation value.
          */
         lz_double FoundStddev(void);

         /**
          * @brief Clears all data and frees memory.
          */
         void Clear();

         /**
          * @brief Destructor. Frees all allocated memory.
          */
         ~LempelZiv76();

         /**
          * @brief Returns the LZ76 complexity (number of factors).
          * @return The factorization complexity.
          */
         constexpr auto getFactorization(void) const;

         /**
          * @brief Returns the factor boundaries vector.
          * @return Vector of starting indices for each factor.
          */
         auto getFactors(void) const;

         /**
          * @brief Returns the epsilon value for entropy estimation.
          * @return The epsilon value.
          */
         constexpr auto getEpsilon(void) const;

         /**
          * @brief Returns the standard deviation of factor lengths.
          * @return The standard deviation value.
          */
         constexpr auto getStddev(void) const;

         /**
          * @brief Returns an iterator to the beginning of the factors vector.
          * @return Iterator to the first factor.
          */
         auto getFactorsBegin() const { return lzf.begin(); }

         /**
          * @brief Returns an iterator to the end of the factors vector.
          * @return Iterator past the last factor.
          */
         auto getFactorsEnd() const { return lzf.end(); }

         /**
          * @brief Copy assignment operator.
          * @param lz The object to copy from.
          * @return Reference to this object.
          */
         LempelZiv76& operator=(const LempelZiv76&);

         /**
          * @brief Move assignment operator.
          * @param lz The object to move from.
          * @return Reference to this object.
          */
         LempelZiv76& operator=(LempelZiv76&&);

         /**
          * @brief Equality comparison based on factorization.
          * @param lhs First object.
          * @param rhs Second object.
          * @return true if factorizations are identical.
          */
         friend bool operator==(const LempelZiv76&, const LempelZiv76&);

         /**
          * @brief Inequality comparison based on factorization.
          * @param lhs First object.
          * @param rhs Second object.
          * @return true if factorizations differ.
          */
         friend bool operator!=(const LempelZiv76&, const LempelZiv76&);

         /**
          * @brief Outputs the factorization to a stream.
          * @param os The output stream.
          * @param lz The LempelZiv76 object to output.
          * @return Reference to the output stream.
          */
         friend std::ostream& operator<<(std::ostream&, LempelZiv76&);

         /**
          * @brief Swaps the contents of two LempelZiv76 objects.
          * @param lhs First object.
          * @param rhs Second object.
          */
         friend void swap(LempelZiv76&, LempelZiv76&);
      };

      //*****************************************************
      //               Inline implementations
      //*****************************************************

      inline void LempelZiv76::FreshStart(void) {
         factorization = 0;
      }

      inline LempelZiv76::LempelZiv76(const utils::LZ_SuffixArray& SA) {
         FreshStart();
         Factorize(SA);
      }

      inline LempelZiv76::LempelZiv76(const LempelZiv76& lz)
        : factorization(lz.factorization), lzf(lz.lzf), epsilon(lz.epsilon), factors_stddev(lz.factors_stddev) {}

      inline LempelZiv76::LempelZiv76(LempelZiv76&& lz) {
         *this = std::move(lz);
      }

      inline LempelZiv76::~LempelZiv76() {
         LempelZiv76::Clear();
      }

      inline void LempelZiv76::Clear() {
         lzf.clear();
      }

      inline constexpr auto LempelZiv76::getFactorization(void) const {
         return factorization;
      }

      inline auto LempelZiv76::getFactors(void) const {
         return lzf;
      }

      inline constexpr auto LempelZiv76::getEpsilon(void) const {
         return epsilon;
      }

      inline constexpr auto LempelZiv76::getStddev(void) const {
         return factors_stddev;
      }

      inline LempelZiv76& LempelZiv76::operator=(const LempelZiv76& lz) {
         if (*this != lz) {
            this->~LempelZiv76();
            new (this) LempelZiv76(lz);
         }
         return *this;
      };

      inline LempelZiv76& LempelZiv76::operator=(LempelZiv76&& lz) {
         factorization  = std::exchange(lz.factorization, std::numeric_limits<lz_uint>::max());
         epsilon        = std::exchange(lz.epsilon, 0.0);
         factors_stddev = std::exchange(lz.factors_stddev, 0.0);
         lzf            = std::move(lz.lzf);

         return *this;
      };

      inline bool operator==(const LempelZiv76& lhs, const LempelZiv76& rhs) {
         return lhs.lzf == rhs.lzf;
      }

      inline bool operator!=(const LempelZiv76& lhs, const LempelZiv76& rhs) {
         return !(lhs == rhs);
      }

      inline void swap(LempelZiv76& lhs, LempelZiv76& rhs) {
         std::swap(lhs.factorization, rhs.factorization);
         std::swap(lhs.epsilon, rhs.epsilon);
         std::swap(lhs.factors_stddev, rhs.factors_stddev);
         std::swap(lhs.lzf, rhs.lzf);
      }

      inline std::ostream& operator<<(std::ostream& os, LempelZiv76& lzT) {
         os << "LZ76 factorization: " << lzT.getFactorization() << std::endl;
         os << "LZ76 factors: ";
         for (auto& lzf: lzT.getFactors()) {
            std::cout << lzf << " ";
         }
         std::cout << std::endl;

         return os;
      }

   }  // namespace internal

}  // namespace lz