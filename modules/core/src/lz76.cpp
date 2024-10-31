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

/**************************************************************************
 *  Tasks to be done:                                                     *
 *   1) Parallelizing via threads the calculation of the excess entropy.  *
 *                                                                        *
 **************************************************************************/

#include <lz/lz76.h>

#include "../../utils/src/lz_tbb_arena.h"

#ifdef _LIBCPP_HAS_PARALLEL_ALGORITHMS
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

namespace lz {

   namespace internal {

      /**
       * @brief
       * Performs the Lempel-Ziv 76 factorization
       * @param _SA Enhanced suffix array structure with LCP values
       * @return the sequence LZ76 complexity.
       */
      LZ_Result LempelZiv76::Factorize(const utils::LZ_SuffixArray _SA) {
         lzf.reserve(_SA.n);
         std::vector<lz_uint>         lpf(_SA.n);
         std::vector<char>::size_type i = 0;

         lzf.clear();

         auto logn = std::log(ALPHABET_SIZE);
         epsilon   = 2 * (1 + std::log(std::log(ALPHABET_SIZE * _SA.SA.size()) / logn) / logn) /
                   (std::log(_SA.SA.size()) / logn);

         try {
            lz::utils::LPF(lpf, std::move(_SA.SA), std::move(_SA.LCP), _SA.n);  // Largest prefix factor.

            // Lets build the factorization table
            i = 1;
            lzf.push_back(0);
            lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         } catch (std::bad_alloc& ba) {
            throw LZBadAlloc();
         } catch (...) {
            throw LZError();
         }

         FoundStddev();
         // done !
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
         return LZ_Result{factorization, epsilon, lzf};
      }

      LZ_Result LempelZiv76::Factorize(const sequence& seq) {
         // parameters should come from flags
         auto max_th = utils::num_workers();
         if (seq.size() < max_th * 10) {
            max_th = 1;
         } else if (seq.size() > 1e6) {
            max_th = 100;
         }

         suffixarray::CaPS_SA alg(max_th);
         auto                 _SA = alg.construct(seq.toString());

         lzf.reserve(_SA.n);
         std::vector<lz_uint>         lpf(_SA.n);
         std::vector<char>::size_type i = 0;

         lzf.clear();

         auto logn = std::log(seq.getAlphabetSize());
         epsilon   = 2 * (1 + std::log(std::log(seq.getAlphabetSize() * seq.size()) / logn) / logn) /
                   (std::log(seq.size()) / logn);

         try {
            // Largest prefix factor.
            lz::utils::LPF(lpf, std::move(_SA.SA), std::move(_SA.LCP), _SA.n);

            // Lets build the factorization table
            i = 1;
            lzf.push_back(0);
            lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         } catch (std::bad_alloc& ba) {
            throw LZBadAlloc();
         } catch (...) {
            throw LZError();
         }

         FoundStddev();
         // done !
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
         return LZ_Result{factorization, epsilon, lzf};
      }

      LZ_Result LempelZiv76::Factorize(const sequence& seq, utils::LZ_Args& sa_args) {
         // parameters should come from flags
         suffixarray::CaPS_SA  alg(sa_args);
         utils::LZ_SuffixArray _SA = alg.construct(seq.toString());

         auto logn = std::log(sa_args.log_base);
         epsilon =
            2 * (1 + std::log(std::log(sa_args.alphabet * seq.size()) / logn) / logn) / (std::log(seq.size()) / logn);

         lzf.reserve(_SA.n);
         std::vector<lz_uint>         lpf(_SA.n);
         std::vector<lz_int>          lpf2(_SA.n);
         std::vector<char>::size_type i = 0;

         lzf.clear();

         try {
            // Largest prefix factor.
            lz::utils::LPF(lpf, _SA.SA, _SA.LCP, _SA.n);

            // lz::utils::LPF_2(seq, _SA.SA, _SA.n, lpf2);
            // lz::utils::LPF_par(lpf, _SA.SA, _SA.LCP, _SA.n);

            // Lets build the factorization table
            i = 1;
            lzf.push_back(0);
            lzf.push_back(1);

            while (i < _SA.n) {
               i = lzf.back() + lpf[i] + 1;
               lzf.push_back(i);
            }

         } catch (std::bad_alloc& ba) {
            throw LZBadAlloc();
         } catch (...) {
            throw LZError();
         }

         // for (auto i: lzf)
         //    std::cout << i << " ";
         // std::cout << std::endl;
         FoundStddev();
         // done !
         factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);

         LZ_Result res;
         res.factorization = factorization;
         res.epsilon       = epsilon;
         res.lzf           = lzf;

         return res;
      }

      // #if defined(__cpp_lib_concepts) && defined(__cpp_lib_variant)
      //       template <typename... SAImpl>
      //       LZ_Result LempelZiv76::Factorize(const sequence seq, utils::sa_type_t<SAImpl...> sa_impl) {
      //          // parameters should come from flags
      //          utils::LZ_SuffixArray _SA;
      //          std::visit([&](auto&& alg) { _SA = alg.construct(seq.toString()); }, sa_impl);

      //          auto logn = std::log(seq.getAlphabetSize());
      //          epsilon = 2 * (1 + std::log(std::log(seq.getAlphabetSize() * seq.size()) / logn) / logn) /
      //                    (std::log(seq.size()) / logn);

      //          lzf.reserve(_SA.n);
      //          lz_int* lpf = NULL;
      //          std::vector<char>::size_type i = 0;

      //          lzf.clear();

      //          try {
      //             lpf = (lz_int*)std::malloc(_SA.n * sizeof(lz_int));
      //          } catch (std::bad_alloc& ba) {
      //             std::free(lpf);
      //             throw LZBadAlloc();
      //          }

      //          try {
      //             lz::utils::LPF(lpf, reinterpret_cast<lz_int*>(_SA.SA.data()),
      //             reinterpret_cast<lz_int*>(_SA.LCP.data()), _SA.n);  // Largest prefix factor.

      //             // Lets build the factorization table
      //             i = 1;
      //             lzf.push_back(0);
      //             lzf.push_back(1);

      //             while (i < _SA.n) {
      //                i = lzf.back() + lpf[i] + 1;
      //                lzf.push_back(i);
      //             }

      //          } catch (std::bad_alloc& ba) {
      //             std::free(lpf);
      //             throw LZBadAlloc();
      //          } catch (...) {
      //             std::free(lpf);
      //             throw LZError();
      //          }

      //          FoundStddev();
      //          // done !
      //          std::free(lpf);
      //          factorization = ((lzf.back() <= _SA.n) ? lzf.size() - 1 : lzf.size() - 2);
      //          return LZ_Result{factorization, epsilon, lzf};
      //       }
      // #endif

      lz_double LempelZiv76::FoundStddev() {
         std::vector<lz_uint> factors_length(lzf.size());
         lz_uint              max_factor_size = 0;

         for (auto i = 1ul; i < lzf.size(); i++) {
            auto size = lzf[i] - lzf[i - 1];
            factors_length.push_back(size);
            if (size > max_factor_size)
               max_factor_size = size;
         }

         // std::vector<lz_uint> f(max_factor_size + 3);

         // for (auto& len: factors_length) {
         //    if (f[len] == 0 || f[len] == std::numeric_limits<lz_uint>::max())
         //       f[len] = 1;
         //    else
         //       f[len] += 1;
         // }

         // lz_double sum  = std::accumulate(PAR factors_length.begin(), factors_length.end(), 0.0);
         lz_double sum  = lzf[lzf.size() - 1] - 1;
         lz_double mean = sum / lzf.size();

         // std::vector<lz_double> diff(factors_length.size());
         // std::transform(PAR lzf.begin(), lzf.end(), diff.begin(), [mean](lz_double x) { return x - mean; });
         // utils::parallel_for(0, diff.size(), [&](auto idx) { diff[idx] = factors_length[idx] - mean; });
         // lz_double sq_sum =
         //     std::transform_reduce(PAR diff.begin(), diff.end(), 0.0, std::plus{}, [](auto val) { return val * val;
         //     });

         auto body = [&factors_length, mean](const auto& rng, auto init) {
            for (auto i = rng.begin(); i < rng.end(); i++) {
               auto diff = factors_length[i] - mean;
               init += diff * diff;
            }
            return init;
         };
         auto      reduce = [](auto a, auto b) { return a + b; };
         lz_double sq_sum = utils::parallel_reduce(0ul, factors_length.size(), 0.0, body, reduce);

         return factors_stddev = std::sqrt(sq_sum / max_factor_size);
      }
   }  // namespace internal

}  // namespace lz
