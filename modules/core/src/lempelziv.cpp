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

#include <map>

#ifdef _LIBCPP_HAS_PARALLEL_ALGORITHMS
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

#include "../../utils/src/lz_tbb_arena.h"

namespace lz {
   //.......................................................................//
   //                            THE BANANA                                 //
   //.......................................................................//

   namespace internal {
      auto getPartitionsFromSequenceSize(const auto seq_size) {
         auto chunks = utils::num_workers();
         if (seq_size < chunks * 10) {
            chunks = 1;
         } else if (seq_size > 1e6) {
            chunks = 100;
         }
         return chunks;
      }

      utils::LZ_Args getDefaultArgs(const sequence& seq) {
         utils::LZ_Args args;
         args.chunks = internal::getPartitionsFromSequenceSize(seq.size());
         return args;
      }

      LZ_Data LempelZivFactorization(const sequence& text) {
         return internal::LempelZivFactorization(text, getDefaultArgs(text));
      };

      LZ_Data LempelZivFactorization(const sequence& text, utils::LZ_Args args) {
         lz76::LempelZiv76 L;
         L.Factorize(text, args);

         auto pos =
            std::find_if(PAR L.getFactorsBegin(), L.getFactorsEnd(), [&](auto val) { return val > text.size() / 2; });

         lz_int half_lz = std::distance(L.getFactorsBegin(), pos) - 1;
         return {L.getFactorization(), half_lz, L.getStddev(), L.getEpsilon()};
      };

      sequence MergeSequences(sequence s1, sequence s2) {
         std::vector<char>           seq;
         std::map<std::string, char> new_alphabet;
         // for now both sequence have the same alphabet
         for (auto i = 0ul; i < std::min(s1.size(), s2.size()); i++) [[likely]] {
            std::string key{s1[i], s2[i]};
            if (auto val = new_alphabet.find(key); val != new_alphabet.end())
               seq.push_back(val->second);
            else {
               char new_element = '0' + new_alphabet.size();
               new_alphabet.insert({key, new_element});
               seq.push_back(new_element);
            }
         }

         return sequence{seq, s1.getAlphabetSize() + s2.getAlphabetSize()};
      }
   }  // namespace internal

   utils::LempelZiv LZ(const sequence& seq, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      // factorization, factors, epsilon and factors stddev
      L.Factorize(seq, args);

      lz_double div = static_cast<double>(seq.size()) / utils::log(seq.size(), args.log_base);
      // entropy density
      auto entropy = L.getFactorization() / div;
      // random shuffle complexity (rsc)
      utils::LZ_Shuffle rsc;
      auto              rsc_fun = [&rsc, &seq, &args]() { rsc = RandomShuffleComplexity(seq, args); };
      // whole sequence random shuffle complexity (w_rsc -- optional)
      utils::LZ_Shuffle w_rsc;
      auto              w_rsc_fun = [&seq, &args, &L]() {
         auto [H_rand, mm]       = ShuffleFactorization(seq, args);
         utils::LZ_Shuffle w_rsc = ShuffleEntropyCalculation(seq, args, L.getFactorization(), H_rand, mm);
      };
      // Extras
      utils::LZ_Extra extras;
      auto            extra_fun = [&]() { extras = ExtraMeasures(seq, args); };

      utils::par_do(rsc_fun, w_rsc_fun, extra_fun);

      // Error for Normal distribution factors length
      div               = std::sqrt(seq.size() / utils::log(seq.size(), args.log_base));
      auto normal_error = std::sqrt(entropy * entropy * entropy) * L.getStddev() / div;
      // Error for Poison distribution factors length
      auto poison_error = entropy / seq.size();

      return {L.getFactorization(),
              L.getFactors(),
              entropy,
              w_rsc,
              rsc,
              normal_error,
              poison_error,
              L.getEpsilon(),
              L.getStddev(),
              extras};
   }

   //-------------------- Factorization functions ----------------------------//
   lz_uint LempelZivFactorization(const sequence& text, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      L.Factorize(text, args);
      return L.getFactorization();
   };

   lz76::LZ_Result LempelZivFactors(const sequence& text, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      return L.Factorize(text, args);
   };

   //-------------------- Entropy density functions ----------------------------//
   lz_double EntropyDensity(const sequence& text, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      L.Factorize(text, args);
      // lz_double div = text.size() * std::log(text.getAlphabetSize()) / std::log(text.size());
      lz_double div = static_cast<double>(text.size()) / utils::log(text.size(), args.log_base);

      return L.getFactorization() / div;
   }

   //-------------------- Excess of entropy functions ----------------------------//
   lz_int LZEffectiveComplexity(const sequence& text, utils::LZ_Args args) {
      std::vector<char>::size_type mid = text.size() / 2;  // the half of the sequence

      lz_int C_, C_fh, C_lh = 0;
      auto   sequences = text.Split(mid);
      auto   new_seq   = internal::MergeSequences(sequences.first, sequences.second);

      auto fh_fun  = [&]() { C_fh = LempelZivFactorization(sequences.first, args); };
      auto lh_fun  = [&]() { C_lh = LempelZivFactorization(sequences.second, args); };
      auto all_fun = [&C_, &new_seq, &args]() {
         auto cpy     = args;
         cpy.alphabet = new_seq.getAlphabetSize();
         cpy.log_base = new_seq.getAlphabetSize();
         C_           = LempelZivFactorization(new_seq, cpy);
      };

      utils::par_do(all_fun, fh_fun, lh_fun);
      return C_fh + C_lh - 2 * C_;
   };

   lz_double LZEffectiveComplexityNormalized(const sequence& text, utils::LZ_Args args) {
      auto excess = LZEffectiveComplexity(text, args);
      auto N      = text.size() / 2;
      auto div    = (N * utils::log(args.alphabet, args.log_base)) / utils::log(N, args.log_base);

      return excess / div;
   }

   utils::LZ_Shuffle RandomShuffleComplexitySequential(const sequence& str, utils::LZ_Args args) {
      std::size_t mm = args.block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
         mm += 10;                                // begin aggressive
      }

      utils::LZ_Shuffle result;
      result.max_block_size    = mm;
      lz_double excess_entropy = 0;
      auto      complex        = LempelZivFactorization(str, args);

      for (unsigned int m = 1; m <= mm; m++) {
         sequence rand_seq = Shuffle(
            str, m, str.size() / 2);  // Shuffling is made for half the size of the sequence, hope that is enough
         std::vector<unsigned int> slzf;
         auto                      rand_complexity = LempelZivFactorization(rand_seq, args);
         lz_double                 ee_term         = utils::log(str.size(), args.log_base) *
                             std::fabs((lz_double)rand_complexity - (lz_double)complex) /
                             (str.size() * utils::log(args.alphabet, args.log_base));

         excess_entropy += ee_term;

         if (args.get_shuffle_terms)
            result.excess_by_terms.push_back(ee_term);
         if (m == 1)
            result.multi_information = ee_term;
      }

      result.excess_value = excess_entropy;
      return result;
   }
   utils::LZ_Shuffle RandomShuffleComplexitySequential(const std::string& str, utils::LZ_Args args) {
      return RandomShuffleComplexitySequential(sequence{str}, args);
   }

   std::pair<std::vector<lz_int>, lz_size> ShuffleFactorization(const sequence& str, utils::LZ_Args args) {
      lz_int mm = args.block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
         mm += 10;                                // begin aggressive
      }

      std::vector<lz_int> res(mm + 3);  // Reserve the number of blocks for shuffling + 3

      auto fun = [&](lz_size idx) {
         sequence rand_seq = Shuffle(str, idx, str.size() / 2);  // Shuffling is made for half the
                                                                 // size of the sequence, hope that is enough
         auto rand_complexity = LempelZivFactorization(rand_seq, args);
         res[idx]             = rand_complexity;
      };

      utils::parallel_for(1, mm + 1, fun);

      return {res, mm};
   }

   utils::LZ_Shuffle ShuffleEntropyCalculation(const sequence&           str,
                                               const utils::LZ_Args      args,
                                               const lz_int              complexity,
                                               const std::vector<lz_int> H_rand,
                                               lz_int                    mm) {
      utils::LZ_Shuffle result;
      result.max_block_size = mm;

      auto terms = utils::map(H_rand, [&](lz_int val) {
         return utils::log(str.size(), args.log_base) * std::fabs((lz_double)val - (lz_double)complexity) /
                (str.size() * utils::log(args.alphabet, args.log_base));
      });

      if (args.get_shuffle_terms) {
         result.excess_by_terms = terms;
      }

      result.multi_information = terms[1];

      auto body = [&terms](const auto& rng, lz_double init) -> lz_double {
         for (auto idx = rng.begin(); idx != rng.end(); idx++) {
            init += terms[idx];
         }
         return init;
      };
      auto reduce = [](const lz_double& a, const lz_double& b) -> lz_double { return a + b; };

      // result.excess_value = tbb::parallel_reduce(tbb_range, 0.0, body, reduce);
      result.excess_value = utils::parallel_reduce(1, mm + 1, 0.0, body, reduce);
      return result;
   }

   utils::LZ_Shuffle RandomShuffleComplexity(const sequence& str, utils::LZ_Args args) {
      auto [past, future] = str.Split(str.size() / 2);
      auto new_seq        = internal::MergeSequences(past, future);

      std::pair<std::vector<lz_int>, lz_size> random_run;
      lz_int                                  complexity;

      auto factor_fun = [&]() { complexity = LempelZivFactorization(new_seq, args); };
      auto rand_fun   = [&]() { random_run = ShuffleFactorization(new_seq, args); };

      utils::par_do(factor_fun, rand_fun);

      auto [H_rand, mm] = random_run;

      args.alphabet = new_seq.getAlphabetSize();
      args.log_base = new_seq.getAlphabetSize();
      return ShuffleEntropyCalculation(new_seq, args, complexity, H_rand, mm);
   }

   utils::LZ_Shuffle WholeRandomShuffleComplexity(const sequence& str, utils::LZ_Args args) {
      std::pair<std::vector<lz_int>, lz_size> random_run;
      lz_int                                  complexity;

      auto factor_fun = [&]() { complexity = LempelZivFactorization(str, args); };
      auto rand_fun   = [&]() { random_run = ShuffleFactorization(str, args); };

      utils::par_do(factor_fun, rand_fun);

      auto [H_rand, mm] = random_run;

      return ShuffleEntropyCalculation(str, args, complexity, H_rand, mm);
   }

   lz_double ExcessEntropyDistance(const sequence& str, utils::LZ_Args args) {
      std::vector<char>::size_type mid = str.size() / 2;  // the half of the sequence

      lz_double dist          = 0;
      lz_int    fh_complexity = 0;
      lz_int    lh_complexity = 0;
      auto      par_seq       = str.Split(mid);

      auto fh_fun   = [&]() { fh_complexity = LempelZivFactorization(par_seq.first, args); };
      auto lh_fun   = [&]() { lh_complexity = LempelZivFactorization(par_seq.second, args); };
      auto dist_fun = [&]() { dist = InformationDistance(par_seq.first, par_seq.second, args); };

      utils::par_do(dist_fun, fh_fun, lh_fun);

      return (1.0 - dist) * std::fmax(fh_complexity, lh_complexity);
   }

   //-------------------- Distance functions ----------------------------//
   lz_double InformationDistance(const sequence&        T1,
                                 const lz76::LZ_Result& R1,
                                 const sequence&        T2,
                                 const lz76::LZ_Result& R2) {
      lz_int C_all = 0;

      C_all = LempelZivFactorization(T1 + T2);

      auto res =
         (C_all - std::fmin(R1.factorization, R2.factorization)) * 1.0 / std::fmax(R1.factorization, R2.factorization);

      return res;
   }

   lz_double InformationDistance(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      lz_int C_t1, C_t2, C_all = 0;

      auto fh_fun = [&C_t1, &T1, &args]() { C_t1 = LempelZivFactorization(T1, args); };

      auto lh_fun = [&C_t2, &T2, &args]() { C_t2 = LempelZivFactorization(T2, args); };

      auto all_fun = [&C_all, &T1, &T2, &args]() { C_all = LempelZivFactorization(T1 + T2, args); };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);

      return res;
   }

   lz_double InformationDistanceZ(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      lz_int C_t1, C_t2, C_all = 0;

      auto z = internal::MergeSequences(T1, T2);

      auto fh_fun = [&C_t1, &T1, &args]() { C_t1 = LempelZivFactorization(T1, args); };

      auto lh_fun = [&C_t2, &T2, &args]() { C_t2 = LempelZivFactorization(T2, args); };

      auto all_fun = [&C_all, &z, &args]() {
         auto args_cpy     = args;
         args_cpy.alphabet = z.getAlphabetSize();
         args_cpy.log_base = z.getAlphabetSize();
         C_all             = LempelZivFactorization(z, args_cpy);
      };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::fmin(C_t1, C_t2)) * 1.0 / std::fmax(C_t1, C_t2);

      return res;
   }

   // Mutual information using shuffle of merged (Z) sequence
   lz_double MutualInformation(const sequence& s1, const sequence& s2, utils::LZ_Args args) {
      auto                                    seq = internal::MergeSequences(s1, s2);
      std::pair<std::vector<lz_int>, lz_size> random_run;
      lz_uint                                 complexity;

      auto factor_fun = [&]() { complexity = LempelZivFactorization(seq, args); };
      auto rand_fun   = [&]() { random_run = ShuffleFactorization(seq, args); };

      utils::par_do(factor_fun, rand_fun);

#if __cplusplus >= 201703L
      lz_double random_sum = std::reduce(PAR random_run.first.begin(), random_run.first.end());
#else
      auto random_sum = std::accumulate(PAR random_run.first.begin(), random_run.first.end(), 0.0);
#endif
      return 1.0 - ((lz_double)random_run.second * (lz_double)complexity) / random_sum;
   }

   lz_double RandomShuffleDistance(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      auto MI = MutualInformation(T1, T2, args);
      return 1 - MI;
   }

   //-------------------- Other distance functions ----------------------------//
   lz_double LZNormalError(const sequence& seq, utils::LZ_Args args) {
      lz76::LempelZiv76 L;
      L.Factorize(seq, args);

      auto entropy = L.getFactorization() * utils::log(seq.size(), args.log_base) / static_cast<double>(seq.size());

      auto div = std::sqrt(seq.size() / utils::log(seq.size(), args.log_base));

      return std::sqrt(entropy * entropy * entropy) * L.getStddev() / div;
   }

   lz_double LZPoisonError(const sequence& seq, utils::LZ_Args args) {
      auto entropy = EntropyDensity(seq, args);
      return entropy / seq.size();
   }

   //-------------------------------- Extras --------------------------------//
   utils::LZ_Extra ExtraMeasures(const sequence& str, utils::LZ_Args args) {
      auto              mid           = str.size() / 2;  // the half of the sequence
      auto              par_seq       = str.Drop(mid);
      lz_uint           lh_complexity = 0;
      internal::LZ_Data lz;

      auto lz_fun = [&lz, &str, &args]() { lz = internal::LempelZivFactorization(str, args); };
      auto lh_fun = [&lh_complexity, &par_seq, &args]() { lh_complexity = LempelZivFactorization(par_seq, args); };

      utils::par_do(lz_fun, lh_fun);

      auto C_lz          = static_cast<lz_double>(lz.factorization);
      auto fh_complexity = static_cast<lz_double>(lz.half_factorization);

      auto MI = fh_complexity + lh_complexity - C_lz;

      auto rajski         = 2 - (fh_complexity + lh_complexity) / C_lz;
      auto fh_uncertainty = MI / fh_complexity;
      auto lh_uncertainty = MI / lh_complexity;
      auto redundancy     = MI / (fh_complexity + lh_complexity);
      auto pearson        = MI / std::sqrt(fh_complexity * lh_complexity);

      return {rajski, redundancy, fh_uncertainty, lh_uncertainty, pearson};
   }
}  // namespace lz
