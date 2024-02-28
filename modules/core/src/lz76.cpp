/***************************************************************************
                          lz76.cpp  -  description
                             -------------------
    begin                : 31 Oct 2023
    email                : efrenaragon96@gmail.com
 ***************************************************************************/

 /***************************************************************************
  *   Copyright (C) 2013-2022 by Efren Aragon Perez   			    	         *
  *   efrenaragon96@gmail.com  										               *
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

  /***************************************************************************
   * Tasks to be done:                                                       *
   *   1) Parallelizing via threads the calculation of the excess entropy.    *
   *                                                                         *
   ***************************************************************************/

#include <lz/lz76.h>

namespace lz {

   namespace lz76 {

      /**
       * @brief
       * Performs the Lempel-Ziv 76 factorization
       * @param _SA Enhanced suffix array structure with LCP values
       * @return the sequence LZ76 complexity.
       */
      LZ_Result LempelZiv76::Factorize(const utils::LZ_SuffixArray _SA) {
         lzf.reserve(_SA.n);
         lz_int* lpf = NULL;
         std::vector<char>::size_type i = 0;

         lzf.clear();

         try {
            lpf = (lz_int*)std::malloc(_SA.n * sizeof(lz_int));
         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }

         try {
            lz::utils::LPF(lpf, (lz_int*)_SA.SA.data(), (lz_int*)_SA.LCP.data(), _SA.n); // Largest prefix factor.

            // Lets build the factorization table
            i = 1; lzf.push_back(0); lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }
         catch (...) {
            std::free(lpf);
            throw LZError();
         }

         // done !
         std::free(lpf);
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
         return LZ_Result{ factorization, lzf };
      }

      LZ_Result LempelZiv76::Factorize(const sequence seq) {
         // parameters should come from flags
         auto max_th = std::thread::hardware_concurrency();
         if(seq.size() < max_th) {
            max_th = 1;
         } else if(seq.size() > 1e6) {
            max_th = 100;
         }
         auto _SA = suffixarray::CaPS_SA(max_th).construct(seq.toString());

         lzf.reserve(_SA.n);
         lz_int* lpf = NULL;
         std::vector<char>::size_type i = 0;

         lzf.clear();

         try {
            lpf = (lz_int*)std::malloc(_SA.n * sizeof(lz_int));
         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }

         try {
            lz::utils::LPF(lpf, (lz_int*)_SA.SA.data(), (lz_int*)_SA.LCP.data(), _SA.n); // Largest prefix factor.

            // Lets build the factorization table
            i = 1; lzf.push_back(0); lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }
         catch (...) {
            std::free(lpf);
            throw LZError();
         }

         // done !
         std::free(lpf);
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
         return LZ_Result{ factorization, lzf };
      }

      LZ_Result LempelZiv76::Factorize(const sequence seq, utils::LZ_Args& sa_args) {
         // parameters should come from flags
         auto _SA = suffixarray::CaPS_SA(sa_args).construct(seq.toString());

         lzf.reserve(_SA.n);
         lz_int* lpf = NULL;
         std::vector<char>::size_type i = 0;

         lzf.clear();

         try {
            lpf = (lz_int*)std::malloc(_SA.n * sizeof(lz_int));
         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }

         try {
            lz::utils::LPF(lpf, (lz_int*)_SA.SA.data(), (lz_int*)_SA.LCP.data(), _SA.n); // Largest prefix factor.

            // Lets build the factorization table
            i = 1; lzf.push_back(0); lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }
         catch (...) {
            std::free(lpf);
            throw LZError();
         }

         // done !
         std::free(lpf);
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
         return LZ_Result{ factorization, lzf };
      }
      
      #if __cplusplus >= 201703L
      template <typename ...SAImpl>
      LZ_Result LempelZiv76::Factorize(const sequence seq, utils::sa_type<SAImpl...>::type sa_impl)  {
         // parameters should come from flags
         utils::LZ_SuffixArray _SA;
         std::visit([&](auto&& alg) { _SA = alg.construct(seq.toString()); }, sa_impl);

         lzf.reserve(_SA.n);
         lz_int* lpf = NULL;
         std::vector<char>::size_type i = 0;

         lzf.clear();

         try {
            lpf = (lz_int*)std::malloc(_SA.n * sizeof(lz_int));
         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }

         try {
            lz::utils::LPF(lpf, (lz_int*)_SA.SA.data(), (lz_int*)_SA.LCP.data(), _SA.n); // Largest prefix factor.

            // Lets build the factorization table
            i = 1; lzf.push_back(0); lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         }
         catch (std::bad_alloc& ba) {
            std::free(lpf);
            throw LZBadAlloc();
         }
         catch (...) {
            std::free(lpf);
            throw LZError();
         }

         // done !
         std::free(lpf);
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
         return LZ_Result{ factorization, lzf };
      }
      #endif
   } // namespace lz76

} // namespace lz
