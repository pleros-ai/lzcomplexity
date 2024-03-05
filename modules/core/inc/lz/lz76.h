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
#include <lz/utils.h>

#include <thread>
#include <vector>

#include "lpf.h"
#include "sequence.h"
#include "structures.h"
#include "types.h"

namespace lz {
   //................................................
   //             Lempel-Ziv 76
   //...............................................
   namespace lz76 {

      struct LZ_Result {
         lz_uint factorization;     //!> The factorization (Lempel-Ziv 76 complexity)
         std::vector<lz_uint> lzf;  //!> The factors vector.
      };

      class LempelZiv76 {
        private:
         void FreshStart(void);

        protected:
         lz_uint factorization;     //!> The LZ76 factorization.
         std::vector<lz_uint> lzf;  //!> The factorization vector.

        public:
         LempelZiv76()
             : factorization(0){};                   //!> default constructor.
         LempelZiv76(const utils::LZ_SuffixArray&);  //!> default constructor.
         LempelZiv76(const LempelZiv76&);            //!> copy constructor.
         LempelZiv76(LempelZiv76&&);                 //!> move constructor.

         LZ_Result Factorize(const sequence);  //!> Find the factors and calculate the lz76 complexity using CaPS.
         LZ_Result Factorize(const sequence, utils::LZ_Args&);  //!> Find the factors and calculate the lz76 complexity
                                                                //! using CaPS with the parameters specified.
         LZ_Result Factorize(const utils::LZ_SuffixArray);      //!> Find the factors and calculate the lz76 complexity
                                                                //! using the SA calculated.
#if __cplusplus >= 201703L
         template <typename... SAImpl>
         LZ_Result Factorize(const sequence, utils::sa_type_t<SAImpl...>);  //!> Find the factors and calculate the
                                                                            //! lz76 complexity using a SA algorithm.
#endif

         void Clear();    //!> Clears the field of the object freeing memory.
         ~LempelZiv76();  //!> Destructor.

         constexpr auto getFactorization(void) const;  //!> Returns the LZ76 complexity of the string.
         auto getFactors(void) const;                  //!> Returns the LZ76 factors

         LempelZiv76& operator=(LempelZiv76);
         LempelZiv76& operator=(const LempelZiv76&);

         friend constexpr bool operator==(const LempelZiv76&, const LempelZiv76&);
         friend constexpr bool operator!=(const LempelZiv76&, const LempelZiv76&);

         // void printFactors(std::ostream&, std::string&); //!< pretty print the factors
         friend std::ostream& operator<<(std::ostream&, LempelZiv76&);  //!< the usual io

         friend void swap(LempelZiv76&, LempelZiv76&);
      };

      //................. Constructors  .................................
      inline void LempelZiv76::FreshStart(void) { factorization = 0; }

      inline LempelZiv76::LempelZiv76(const utils::LZ_SuffixArray& SA) {
         FreshStart();
         Factorize(SA);
      }

      inline LempelZiv76::LempelZiv76(const LempelZiv76& lz)
          : factorization(lz.factorization), lzf(lz.lzf) {}

      inline LempelZiv76::LempelZiv76(LempelZiv76&& lz)
          : factorization(lz.factorization), lzf(std::move(lz.lzf)) {}

      /// @brief
      /// Destructor. Frees all allocated memory
      /// @sa LempelZiv76()
      inline LempelZiv76::~LempelZiv76() { LempelZiv76::Clear(); }

      //....................................... End constructors ...........................................

      /// @brief
      /// Frees all allocated memory
      /// @sa ~LempelZiv76()
      inline void LempelZiv76::Clear() { lzf.clear(); }

      /// @brief
      /// Returns the factorization of the sequence
      /// @return The factorization  of the sequence
      inline constexpr auto LempelZiv76::getFactorization(void) const { return factorization; }

      /// @brief
      /// Returns the Lempel-Ziv factors of the sequence
      /// @return The array of Lempel-Ziv factors
      inline auto LempelZiv76::getFactors(void) const { return lzf; }

      //............................ Assignment operator  ..........................

      /// @brief
      /// Copy operator
      /// @param lz the source
      /// @return *this
      inline LempelZiv76& LempelZiv76::operator=(LempelZiv76 lz) {
         if (this != &lz) {
            this->~LempelZiv76();
            new (this) LempelZiv76(lz);
         }
         return *this;
      }

      inline LempelZiv76& LempelZiv76::operator=(const LempelZiv76& lz) {
         if (this != &lz) {
            this->~LempelZiv76();
            new (this) LempelZiv76(lz);
         }
         return *this;
      };

      //............................ Logical operators  ..........................

      /// @brief
      /// logical equality operator. Compares factorizations, not complexities.
      /// @param lz the source
      /// @return *this
      inline constexpr bool operator==(const LempelZiv76& lhs, const LempelZiv76& rhs) { return lhs.lzf == rhs.lzf; }

      /// @brief
      /// logical non-equality operator. Compares factorizations, not complexities.
      /// @param lz the source
      /// @return *this
      inline constexpr bool operator!=(const LempelZiv76& lhs, const LempelZiv76& rhs) { return !(lhs == rhs); }

      /**
       * @brief
       * Swap the contents of two LempelZiv76 objects.
       * @param lhs LempelZiv76 object
       * @param rhs LempelZiv76 object
       */
      inline void swap(LempelZiv76& lhs, LempelZiv76& rhs) {
         std::swap(lhs.factorization, rhs.factorization);
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

   }  // namespace lz76

}  // namespace lz