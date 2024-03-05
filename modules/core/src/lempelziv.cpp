/***************************************************************************
                          lempelziv.cpp  -  description
                             -------------------
    begin                : 29 Oct 2023
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

#include <lz/lempelziv.h>

namespace lz {
   //.......................................................................//
   //                            THE BANANA                                 //
   //.......................................................................//

   //-------------------- Factorization functions ----------------------------//
   lz_int LempelZivFactorization(const sequence& text) {
      lz76::LempelZiv76 L;
      L.Factorize(text);
      return L.getFactorization();
   };

   lz76::LZ_Result LempelZivFactors(const sequence& text) {
      lz76::LempelZiv76 L;
      return L.Factorize(text);
   };

   lz_int LempelZivFactorization(const sequence& text, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      L.Factorize(text, args);
      return L.getFactorization();
   };

   lz76::LZ_Result LempelZivFactors(const sequence& text, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      return L.Factorize(text, args);
   };

   //-------------------- Entropy density functions ----------------------------//
   lz_double EntropyDensity(const sequence& text) {
      lz76::LempelZiv76 L;
      L.Factorize(text);
      lz_double div = (text.size() * std::log(text.alphabetSize())) / std::log(text.size());

      return L.getFactorization() / div;
   }

   lz_double EntropyDensity(const sequence& text, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      L.Factorize(text, args);
      lz_double div = text.size() * std::log(text.alphabetSize()) / std::log(text.size());

      return L.getFactorization() / div;
   }

   //-------------------- Excess of entropy functions ----------------------------//
   lz_int ExcessEntropyMi(const sequence& text) {
      std::vector<char>::size_type mid = text.size() / 2;  // the half of the sequence

      lz_int C_, C_fh, C_lh = 0;
      auto [seq_fh, seq_lh] = text.Split(mid);

      auto fh_fun = [&]() { C_fh = LempelZivFactorization(seq_fh); };
      auto lh_fun = [&]() { C_lh = LempelZivFactorization(seq_lh); };
      auto all_fun = [&]() { C_ = LempelZivFactorization(text); };

      utils::par_do(all_fun, fh_fun, lh_fun);
      return C_fh + C_lh - C_;
   };

   lz_double ExcessEntropyMiNormalized(const sequence& text) {
      auto excess = ExcessEntropyMi(text);
      auto div = (text.size() * std::log(text.alphabetSize())) / std::log(text.size());

      return excess / div;
   }

   lz_int ExcessEntropyMi(const sequence& text, utils::LZ_Args args) {
      std::vector<char>::size_type mid = text.size() / 2;  // the half of the sequence

      lz_int C_, C_fh, C_lh = 0;
      auto [seq_fh, seq_lh] = text.Split(mid);

      auto fh_fun = [&]() { C_fh = LempelZivFactorization(seq_fh, args); };
      auto lh_fun = [&]() { C_lh = LempelZivFactorization(seq_lh, args); };
      auto all_fun = [&]() { C_ = LempelZivFactorization(text, args); };

      utils::par_do(all_fun, fh_fun, lh_fun);
      return C_fh + C_lh - C_;
   };

   lz_double ExcessEntropyMiNormalized(const sequence& text, utils::LZ_Args args) {
      auto excess = ExcessEntropyMi(text, args);
      auto div = (text.size() * std::log(text.alphabetSize())) / std::log(text.size());

      return excess / div;
   }

   utils::LZ_ExcessInfo ExcessEntropyShuffle(const sequence& str) {
      utils::LZ_ExcessInfo result;

      std::size_t mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
      result.max_block_size = mm;

      auto complex = LempelZivFactorization(str);
      auto tbb_range = tbb::blocked_range<size_t>(1, mm);
      auto main_fun = [&](const tbb::blocked_range<size_t>& r, lz_double init) -> lz_double {
         for (auto m = r.begin(); m <= r.end(); m++) {
            sequence rand_seq = Shuffle(
                str, m, str.size() / 2);  // Shuffling is made for half the size of the sequence, hope that is enough
            auto rand_complexity = LempelZivFactorization(rand_seq);
            lz_double eeterm = 0;
            eeterm = std::log(str.size()) * std::fabs((lz_double)rand_complexity - (lz_double)complex) /
                     (str.size() * std::log(str.alphabetSize()));

            init += eeterm;

            if (m == 1) result.multi_information = eeterm;
         }
         return init;
      };
      auto reduce_fun = [&](const lz_double& a, const lz_double& b) -> lz_double { return a + b; };

      result.excess_value = tbb::parallel_reduce(tbb_range, 0.0, main_fun, reduce_fun);
      return result;
   }

   utils::LZ_ExcessInfo ExcessEntropyShuffleSequential(const sequence& str, utils::LZ_Args args) {
      std::size_t mm = args.block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
         mm += 10;                                // begin aggressive
      }

      utils::LZ_ExcessInfo result;
      result.max_block_size = mm;
      lz_double excess_entropy = 0;
      auto complex = LempelZivFactorization(str, args);

      for (unsigned int m = 1; m <= mm; m++) {
         sequence rand_seq = Shuffle(
             str, m, str.size() / 2);  // Shuffling is made for half the size of the sequence, hope that is enough
         std::vector<unsigned int> slzf;
         auto rand_complexity = LempelZivFactorization(rand_seq, args);
         lz_double ee_term = std::log(str.size()) * std::fabs((lz_double)rand_complexity - (lz_double)complex) /
                             (str.size() * std::log(str.alphabetSize()));

         excess_entropy += ee_term;

         if (args.excess_line >= 0) result.excess_by_terms.push_back(ee_term);
         if (m == 1) result.multi_information = ee_term;
      }

      result.excess_value = excess_entropy;
      return result;
   }

   utils::LZ_ExcessInfo ExcessEntropyShuffle(const sequence& str, utils::LZ_Args args) {
      std::size_t mm = args.block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
         mm += 10;                                // begin aggressive
      }

      utils::LZ_ExcessInfo result;
      result.max_block_size = mm;

      auto complex = LempelZivFactorization(str, args);

      auto tbb_range = tbb::blocked_range<size_t>(1, mm + 1);
      auto main_fun = [&](const tbb::blocked_range<size_t>& r, lz_double init) -> lz_double {
         for (auto m = r.begin(); m < r.end(); m++) {
            sequence rand_seq = Shuffle(
                str, m, str.size() / 2);  // Shuffling is made for half the size of the sequence, hope that is enough
            auto rand_complexity = LempelZivFactorization(rand_seq, args);
            lz_double eeterm = std::log(str.size()) * std::fabs((lz_double)rand_complexity - (lz_double)complex) /
                               (str.size() * std::log(str.alphabetSize()));
            init += eeterm;

            if (args.excess_line >= 0) result.excess_by_terms.push_back(eeterm);
            if (m == 1) result.multi_information = eeterm;
         }
         return init;
      };
      auto reduce_fun = [](const lz_double& a, const lz_double& b) -> lz_double { return a + b; };

      result.excess_value = tbb::parallel_reduce(tbb_range, 0.0, main_fun, reduce_fun);
      return result;
   }

   lz_double ExcessEntropyDistance(const sequence& str) {
      std::vector<char>::size_type mid = str.size() / 2;  // the half of the sequence

      lz_double dist = 0;
      lz_int fh_complexity = 0;
      lz_int lh_complexity = 0;
      auto [seq_fh, seq_lh] = str.Split(mid);

      auto fh_fun = [&]() { fh_complexity = LempelZivFactorization(seq_fh); };
      auto lh_fun = [&]() { lh_complexity = LempelZivFactorization(seq_lh); };
      auto dist_fun = [&]() { dist = InformationDistance(seq_fh, seq_lh); };

      utils::par_do(dist_fun, fh_fun, lh_fun);

      return (1.0 - dist) * std::fmax(fh_complexity, lh_complexity);
   }

   lz_double ExcessEntropyDistance(const sequence& str, utils::LZ_Args args) {
      std::vector<char>::size_type mid = str.size() / 2;  // the half of the sequence

      lz_double dist = 0;
      lz_int fh_complexity = 0;
      lz_int lh_complexity = 0;
      auto [seq_fh, seq_lh] = str.Split(mid);

      auto fh_fun = [&]() { fh_complexity = LempelZivFactorization(seq_fh, args); };
      auto lh_fun = [&]() { lh_complexity = LempelZivFactorization(seq_lh, args); };
      auto dist_fun = [&]() { dist = InformationDistance(seq_fh, seq_lh, args); };

      utils::par_do(dist_fun, fh_fun, lh_fun);

      return (1.0 - dist) * std::fmax(fh_complexity, lh_complexity);
   }

   //-------------------- Distance functions ----------------------------//
   lz_double InformationDistance(const sequence& T1, const sequence& T2) {
      lz_int C_t1, C_t2, C_all = 0;

      auto fh_fun = [&]() { C_t1 = LempelZivFactorization(T1); };

      auto lh_fun = [&]() { C_t2 = LempelZivFactorization(T2); };

      auto all_fun = [&]() { C_all = LempelZivFactorization(T1 + T2); };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);

      return res;
   }

   lz_double InformationDistance(const sequence& T1, const lz76::LZ_Result& R1, const sequence& T2,
                                 const lz76::LZ_Result& R2) {
      lz_int C_all = 0;

      C_all = LempelZivFactorization(T1 + T2);

      auto res =
          (C_all - std::fmin(R1.factorization, R2.factorization)) * 1.0 / std::fmax(R1.factorization, R2.factorization);

      return res;
   }

   lz_double InformationDistance(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      lz_int C_t1, C_t2, C_all = 0;

      auto fh_fun = [&]() { C_t1 = LempelZivFactorization(T1, args); };

      auto lh_fun = [&]() { C_t2 = LempelZivFactorization(T2, args); };

      auto all_fun = [&]() { C_all = LempelZivFactorization(T1 + T2, args); };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);

      return res;
   }

}  // namespace lz
