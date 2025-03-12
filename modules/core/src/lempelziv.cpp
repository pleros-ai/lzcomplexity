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
         } else if (seq_size > 1e8) {
            chunks = 1000;
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

      LZ_Data lz76Factorization(const sequence& text) {
         return internal::lz76Factorization(text, getDefaultArgs(text));
      };

      LZ_Data lz76Factorization(const sequence& text, utils::LZ_Args args) {
         lz_uint   factorization = 0x01;
         lz_int    half_lz       = 0x01;
         lz_double epsilon       = 0;
         lz_double stddev        = 0;

         if (internal::CheckCharDensity(text).size() > 1) {
            internal::LempelZiv76 L;
            // factorization, factors, epsilon and factors stddev
            L.Factorize(text, args);
            factorization = L.getFactorization();
            epsilon       = L.getEpsilon();
            stddev        = L.getStddev();

            auto pos = std::find_if(
               PAR L.getFactorsBegin(), L.getFactorsEnd(), [&](auto val) { return val > text.size() / 2; });

            half_lz = std::distance(L.getFactorsBegin(), pos) - 1;
         }

         return {factorization, half_lz, stddev, epsilon};
      };

      sequence MergeSequences(sequence s1, sequence s2) {
         std::vector<char>           seq;
         std::map<std::string, char> new_alphabet;
         const lz_size               max_iter = std::min(s1.size(), s2.size());
         // for now both sequence have the same alphabet
         for (auto i = 0ul; i < max_iter; i++) [[likely]] {
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

      std::map<char, lz_double> CheckCharDensity(const sequence& seq) {
         std::map<char, lz_double> res = seq.charDensity();

         return res;
      };
   }  // namespace internal

   utils::LempelZiv lz76(const sequence& seq, utils::LZ_Args args) {
      lz_uint              factorization = 0x01;
      lz_double            epsilon       = 0;
      lz_double            stddev        = 0;
      std::vector<lz_uint> factors{0, 1, static_cast<lz_uint>(seq.length())};

      if (internal::CheckCharDensity(seq).size() > 1) {
         internal::LempelZiv76 L;
         // factorization, factors, epsilon and factors stddev
         L.Factorize(seq, args);
         factorization = L.getFactorization();
         epsilon       = L.getEpsilon();
         stddev        = L.getStddev();
         factors       = L.getFactors();
      }

      const auto log_base = args.log_base == NO_ALPHABET ? seq.getAlphabetSize() : args.log_base;

      lz_double div = static_cast<double>(seq.size()) / utils::log(seq.size(), log_base);
      // entropy density
      auto entropy = factorization / div;
      // random shuffle complexity (rsc)
      utils::LZ_Shuffle rsc;
      auto              rsc_fun = [&rsc, &seq, &args]() { rsc = lz76RandomShuffleComplexity(seq, args); };
      // whole sequence random shuffle complexity (w_rsc -- optional)
      utils::LZ_Shuffle w_rsc;
      auto              w_rsc_fun = [&seq, &args, &factorization]() {
         auto [H_rand, mm]       = ShuffleFactorization(seq, args);
         utils::LZ_Shuffle w_rsc = ShuffleEntropyCalculation(seq, args, factorization, H_rand, mm);
      };
      // Extras
      utils::LZ_Extra extras;
      auto            extra_fun = [&]() { extras = lz76ExtraMeasures(seq, args); };

      utils::par_do(rsc_fun, w_rsc_fun, extra_fun);

      // Error for Normal distribution factors length
      div               = std::sqrt(seq.size() / utils::log(seq.size(), log_base));
      auto normal_error = std::sqrt(entropy * entropy * entropy) * stddev / div;
      // Error for Poison distribution factors length
      auto poison_error = entropy / seq.size();

      return {factorization, factors, entropy, w_rsc, rsc, normal_error, poison_error, epsilon, stddev, extras};
   }

   //-------------------- Factorization functions ----------------------------//
   lz_uint lz76Factorization(const sequence& text, utils::LZ_Args args) {
      lz_uint factorization = 0x01;

      if (internal::CheckCharDensity(text).size() > 1) {
         internal::LempelZiv76 L;
         L.Factorize(text, args);
         factorization = L.getFactorization();
      }
      return factorization;
   };

   internal::LZ_Result lz76Factors(const sequence& text, utils::LZ_Args args) {
      lz_uint              factorization = 0x01;
      lz_double            epsilon       = 0;
      std::vector<lz_uint> factors{0, 1, static_cast<lz_uint>(text.length())};

      if (internal::CheckCharDensity(text).size() > 1) {
         internal::LempelZiv76 L;
         // factorization, factors, epsilon and factors stddev
         L.Factorize(text, args);
         factorization = L.getFactorization();
         epsilon       = L.getEpsilon();
         factors       = L.getFactors();
      }

      return {factorization, epsilon, factors};
   };

   //-------------------- Entropy density functions ----------------------------//
   lz_double lz76EntropyDensity(const sequence& text, utils::LZ_Args args) {
      auto factorization = lz76Factorization(text, args);

      const auto log_base = args.log_base == NO_ALPHABET ? text.getAlphabetSize() : args.log_base;
      // lz_double div = text.size() * std::log(text.getAlphabetSize()) / std::log(text.size());
      lz_double div = static_cast<double>(text.size()) / utils::log(text.size(), log_base);

      lz_double res = factorization / div;
      return res;
   }

   //-------------------- Excess of entropy functions ----------------------------//
   lz_int lz76EffectiveComplexity(const sequence& text, utils::LZ_Args args) {
      std::vector<char>::size_type mid = text.size() / 2;  // the half of the sequence

      lz_int C_, C_fh, C_lh = 0;
      auto   sequences = text.Split(mid);
      auto   new_seq   = internal::MergeSequences(sequences.first, sequences.second);

      auto fh_fun  = [&]() { C_fh = lz76Factorization(sequences.first, args); };
      auto lh_fun  = [&]() { C_lh = lz76Factorization(sequences.second, args); };
      auto all_fun = [&C_, &new_seq, &args]() {
         auto cpy     = args;
         cpy.alphabet = new_seq.getAlphabetSize();
         cpy.log_base = new_seq.getAlphabetSize();
         C_           = lz76Factorization(new_seq, cpy);
      };

      utils::par_do(all_fun, fh_fun, lh_fun);
      return C_fh + C_lh - 2 * C_;
   };

   lz_double lz76EffectiveComplexityNormalized(const sequence& text, utils::LZ_Args args) {
      const auto log_base = args.log_base == NO_ALPHABET ? text.getAlphabetSize() : args.log_base;
      const auto alphabet = args.alphabet == NO_ALPHABET ? text.getAlphabetSize() : args.alphabet;

      auto excess = lz76EffectiveComplexity(text, args);
      auto N      = text.size() / 2;
      auto div    = (N * utils::log(alphabet, log_base)) / utils::log(N, log_base);

      return excess / div;
   }

   utils::LZ_Shuffle lz76RandomShuffleComplexitySequential(const sequence& str, utils::LZ_Args args) {
      std::size_t mm = args.block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
         mm += 10;                                // begin aggressive
      }

      const auto        log_base = args.log_base == NO_ALPHABET ? str.getAlphabetSize() : args.log_base;
      const auto        alphabet = args.alphabet == NO_ALPHABET ? str.getAlphabetSize() : args.alphabet;
      utils::LZ_Shuffle result;
      result.max_block_size    = mm;
      lz_double excess_entropy = 0;
      auto      complex        = lz76Factorization(str, args);

      for (unsigned int m = 1; m <= mm; m++) {
         sequence rand_seq = Shuffle(
            str, m, str.size() / 2);  // Shuffling is made for half the size of the sequence, hope that is enough
         std::vector<unsigned int> slzf;
         auto                      rand_complexity = lz76Factorization(rand_seq, args);
         lz_double                 ee_term         = utils::log(str.size(), log_base) *
                             std::fabs((lz_double)rand_complexity - (lz_double)complex) /
                             (str.size() * utils::log(alphabet, log_base));

         excess_entropy += ee_term;

         if (args.get_shuffle_terms)
            result.summands.push_back(ee_term);
         if (m == 1)
            result.multi_information = ee_term;
      }

      result.excess_value = excess_entropy;
      return result;
   }

   std::pair<std::vector<lz_int>, lz_size> ShuffleFactorization(const sequence& str, utils::LZ_Args args) {
      lz_int mm = args.block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
         mm += str.size() > 50 ? 10 : 0;          // begin aggressive
      }

      std::vector<lz_int> res(mm + 3);  // Reserve the number of blocks for shuffling + 3

      auto fun = [&res, str, args](lz_size idx) {
         sequence rand_seq = Shuffle(str, idx, str.size() / 2);  // Shuffling is made for half the
                                                                 // size of the sequence, hope that is enough
         auto rand_complexity = lz76Factorization(rand_seq, args);
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

      if (args.get_shuffle_terms) {
         result.summands = std::vector<lz_double>(mm);
      }

      const auto log_base = args.log_base == NO_ALPHABET ? str.getAlphabetSize() : args.log_base;
      const auto alphabet = args.alphabet == NO_ALPHABET ? str.getAlphabetSize() : args.alphabet;

      auto body = [&](const auto& rng, lz_double init) -> lz_double {
         for (auto idx = rng.begin(); idx != rng.end(); idx++) {
            auto term = utils::log(str.size(), log_base) * std::fabs((lz_double)H_rand[idx] - (lz_double)complexity) /
                        (str.size() * utils::log(alphabet, log_base));
            init += term;

            if (args.get_shuffle_terms) {
               result.summands[idx - 1] = term;
            }

            if (idx == 1)
               result.multi_information = term;
         }
         return init;
      };
      auto reduce = [](const lz_double& a, const lz_double& b) -> lz_double { return a + b; };

      // result.excess_value = tbb::parallel_reduce(tbb_range, 0.0, body, reduce);
      result.excess_value = utils::parallel_reduce(1, mm + 1, 0.0, body, reduce);
      return result;
   }

   utils::LZ_Shuffle
      ShuffleEntropyCalculation(const sequence& str, const utils::LZ_Args args, const lz_int complexity, lz_int mm) {
      utils::LZ_Shuffle result;
      result.max_block_size = mm;

      if (args.get_shuffle_terms) {
         result.summands = std::vector<lz_double>(mm);
      }

      const auto log_base = args.log_base == NO_ALPHABET ? str.getAlphabetSize() : args.log_base;
      const auto alphabet = args.alphabet == NO_ALPHABET ? str.getAlphabetSize() : args.alphabet;

      auto body = [&](const auto& rng, lz_double init) -> lz_double {
         for (auto idx = rng.begin(); idx != rng.end(); idx++) {
            sequence rand_seq = Shuffle(str, idx, str.size() / 2);
            auto     c_rand   = lz76Factorization(rand_seq, args);

            auto term = utils::log(str.size(), log_base) * std::fabs((lz_double)c_rand - (lz_double)complexity) /
                        (str.size() * utils::log(alphabet, log_base));
            init += term;

            if (args.get_shuffle_terms) {
               result.summands[idx - 1] = term;
            }

            if (idx == 1)
               result.multi_information = term;
         }
         return init;
      };
      auto reduce = [](const lz_double& a, const lz_double& b) -> lz_double { return a + b; };

      // result.excess_value = tbb::parallel_reduce(tbb_range, 0.0, body, reduce);
      result.excess_value = utils::parallel_reduce(1, mm + 1, 0.0, body, reduce);
      return result;
   }

   utils::LZ_Shuffle lz76PairedShuffleComplexity(const sequence& str, utils::LZ_Args args) {
      auto [past, future] = str.Split(str.size() / 2);
      auto new_seq        = internal::MergeSequences(past, future);

      // lz_int mm = args.block_size;
      // if (mm <= 0) {
      //    mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
      //    mm += 10;                                // begin aggressive
      // }

      std::pair<std::vector<lz_int>, lz_size> random_run;
      // std::vector<lz_int>                     H_rand;
      lz_int complexity;

      // args.alphabet = new_seq.getAlphabetSize();
      // args.log_base = new_seq.getAlphabetSize();

      auto factor_fun = [&complexity, new_seq, args]() { complexity = lz76Factorization(new_seq, args); };
      auto rand_fun   = [&random_run, new_seq, args]() { random_run = ShuffleFactorization(new_seq, args); };

      utils::par_do(factor_fun, rand_fun);
      // complexity = lz76Factorization(new_seq, args);

      // for (int i = 1; i <= mm; i++) {
      //    sequence rand_seq = Shuffle(new_seq, i, new_seq.size() / 2);  // Shuffling is made for half the
      //                                                                  // size of the sequence, hope that is enough
      //    auto rand_complexity = lz76Factorization(rand_seq, args);
      //    H_rand.push_back(rand_complexity);
      // }

      auto [H_rand, mm] = random_run;

      return ShuffleEntropyCalculation(new_seq, args, complexity, H_rand, mm);
   }

   utils::LZ_Shuffle lz76RandomShuffleComplexity(const sequence& str, utils::LZ_Args args) {
      std::pair<std::vector<lz_int>, lz_size> random_run;
      lz_int                                  complexity;

      auto factor_fun = [&]() { complexity = lz76Factorization(str, args); };
      auto rand_fun   = [&]() { random_run = ShuffleFactorization(str, args); };

      utils::par_do(factor_fun, rand_fun);

      auto [H_rand, mm] = random_run;

      return ShuffleEntropyCalculation(str, args, complexity, H_rand, mm);
   }

   lz_double lz76ExcessEntropyDistance(const sequence& str, utils::LZ_Args args) {
      std::vector<char>::size_type mid = str.size() / 2;  // the half of the sequence

      lz_double dist          = 0;
      lz_int    fh_complexity = 0;
      lz_int    lh_complexity = 0;
      auto      par_seq       = str.Split(mid);

      auto fh_fun   = [&]() { fh_complexity = lz76Factorization(par_seq.first, args); };
      auto lh_fun   = [&]() { lh_complexity = lz76Factorization(par_seq.second, args); };
      auto dist_fun = [&]() { dist = lz76InformationDistance(par_seq.first, par_seq.second, args); };

      utils::par_do(dist_fun, fh_fun, lh_fun);

      return (1.0 - dist) * std::fmax(fh_complexity, lh_complexity);
   }

   //-------------------- Distance functions ----------------------------//
   lz_double lz76InformationDistance(const sequence&            T1,
                                     const internal::LZ_Result& R1,
                                     const sequence&            T2,
                                     const internal::LZ_Result& R2) {
      lz_int C_all = 0;

      C_all = lz76Factorization(T1 + T2);

      auto res =
         (C_all - std::fmin(R1.factorization, R2.factorization)) / std::fmax(R1.factorization, R2.factorization);

      return res;
   }

   lz_double lz76InformationDistance(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      lz_int C_t1, C_t2, C_all = 0;

      auto fh_fun = [&C_t1, &T1, &args]() { C_t1 = lz76Factorization(T1, args); };

      auto lh_fun = [&C_t2, &T2, &args]() { C_t2 = lz76Factorization(T2, args); };

      auto all_fun = [&C_all, &T1, &T2, &args]() { C_all = lz76Factorization(T1 + T2, args); };

      utils::par_do(fh_fun, lh_fun, all_fun);

      auto res = (C_all - std::fmin(C_t1, C_t2)) / std::fmax(C_t1, C_t2);

      return res;
   }

   // Mutual information using shuffle of concatenated sequence
   lz_double MutualInformation(const sequence& s1, const sequence& s2, utils::LZ_Args args) {
      std::pair<std::vector<lz_int>, lz_size> random_run;
      lz_uint                                 complexity;
      auto                                    s_ = s1 + s2;

      auto factor_fun = [&]() { complexity = lz76Factorization(s_, args); };
      auto rand_fun   = [&]() { random_run = ShuffleFactorization(s_, args); };

      utils::par_do(factor_fun, rand_fun);

#if __cplusplus >= 201703L
      lz_double random_sum = std::reduce(PAR random_run.first.begin(), random_run.first.end());
#else
      auto random_sum = std::accumulate(PAR random_run.first.begin(), random_run.first.end(), 0.0);
#endif
      return 1.0 - ((lz_double)random_run.second * (lz_double)complexity) / random_sum;
   }

   lz_double lz76RandomShuffleDistance(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      auto MI = MutualInformation(T1, T2, args);
      return 1 - MI;
   }

   // Mutual information using shuffle of merged (Z) sequence
   lz_double MutualInformationZ(const sequence& s1, const sequence& s2, utils::LZ_Args args) {
      auto                                    seq = internal::MergeSequences(s1, s2);
      std::pair<std::vector<lz_int>, lz_size> random_run;
      lz_uint                                 complexity;

      auto s1_c = lz76Factorization(s1, args);
      auto s2_c = lz76Factorization(s2, args);

      // args.alphabet = seq.getAlphabetSize();
      // args.log_base = seq.getAlphabetSize();

      auto factor_fun = [&]() { complexity = lz76Factorization(seq, args); };
      auto rand_fun   = [&]() { random_run = ShuffleFactorization(seq, args); };

      utils::par_do(factor_fun, rand_fun);

#if __cplusplus >= 201703L
      lz_double random_sum = std::reduce(PAR random_run.first.begin(), random_run.first.end());
#else
      auto random_sum = std::accumulate(PAR random_run.first.begin(), random_run.first.end(), 0.0);
#endif
      return 1.0 - ((lz_double)random_run.second * (lz_double)complexity) / random_sum;
   }

   lz_double lz76RandomShuffleDistanceZ(const sequence& T1, const sequence& T2, utils::LZ_Args args) {
      auto MI = MutualInformationZ(T1, T2, args);
      return 1 - MI;
   }

   //-------------------- Other distance functions ----------------------------//
   lz_double lz76NormalError(const sequence& seq, utils::LZ_Args args) {
      internal::LempelZiv76 L;
      L.Factorize(seq, args);

      const auto log_base = args.log_base == NO_ALPHABET ? seq.getAlphabetSize() : args.log_base;
      const auto alphabet = args.alphabet == NO_ALPHABET ? seq.getAlphabetSize() : args.alphabet;

      auto entropy = L.getFactorization() * utils::log(seq.size(), log_base) / static_cast<double>(seq.size());

      auto entropy_c = entropy * entropy * entropy;
      auto num       = std::sqrt(entropy_c * utils::log(seq.size(), log_base) / seq.size());

      return num * L.getStddev();
   }

   lz_double lz76PoisonError(const sequence& seq, utils::LZ_Args args) {
      auto entropy = lz76EntropyDensity(seq, args);
      return entropy / seq.size();
   }

   //-------------------------------- Extras --------------------------------//
   utils::LZ_Extra lz76ExtraMeasures(const sequence& str, utils::LZ_Args args) {
      auto              mid           = str.size() / 2;  // the half of the sequence
      auto              par_seq       = str.Drop(mid);
      lz_uint           lh_complexity = 0;
      internal::LZ_Data lz;

      auto lz_fun = [&lz, &str, &args]() { lz = internal::lz76Factorization(str, args); };
      auto lh_fun = [&lh_complexity, &par_seq, &args]() { lh_complexity = lz76Factorization(par_seq, args); };

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
