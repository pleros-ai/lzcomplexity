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

#include <lz/lzApp.h>

namespace lz {
   //.......................................................................
   //                            THE BANANA
   //.......................................................................
   lz_int LZ(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      for (auto& seq: flags.input) {
         auto res = LZ(seq, flags.sa_args);
         lz.data.push_back(res);
      }

      return EXIT_SUCCESS;
   };

   lz_int LempelZivFactorization(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      lz.complexity.clear();
      lz.complexity.reserve(flags.input.size() + 3);
      lz.calculated_complexity = std::vector<lz_bool>(flags.input.size() + 3, false);

      for (lz_size idx = 0; idx < flags.input.size(); idx++) {
         auto str = flags.input[idx];
         auto clx = internal::LempelZivFactorization(str);
         lz.complexity.push_back(clx.factorization);
         lz.half_complexity.push_back(clx.half_factorization);
         lz.calculated_complexity[idx] = true;

         lz.setComplexity(idx, clx.factorization);
         lz.setFactorsStddev(idx, clx.factors_stddev);
         lz.setEpsilon(idx, clx.epsilon);
      }

      return EXIT_SUCCESS;
   };

   lz_int LZEffectiveComplexity(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      // std::vector<lz_int> lz_effective_complexity;
      lz.lz_effective_complexity.clear();
      lz.lz_effective_complexity.reserve(flags.input.size());
      lz_double res = 0;

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto str = flags.input[i];
         if (false) {
            lz_size mid = str.size() / 2;  // the half of the sequence

            lz_double C_fh = static_cast<double>(lz.half_complexity[i]);
            lz_double C_, C_lh = 0;
            auto      sequences = str.Split(mid);
            auto      new_seq   = internal::MergeSequences(sequences.first, sequences.second);

            // C_lh = LempelZivFactorization(sequences.second, flags.sa_args);
            auto fh_fun  = [&]() { C_fh = EntropyDensity(sequences.first, flags.sa_args); };
            auto lh_fun  = [&]() { C_lh = EntropyDensity(sequences.second, flags.sa_args); };
            auto new_fun = [&]() { C_ = EntropyDensity(new_seq, flags.sa_args); };

            utils::par_do(new_fun, fh_fun, lh_fun);
            // res = (C_fh + C_lh - 2 * C_) * std::log(mid) / (mid * std::log(str.getAlphabetSize()));
            res = C_fh + C_lh - C_;
         } else {
            res = LZEffectiveComplexityNormalized(str, flags.sa_args);
         }

         lz.lz_effective_complexity.push_back(res);
      }

      return EXIT_SUCCESS;
   };

   template<typename Fun>
   static lz_int ShuffleCalc(utils::LZ_Flags& flags, utils::LZ_Output& lz, Fun&& fun) {
      utils::LZ_Shuffle excess_entropy;
      auto              init_line = flags.shuffle_init_line;
      auto              end_line  = flags.shuffle_end_line;

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto       str             = flags.input[i];
         const auto processAllLines = init_line == utils::LZ_Args::ALL_LINES;
         const auto processOneLine =
            static_cast<lz_int>(i + 1) == init_line && end_line == utils::LZ_Args::UNDEFINED_LINES;
         const auto processRange = init_line <= static_cast<lz_int>(i + 1) && end_line >= static_cast<lz_int>(i + 1);

         utils::shuffle_terms terms;

         if (processAllLines || processOneLine || processRange) {
            flags.sa_args.get_shuffle_terms = true;
         } else {
            flags.sa_args.get_shuffle_terms = false;
         }

         if (lz.calculated_complexity[i]) {
            std::pair<std::vector<lz_int>, lz_size> random_run = ShuffleFactorization(str, flags.sa_args);
            auto [H_rand, mm]                                  = random_run;

            excess_entropy = ShuffleEntropyCalculation(str, flags.sa_args, lz.complexity[i], H_rand, mm);
         } else {
            excess_entropy = fun(str, flags.sa_args);
         }

         if (processAllLines || processOneLine || processRange) {
            terms = {i + 1, excess_entropy.excess_by_terms};
         }

         lz.whole_random_shuffle_complexity.push_back(
            {excess_entropy.max_block_size, excess_entropy.excess_value, terms});
         // lz.multi_information.push_back(excess_entropy.multi_information);

         lz.setWholeRandomShuffleComplexity(i, excess_entropy);
      }

      return EXIT_SUCCESS;
   }

   lz_int WholeRandomShuffleComplexity(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      return ShuffleCalc(
         flags, lz, [&](sequence s, utils::LZ_Args args) { return WholeRandomShuffleComplexity(s, args); });
   }

   lz_int RandomShuffleComplexity(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      utils::LZ_Shuffle excess_entropy;
      auto              init_line = flags.shuffle_init_line;
      auto              end_line  = flags.shuffle_end_line;

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto       str             = flags.input[i];
         const auto processAllLines = init_line == utils::LZ_Args::ALL_LINES;
         const auto processOneLine =
            static_cast<lz_int>(i + 1) == init_line && end_line == utils::LZ_Args::UNDEFINED_LINES;
         const auto processRange = init_line <= static_cast<lz_int>(i + 1) && end_line >= static_cast<lz_int>(i + 1);

         utils::shuffle_terms terms;

         if (processAllLines || processOneLine || processRange) {
            flags.sa_args.get_shuffle_terms = true;
         } else {
            flags.sa_args.get_shuffle_terms = false;
         }

         excess_entropy = RandomShuffleComplexity(str, flags.sa_args);

         if (processAllLines || processOneLine || processRange) {
            terms = {i + 1, excess_entropy.excess_by_terms};
         }

         lz.random_shuffle_complexity.push_back({excess_entropy.max_block_size, excess_entropy.excess_value, terms});
         lz.multi_information.push_back(excess_entropy.multi_information);

         lz.setRandomShuffleComplexity(i, excess_entropy);
      }

      return EXIT_SUCCESS;
   }

   lz_int RandomShuffleComplexitySequential(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      return ShuffleCalc(
         flags, lz, [&](sequence s, utils::LZ_Args args) { return RandomShuffleComplexitySequential(s, args); });
   }

   lz_int ExcessEntropyDistance(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      // std::vector<lz_int> excess_entropy_dist;
      lz.excess_entropy_dist.clear();
      lz.excess_entropy_dist.reserve(flags.input.size());
      lz_double res = 0;

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto str = flags.input[i];
         if (lz.calculated_complexity[i]) {
            lz_size mid = str.size() / 2;  // the half of the sequence

            lz_int    C_lz          = lz.complexity[i];
            lz_double fh_complexity = static_cast<lz_double>(lz.half_complexity[i]);
            lz_int    lh_complexity = 0;
            auto      par_seq       = str.Drop(mid);

            lh_complexity = LempelZivFactorization(par_seq, flags.sa_args);
            // auto fh = str.Take(mid);
            // auto fh_complexity2 = LempelZivFactorization(fh, flags.sa_args);
            // distance
            auto dist = (C_lz - std::fmin(fh_complexity, lh_complexity)) / std::fmax(fh_complexity, lh_complexity);

            res = (1.0 - dist) * std::fmax(lz.half_complexity[i], lh_complexity);
         } else {
            res = ExcessEntropyDistance(str, flags.sa_args);
         }

         lz.excess_entropy_dist.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int ExtraMeasures(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      // std::vector<lz_int> excess_entropy_dist;
      lz.excess_entropy_dist.clear();
      lz.excess_entropy_dist.reserve(flags.input.size());
      lz_double res = 0;

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto str = flags.input[i];

         lz_int    C_lz          = 0;
         lz_double fh_complexity = 0;
         lz_int    lh_complexity = 0;

         if (lz.calculated_complexity[i]) {
            C_lz          = lz.complexity[i];
            fh_complexity = static_cast<lz_double>(lz.half_complexity[i]);
         } else {
            auto lz       = internal::LempelZivFactorization(str, flags.sa_args);
            C_lz          = lz.factorization;
            fh_complexity = lz.half_factorization;
         }

         lz_size mid     = str.size() / 2;  // the half of the sequence
         auto    par_seq = str.Drop(mid);

         lh_complexity = LempelZivFactorization(par_seq, flags.sa_args);

         auto MI = fh_complexity + lh_complexity - C_lz;

         auto rajski         = 2 - (fh_complexity + lh_complexity) / C_lz;
         auto fh_uncertainty = MI / fh_complexity;
         auto lh_uncertainty = MI / lh_complexity;
         auto redundancy     = MI / (fh_complexity + lh_complexity);
         auto pearson        = MI / std::sqrt(fh_complexity * lh_complexity);

         lz.extra.push_back({rajski, redundancy, fh_uncertainty, lh_uncertainty, pearson});
         lz.setExtras(i, {rajski, redundancy, fh_uncertainty, lh_uncertainty, pearson});
      }

      return EXIT_SUCCESS;
   }

   lz_int EntropyDensity(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      lz.entropy_density.clear();
      lz.entropy_density.reserve(flags.input.size());

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto   str     = flags.input[i];
         double density = 0.0;

         if (lz.calculated_complexity[i]) {
            lz_double div = str.size() * utils::log(flags.alphabet_size, flags.sa_args.log_base) /
                            utils::log(str.size(), flags.sa_args.log_base);

            density = lz.complexity[i] / div;
         } else {
            density = EntropyDensity(str, flags.sa_args);
         }

         lz.entropy_density.push_back(density);
         lz.setEntropyDensity(i, density);
      }

      return EXIT_SUCCESS;
   }

   lz_int InformationDistance(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      if (flags.input.size() == 1) {
         lz.info_distance = {0};
         return EXIT_SUCCESS;
      }

      lz.info_distance.clear();
      lz_double             res  = 0.0;
      std::vector<sequence> text = flags.input;
      for (std::size_t i = 1; i < text.size(); i++) {
         if (lz.calculated_complexity[i] && lz.calculated_complexity[i - 1]) {
            auto C_t1 = lz.complexity[i - 1];
            auto C_t2 = lz.complexity[i];
            auto _C   = LempelZivFactorization(text[i - 1] + text[i], flags.sa_args);

            res = (_C - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);
         } else {
            res = InformationDistance(text[i - 1], text[i], flags.sa_args);
         }
         lz.info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int RandomShuffleDistance(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      if (flags.input.size() == 1) {
         lz.info_distance = {0};
         return EXIT_SUCCESS;
      }

      lz.info_distance.clear();
      lz_double             res  = 0.0;
      std::vector<sequence> text = flags.input;
      for (std::size_t i = 1; i < text.size(); i++) {
         res = RandomShuffleDistance(text[i - 1], text[i], flags.sa_args);
         lz.info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int InformationDistanceBySequence(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      std::vector<sequence> text = flags.input;
      lz.sequence_info_distance.clear();
      lz.sequence_info_distance.reserve(flags.input.size() + 3);

      lz_double res = 0.0;
      for (std::size_t i = 0; i < flags.input.size(); i++) {
         auto        str       = flags.input[i];
         std::size_t mid       = str.size() / 2;  // the half of the sequence
         auto        sequences = str.Split(mid);
         if (lz.calculated_complexity[i]) {
            lz_uint C_t1, C_t2;
            auto    _C = lz.complexity[i];

            auto fh_fun = [&]() { C_t1 = LempelZivFactorization(sequences.first, flags.sa_args); };
            auto lh_fun = [&]() { C_t2 = LempelZivFactorization(sequences.second, flags.sa_args); };
            utils::par_do(fh_fun, lh_fun);

            res = (_C - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);
         } else {
            res = InformationDistance(sequences.first, sequences.second, flags.sa_args);
         }
         lz.sequence_info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int RandomShuffleDistanceBySequence(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      std::vector<sequence> text = flags.input;
      lz.sequence_info_distance.clear();
      lz.sequence_info_distance.reserve(flags.input.size() + 3);

      lz_double res = 0.0;
      for (std::size_t i = 0; i < flags.input.size(); i++) {
         auto str       = flags.input[i];
         auto mid       = str.size() / 2;  // the half of the sequence
         auto sequences = str.Split(mid);
         // if (lz.calculated_complexity[i]) {
         //    std::pair<std::vector<lz_int>, lz_size> random_run;
         //    lz_uint t2_cpx;
         //    lz_double complexity = static_cast<lz_double>(lz.complexity[i]);
         //    lz_double t1_cpx;

         //    auto t1_fun = [&]() { t1_cpx = EntropyDensity(sequences.first, flags.sa_args); };
         //    auto t2_fun = [&]() { t2_cpx = EntropyDensity(sequences.second, flags.sa_args); };
         //    // auto factor_fun = [&]() { complexity = LempelZivFactorization(str, flags.sa_args); };
         //    auto rand_fun = [&]() { random_run = ShuffleFactorization(str, flags.sa_args); };

         //    utils::par_do(rand_fun, t1_fun, t2_fun);

         //    auto [H_rand, mm] = random_run;

         //    auto shuffle = ShuffleEntropyCalculation(str, complexity, H_rand, mm, false);

         //    res = 1.0 - shuffle.excess_value / std::fmax(t1_cpx, t2_cpx);
         // } else {
         res = RandomShuffleDistance(sequences.first, sequences.second, flags.sa_args);
         // }
         lz.sequence_info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int MutualInformationBySequence(utils::LZ_Flags& flags, utils::LZ_Output& lz) {
      std::vector<sequence> text = flags.input;
      lz.sequence_info_distance.clear();
      lz.sequence_info_distance.reserve(flags.input.size() + 3);

      lz_double res = 0.0;
      for (std::size_t i = 0; i < flags.input.size(); i++) {
         auto str       = flags.input[i];
         auto mid       = str.size() / 2;  // the half of the sequence
         auto sequences = str.Split(mid);
         // if (lz.calculated_complexity[i]) {
         //    std::pair<std::vector<lz_int>, lz_size> random_run;
         //    lz_uint t2_cpx;
         //    lz_double complexity = static_cast<lz_double>(lz.complexity[i]);
         //    lz_double t1_cpx;

         //    auto t1_fun = [&]() { t1_cpx = EntropyDensity(sequences.first, flags.sa_args); };
         //    auto t2_fun = [&]() { t2_cpx = EntropyDensity(sequences.second, flags.sa_args); };
         //    // auto factor_fun = [&]() { complexity = LempelZivFactorization(str, flags.sa_args); };
         //    auto rand_fun = [&]() { random_run = ShuffleFactorization(str, flags.sa_args); };

         //    utils::par_do(rand_fun, t1_fun, t2_fun);

         //    auto [H_rand, mm] = random_run;

         //    auto shuffle = ShuffleEntropyCalculation(str, complexity, H_rand, mm, false);

         //    res = 1.0 - shuffle.excess_value / std::fmax(t1_cpx, t2_cpx);
         // } else {
         res = MutualInformation(sequences.first, sequences.second, flags.sa_args);
         // }
         lz.mutual_information.push_back(res);
      }

      return EXIT_SUCCESS;
   }

}  // namespace lz
