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
   lz_int LempelZivFactorization(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      LZ.complexity.clear();
      LZ.complexity.reserve(flags.input.size() + 3);

      // auto fun = [&LZ, &flags](auto&& idx) {
      //    auto clx = LempelZivFactorization(flags.input[idx], flags.sa_args);
      //    LZ.complexity[idx] = clx;
      //    std::cout << "clx: " << clx << " -- " << idx << " -- " << LZ.complexity[idx] << std::endl;
      // };

      // utils::parallel_for(0, flags.input.size(), fun);
      // for (auto& x: LZ.complexity) std::cout << x << " ";
      // std::cout << std::endl;

      for (sequence& str: flags.input) {
         auto clx = LempelZivFactorization(str);
         LZ.complexity.push_back(clx);
      }

      return EXIT_SUCCESS;
   };

   lz_int LZEffectiveComplexity(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      // std::vector<lz_int> lz_effective_complexity;
      LZ.lz_effective_complexity.clear();
      LZ.lz_effective_complexity.reserve(flags.input.size());

      for (sequence str: flags.input) {
         auto res = LZEffectiveComplexity(str, flags.sa_args);

         LZ.lz_effective_complexity.push_back(res);
      }

      return EXIT_SUCCESS;
   };

   template <typename Fun>
   static utils::LZ_ExcessInfo ShuffleCalc(utils::LZ_Flags& flags, utils::LZ_Output& LZ, Fun&& fun) {
      utils::LZ_ExcessInfo excess_entropy;
      auto init_line = flags.shuffle_init_line;
      auto end_line = flags.shuffle_end_line;

      for (lz_size i = 0; i < flags.input.size(); i++) {
         auto str = flags.input[i];
         const auto processAllLines = init_line == utils::LZ_Args::ALL_LINES;
         const auto processOneLine =
             static_cast<lz_int>(i + 1) == init_line && end_line == utils::LZ_Args::UNDEFINED_LINES;
         const auto processRange = init_line <= static_cast<lz_int>(i + 1) && end_line >= static_cast<lz_int>(i + 1);

         if (processAllLines || processOneLine || processRange) {
            flags.sa_args.get_shuffle_terms = true;
            excess_entropy = fun(str, flags.sa_args);
            LZ.shuffle_entropy_terms.push_back(utils::LZ_Output::shuffle_terms{i + 1, excess_entropy.excess_by_terms});
         } else {
            excess_entropy = fun(str, flags.sa_args);
         }

         LZ.shuffle_entropy_deficit.push_back(excess_entropy.excess_value);
         LZ.multi_information.push_back(excess_entropy.multi_information);
      }

      return excess_entropy;
   }

   utils::LZ_ExcessInfo ShuffleEntropyDeficit(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      return ShuffleCalc(flags, LZ, [&](sequence s, utils::LZ_Args args) { return ShuffleEntropyDeficit(s, args); });
   }

   utils::LZ_ExcessInfo ShuffleEntropyDeficitSequential(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      return ShuffleCalc(flags, LZ,
                         [&](sequence s, utils::LZ_Args args) { return ShuffleEntropyDeficitSequential(s, args); });
   }

   lz_int ExcessEntropyDistance(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      // std::vector<lz_int> excess_entropy_dist;
      LZ.excess_entropy_dist.clear();
      LZ.excess_entropy_dist.reserve(flags.input.size());

      for (auto str: flags.input) {
         auto res = ExcessEntropyDistance(str, flags.sa_args);
         LZ.excess_entropy_dist.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int EntropyDensity(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      LZ.entropy_density.clear();
      LZ.entropy_density.reserve(flags.input.size());

      for (auto str: flags.input) {
         auto density = EntropyDensity(str, flags.sa_args);

         LZ.entropy_density.push_back(density);
      }

      return EXIT_SUCCESS;
   }

   lz_int InformationDistance(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      if (flags.input.size() == 1) {
         LZ.info_distance = {0};
         return EXIT_SUCCESS;
      }

      LZ.info_distance.clear();
      std::vector<sequence> text = flags.input;
      for (std::size_t i = 1; i < text.size(); i++) {
         auto res = InformationDistance(text[i - 1], text[i], flags.sa_args);
         LZ.info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

   lz_int InformationDistanceBySequence(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      std::vector<sequence> text = flags.input;
      LZ.sequence_info_distance.clear();
      LZ.sequence_info_distance.reserve(flags.input.size());

      for (auto str: flags.input) {
         std::size_t mid = str.size() / 2;  // the half of the sequence
         auto [seq_fh, seq_lh] = str.Split(mid);
         auto res = InformationDistance(seq_fh, seq_lh, flags.sa_args);

         LZ.sequence_info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

}  // namespace lz
