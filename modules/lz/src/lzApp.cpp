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
      // std::vector<lz_int> complex_result;
      LZ.complexity.clear();
      LZ.complexity.reserve(flags.input.size());

      for (sequence& str: flags.input) {
         auto clx = LempelZivFactorization(str, flags.sa_args);
         LZ.complexity.push_back(clx);
      }

      return EXIT_SUCCESS;
   };

   lz_int ExcessEntropyMi(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      // std::vector<lz_int> excess_entropy_mi;
      LZ.excess_entropy_mi.clear();
      LZ.excess_entropy_mi.reserve(flags.input.size());

      for (sequence str: flags.input) {
         auto res = ExcessEntropyMi(str, flags.sa_args);

         LZ.excess_entropy_mi.push_back(res);
      }

      return EXIT_SUCCESS;
   };

   lz_int ExcessEntropyShuffle(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      std::vector<lz_int> excess_entropy;
      excess_entropy.reserve(flags.input.size());

      for (auto str: flags.input) {
         auto excessentropy = ExcessEntropyShuffle(str, flags.sa_args);

         LZ.excess_entropy_shuffle.push_back(excessentropy.excess_value);
      }

      return EXIT_SUCCESS;
   }

   lz_int ExcessEntropyShuffleSequential(utils::LZ_Flags& flags, utils::LZ_Output& LZ) {
      std::vector<lz_int> excess_entropy;
      excess_entropy.reserve(flags.input.size());

      for (auto str: flags.input) {
         auto excessentropy = ExcessEntropyShuffleSequential(str, flags.sa_args);

         LZ.excess_entropy_shuffle.push_back(excessentropy.excess_value);
      }

      return EXIT_SUCCESS;
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

   lz_int InformationDistanceBySequence(utils::LZ_Flags& flags) {
      std::vector<sequence> text = flags.input;
      flags.sequence_info_distance.clear();
      flags.sequence_info_distance.reserve(flags.input.size());

      for (auto str: flags.input) {
         if (str.size() == 1) {
            flags.info_distance.push_back(0);
            continue;
         }
         std::size_t mid = str.size() / 2;  // the half of the sequence
         auto [seq_fh, seq_lh] = str.Split(mid);
         auto res = InformationDistance(seq_fh, seq_lh, flags.sa_args);

         flags.sequence_info_distance.push_back(res);
      }

      return EXIT_SUCCESS;
   }

}  // namespace lz
