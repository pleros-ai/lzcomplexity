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
   //.......................................................................
   //                            THE BANANA
   //.......................................................................
   lz_int LempelZivFactorization(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      // std::vector<lz_int> complex_result;
      LZ.complexity.clear();
      LZ.complexity.reserve(flags.input.size());

      for (sequence str : flags.input) {

         auto clx = LempelZivFactorization(str, flags.sa_impl);
         LZ.complexity.push_back(clx);
      }

      return EXIT_SUCCESS;
   };

   lz_int LempelZivFactorization(const sequence& text) {
      lz76::LempelZiv76 L;
      L.Factorize(text);
      return L.getFactorization();
   };

   lz_int LempelZivFactorization(const sequence& text, utils::sa_type algorithm) {
      utils::LZ_SuffixArray _SA;
      lz76::LempelZiv76 L;

      std::visit([&](auto&& alg) { _SA = alg.construct(text.toString()); }, algorithm);

      L.Factorize(_SA);
      return L.getFactorization();
   };

   lz_int LempelZivFactorization(const std::string& text, utils::sa_type algorithm) {
      utils::LZ_SuffixArray _SA;
      lz76::LempelZiv76 L;

      std::visit([&](auto&& alg) { _SA = alg.construct(text); }, algorithm);

      L.Factorize(_SA);
      return L.getFactorization();
   };

   lz_int LempelZivFactorization(const std::string& text, utils::SA_ALG algorithm, lz_int chunks_count, lz_int max_context) {
      utils::LZ_SuffixArray _SA;
      lz76::LempelZiv76 L;

      if (algorithm == utils::SA_ALG::sais) {
         lz::suffixarray::SAIS alg(text.data(), text.length());
         _SA = alg.construct();
      }

      if (algorithm == utils::SA_ALG::caps) {
         if (chunks_count < 0) throw LZSuffixArrayError();
         lz::suffixarray::CaPS_SA alg(text.data(), text.length(), chunks_count, max_context);
         // alg.debug = true; 
         _SA = alg.construct();
      }

      L.Factorize(_SA);
      return L.getFactorization();
   };

   lz_int ExcessEntropyMi(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      // std::vector<lz_int> excess_entropy_mi;
      LZ.excess_entropy_mi.clear();
      LZ.excess_entropy_mi.reserve(flags.input.size());

      for (sequence str : flags.input) {
         std::vector<char>::size_type mid = str.size() / 2; // the half of the sequence

         lz_int C_, C_fh, C_lh = 0;
         auto [seq_fh, seq_lh] = str.Split(mid);

         auto fh_fun = [&]() {
            C_fh = LempelZivFactorization(seq_fh, flags.sa_impl);
            };
         auto lh_fun = [&]() {
            C_lh = LempelZivFactorization(seq_lh, flags.sa_impl);
            };
         auto all_fun = [&]() {
            C_ = LempelZivFactorization(str, flags.sa_impl);
            };

         utils::par_do(all_fun, fh_fun, lh_fun);

         LZ.excess_entropy_mi.push_back(C_fh + C_lh - C_);
      }

      return EXIT_SUCCESS;
   };

   lz_int ExcessEntropyShuffle(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      std::vector<lz_int> excess_entropy;
      excess_entropy.reserve(flags.input.size());

      for (auto str : flags.input) {
         std::vector<char>::size_type mm = utils::Mmax(str.size()); // the maximum number for the sum in the entropy estimation
         double excessentropy = 0;

         for (unsigned int m = 1; m <= mm; m++) {

            std::random_device rd;
            std::mt19937 gen{ rd() };

            sequence sseq = Shuffle(str, m, str.size() / 2); // Shuffling is made for half the size of the sequence, hope that is enough
            std::vector <unsigned int> slzf;
            auto scomplexity = LempelZivFactorization(sseq);
            double eeterm = 0;
            eeterm = log(str.size()) * fabs((double)scomplexity - (double)LZ.complexity[0]) / (str.size() * log(str.alphabetSize()));

            // cumexcessentropy.push_back(eeterm);
            excessentropy += eeterm;

            if (m == 1)
               LZ.multi_information.push_back(eeterm);
         }

         LZ.excess_entropy_shuffle.push_back(excessentropy);
      }

      return EXIT_SUCCESS;
   }

   lz_int ExcessEntropyDistance(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      // std::vector<lz_int> excess_entropy_dist;
      LZ.excess_entropy_dist.clear();
      LZ.excess_entropy_dist.reserve(flags.input.size());

      for (auto str : flags.input) {
         std::vector<char>::size_type mid = str.size() / 2; // the half of the sequence

         double dist = 0;
         lz_int fh_complexity = 0;
         lz_int lh_complexity = 0;
         auto [seq_fh, seq_lh] = str.Split(mid);

         auto fh_fun = [&]() {
            fh_complexity = LempelZivFactorization(seq_fh, flags.sa_impl);
            };
         auto lh_fun = [&]() {
            lh_complexity = LempelZivFactorization(seq_lh, flags.sa_impl);
            };
         auto dist_fun = [&]() {
            dist = InformationDistance(seq_fh, seq_lh, flags.sa_impl);
            };

         utils::par_do(dist_fun, fh_fun, lh_fun);

         auto res = (1 - dist) * std::fmax(fh_complexity, lh_complexity);
         LZ.excess_entropy_dist.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int EntropyDensity(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      LZ.complexity.clear();
      LZ.complexity.reserve(flags.input.size());
      LZ.entropy_density.clear();
      LZ.entropy_density.reserve(flags.input.size());

      for (auto str : flags.input) {
         utils::LZ_SuffixArray _SA;
         lz76::LempelZiv76 L;

         std::visit([&](auto alg) { _SA = alg.construct(str.toString()); }, flags.sa_impl);

         L.Factorize(_SA);

         double div = str.size() / std::log(str.size());

         LZ.complexity.push_back(L.getFactorization());
         LZ.entropy_density.push_back(L.getFactorization() / div);
         // std::cout << L;
      }

      return EXIT_SUCCESS;
   }

   lz_int InformationDistance(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      if (flags.input.size() == 1) {
         LZ.info_distance = { 0 };
         return EXIT_SUCCESS;
      }

      LZ.info_distance.clear();
      std::vector<sequence> text = flags.input;
      for (std::size_t i = 1; i < text.size(); i++) {
         auto res = InformationDistance(text[i - 1], text[i], flags.sa_impl);
         LZ.info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   double InformationDistance(const std::string& T1, const std::string& T2, utils::sa_type sa_impl) {
      lz_int C_t1, C_t2, C_all = 0;

      auto fh_fun = [&]() {
         C_t1 = LempelZivFactorization(T1, sa_impl);
         };

      auto lh_fun = [&]() {
         C_t2 = LempelZivFactorization(T2, sa_impl);
         };

      auto all_fun = [&]() {
         C_all = LempelZivFactorization(T1 + T2, sa_impl);
         };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::min(C_t1, C_t2)) * 1.0 / std::max(C_t1, C_t2);

      return res;
   }

   double InformationDistance(const sequence& T1, const sequence& T2, utils::sa_type sa_impl) {
      lz_int C_t1, C_t2, C_all = 0;

      auto fh_fun = [&]() {
         C_t1 = LempelZivFactorization(T1, sa_impl);
         };

      auto lh_fun = [&]() {
         C_t2 = LempelZivFactorization(T2, sa_impl);
         };

      auto all_fun = [&]() {
         C_all = LempelZivFactorization(T1 + T2, sa_impl);
         };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);

      return res;
   }

   lz_int InformationDistanceBySequence(utils::LZ_Flags& flags) {
      std::vector<sequence> text = flags.input;
      flags.sequence_info_distance.clear();
      flags.sequence_info_distance.reserve(flags.input.size());

      for (auto str : flags.input) {
         if (str.size() == 1) {
            flags.info_distance.push_back(0);
            continue;
         }
         std::size_t mid = str.size() / 2; // the half of the sequence
         auto [seq_fh, seq_lh] = str.Split(mid);
         auto res = InformationDistance(seq_fh, seq_lh, flags.sa_impl);

         flags.sequence_info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }


} // namespace lz
