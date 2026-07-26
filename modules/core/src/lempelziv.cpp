/***************************************************************************
                          lempelziv.cpp  -  description
                             -------------------
    begin                : 29 Oct 2023
    email                : efrenaragon96@gmail.com
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013-2022 by Efren Aragon Perez   			    	         *
 *   efrenaragon96@gmail.com          *
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

#include "lz/general.h"
#include "lz/sequence.h"

#ifdef _LIBCPP_HAS_PARALLEL_ALGORITHMS
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

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
      lz_int    half_lz = 0x01;
      lz_double epsilon = 0;
      lz_double stddev = 0;

      if (internal::CheckCharDensity(text).size() > 1) {
        internal::LempelZiv76 L;
        // factorization, factors, epsilon and factors stddev
        L.Factorize(text, args);
        // L.Factorize(&text.SequenceVector()[0], text.size());
        factorization = L.getFactorization();
        epsilon = L.getEpsilon();
        stddev = L.getStddev();

        auto pos = std::find_if(
          PAR L.getFactorsBegin(), L.getFactorsEnd(), [&](auto val) { return val > text.size() / 2; });

        half_lz = std::distance(L.getFactorsBegin(), pos) - 1;
      }

      return {factorization, half_lz, stddev, epsilon};
    };

    sequence MergeSequences(const sequence& s1, const sequence& s2) {
      const auto  max_iter = std::min(s1.size(), s2.size());
      std::string seq;
      seq.reserve(max_iter);

      // Use 16-bit packed keys: high byte = s1[i], low byte = s2[i]
      std::array<char, 65536> lookup{};  // 256*256 possible pairs
      char                    next_symbol = '0';

      for (lz_size i = 0; i < max_iter; ++i) [[likely]] {
        const auto key = static_cast<uint16_t>((static_cast<unsigned char>(s1[i]) << 8)
                                               | static_cast<unsigned char>(s2[i]));

        char& mapped = lookup[key];
        if (mapped == 0) [[unlikely]] {
          mapped = next_symbol++;
        }
        seq.push_back(mapped);
      }

      return sequence{std::move(seq), s1.getAlphabetSize() * s2.getAlphabetSize()};
    }

    std::map<char, lz_uint> CheckCharDensity(const sequence& seq) {
      std::map<char, lz_uint> res = seq.charDensity();

      return res;
    };
  }  // namespace internal

  utils::LempelZiv lz76(const sequence& seq, utils::LZ_Args& args) {
    lz_uint              factorization = 0x01;
    lz_double            epsilon = 0;
    lz_double            stddev = 0;
    std::vector<lz_uint> factors{0, 1, static_cast<lz_uint>(seq.length())};

    if (internal::CheckCharDensity(seq).size() > 1) [[likely]] {
      internal::LempelZiv76 L;
      // factorization, factors, epsilon and factors stddev
      L.Factorize(seq, args);
      // L.Factorize(&seq.SequenceVector()[0], seq.size());
      factorization = L.getFactorization();
      epsilon = L.getEpsilon();
      stddev = L.getStddev();
      factors = L.getFactors();
    }

    const auto log_base = args.log_base == details::NO_ALPHABET ? seq.getAlphabetSize() : args.log_base;

    lz_double div = static_cast<double>(seq.size()) / utils::log(seq.size(), log_base);
    // entropy density
    auto entropy = factorization / div;
    // random shuffle complexity (rsc)
    // utils::LZ_Shuffle p_rsc;
    // auto              p_rsc_fun = [&p_rsc, &seq, &args]() { p_rsc = lz76PairedShuffleComplexity(seq, args);
    // }; whole sequence random shuffle complexity (w_rsc -- optional)
    utils::LZ_Shuffle w_rsc;
    auto              w_rsc_fun = [&seq, &args, &factorization, &w_rsc]() {
      auto [H_rand, mm] = ShuffleFactorization(seq, args);
      w_rsc = ShuffleEntropyCalculation(seq, args, factorization, H_rand, mm);
    };
    // Extras
    utils::LZ_Extra extras;
    auto            extra_fun = [&]() { extras = lz76ExtraMeasures(seq, args); };

    utils::par_do(w_rsc_fun, extra_fun);

    // Error for Normal distribution factors length
    div = std::sqrt(seq.size() / utils::log(seq.size(), log_base));
    auto normal_error = std::sqrt(entropy * entropy * entropy) * stddev / div;
    // Error for Poison distribution factors length
    auto poison_error = entropy / seq.size();

    return {factorization, factors, entropy, w_rsc, {}, normal_error, poison_error, epsilon, stddev, extras};
  }

  //-------------------- Factorization functions ----------------------------//
  lz_uint lz76Factorization(const sequence& text, utils::LZ_Args& args) {
    lz_uint factorization = 0x01;

    if (internal::CheckCharDensity(text).size() > 1) [[likely]] {
      internal::LempelZiv76 L;
      L.Factorize(text, args);
      // L.Factorize(&text.SequenceVector()[0], text.size());
      factorization = L.getFactorization();
    }
    return factorization;
  };

  internal::LZ_Result lz76Factors(const sequence& text, utils::LZ_Args& args) {
    lz_uint              factorization = 0x01;
    lz_double            epsilon = 0;
    std::vector<lz_uint> factors{0, 1, static_cast<lz_uint>(text.length())};

    if (internal::CheckCharDensity(text).size() > 1) [[likely]] {
      internal::LempelZiv76 L;
      // factorization, factors, epsilon and factors stddev
      L.Factorize(text, args);
      // L.Factorize(&text.SequenceVector()[0], text.size());
      factorization = L.getFactorization();
      epsilon = L.getEpsilon();
      factors = L.getFactors();
    }

    return {factorization, epsilon, factors};
  };

  //-------------------- Entropy density functions ----------------------------//
  lz_double lz76EntropyDensity(const sequence& text, utils::LZ_Args& args) {
    auto factorization = lz76Factorization(text, args);

    const auto log_base = args.log_base == details::NO_ALPHABET ? text.getAlphabetSize() : args.log_base;
    // lz_double div = text.size() * std::log(text.getAlphabetSize()) / std::log(text.size());
    lz_double div = static_cast<double>(text.size()) / utils::log(text.size(), log_base);

    lz_double res = factorization / div;
    return res;
  }

  //-------------------- Excess of entropy functions ----------------------------//
  lz_int lz76EffectiveComplexity(const sequence& text, utils::LZ_Args& args) {
    const auto mid = text.size() / 2;  // the half of the sequence

    lz_int   C_ = 0, C_fh = 0, C_lh = 0;
    auto     split = text.Split(mid);
    sequence first_half = split.first;
    sequence second_half = split.second;
    auto     new_seq = internal::MergeSequences(first_half, second_half);

    auto fh_fun = [&first_half, &args, &C_fh]() { C_fh = lz76Factorization(first_half, args); };
    auto lh_fun = [&second_half, &args, &C_lh]() { C_lh = lz76Factorization(second_half, args); };
    auto all_fun = [&C_, &new_seq, &args]() {
      auto cpy = args;
      cpy.alphabet = new_seq.getAlphabetSize();
      cpy.log_base = new_seq.getAlphabetSize();
      C_ = lz76Factorization(new_seq, cpy);
    };

    utils::par_do(all_fun, fh_fun, lh_fun);
    return C_fh + C_lh - 2 * C_;
  };

  lz_double lz76EffectiveComplexityNormalized(const sequence& text, utils::LZ_Args& args) {
    const auto log_base = args.log_base == details::NO_ALPHABET ? text.getAlphabetSize() : args.log_base;
    const auto alphabet = args.alphabet == details::NO_ALPHABET ? text.getAlphabetSize() : args.alphabet;

    auto excess = lz76EffectiveComplexity(text, args);
    auto N = text.size() / 2;
    auto div = (N * utils::log(alphabet, log_base)) / utils::log(N, log_base);

    return excess / div;
  }

  namespace {

    /**
     * @brief L2 projection of @p y onto non-negative, non-decreasing sequences.
     *
     * Pool-adjacent-violators (Ayer, Brunk, Ewing, Reid & Silverman, *Ann. Math.
     * Statist.* 26:641, 1955): walk left to right over `(sum, count)` blocks and
     * merge backwards whenever the newest block averages below its predecessor.
     * The non-negativity projection is then the pointwise positive part, which
     * preserves monotonicity. Mirrors `non_negative_isotonic` in the Rust backend.
     */
    std::vector<lz_double> non_negative_isotonic(const std::vector<lz_double>& y) {
      std::vector<lz_double>   sums;
      std::vector<std::size_t> counts;
      sums.reserve(y.size());
      counts.reserve(y.size());

      for (const lz_double v : y) {
        sums.push_back(v);
        counts.push_back(1);
        while (sums.size() > 1) {
          const std::size_t k = sums.size();
          const lz_double   last = sums[k - 1] / static_cast<lz_double>(counts[k - 1]);
          const lz_double   prev = sums[k - 2] / static_cast<lz_double>(counts[k - 2]);
          if (prev <= last) break;
          sums[k - 2] += sums[k - 1];
          counts[k - 2] += counts[k - 1];
          sums.pop_back();
          counts.pop_back();
        }
      }

      std::vector<lz_double> out;
      out.reserve(y.size());
      for (std::size_t b = 0; b < sums.size(); ++b) {
        const lz_double level = std::max(0.0, sums[b] / static_cast<lz_double>(counts[b]));
        out.insert(out.end(), counts[b], level);
      }
      return out;
    }

    /**
     * @brief Read the reported values off the raw per-scale excess-entropy ladder.
     *
     * Each rung is `E(l) = H_l - l*h_hat = l*g*(C_LZ(u^{RS(l)}) - C_LZ(u))`, an
     * estimate of the excess entropy accumulated up to scale `l`. Theory says that
     * ladder is non-negative and non-decreasing, because every increment `h(l) - h`
     * is non-negative (Crutchfield & Feldman, *Chaos* 13:25 (2003), Lemma 1 and
     * Eq. 52). The raw rungs are neither, because each rests on its own surrogate
     * draw -- so project, then report the projection's increments as `summands` and
     * its top as `emc_value`.
     *
     * Do not sum the raw first differences `(H_l - H_{l-1}) - h_hat` instead. That
     * is the identity `sum_l [h(l) - h] = H(L) - L*h`, so the terms telescope and
     * the total collapses to the largest block size alone: able to go negative, and
     * exactly zero whenever the source period divides `mm`.
     *
     * `multi_information` stays the *raw* scale-1 rung -- a separate diagnostic,
     * not a term of the sum.
     */
    void ReadOffLadder(utils::LZ_Shuffle& result, const std::vector<lz_double>& raw, bool want_terms) {
      if (raw.empty()) return;
      result.multi_information = raw.front();

      const std::vector<lz_double> fitted = non_negative_isotonic(raw);
      lz_double                    prev = 0.0;  // E_fit(0) = 0
      for (std::size_t i = 0; i < fitted.size(); ++i) {
        const lz_double term = fitted[i] - prev;
        if (want_terms) result.summands[i] = term;
        prev = fitted[i];
      }
      result.emc_value = prev;  // = E_fit(mm)
    }

  }  // namespace

  utils::LZ_Shuffle lz76RandomShuffleComplexitySequential(const sequence& str, utils::LZ_Args& args) {
    lz_size mm = 0;
    if (args.block_size <= 0) {
      mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
      mm += str.size() > 50 ? 10 : 0;          // begin aggressive
    } else {
      mm = static_cast<lz_size>(args.block_size);
    }

    const auto        log_base = args.log_base == details::NO_ALPHABET ? str.getAlphabetSize() : args.log_base;
    utils::LZ_Shuffle result;
    result.max_block_size = static_cast<lz_int>(mm);

    if (args.get_shuffle_terms) [[unlikely]] {
      result.summands = std::vector<lz_double>(mm);
    }
    if (str.size() <= 1 || mm == 0) return result;

    const auto      complex = lz76Factorization(str, args);
    const lz_double g = utils::log(str.size(), log_base) / static_cast<lz_double>(str.size());
    const lz_double h_hat = static_cast<lz_double>(complex) * g;
    const auto      seed_base = rng::fnv1a(str.SequenceVector());

    std::vector<lz_double> raw(mm);
    for (lz_size m = 1; m <= mm; m++) {
      // Shuffling is made for half the size of the sequence, hope that is enough
      const sequence rand_seq
        = Shuffle(str, static_cast<lz_uint>(m), str.size() / 2, rng::seed_from_base(seed_base, m));
      const auto rand_complexity = lz76Factorization(rand_seq, args);
      raw[m - 1] = static_cast<lz_double>(m) * (static_cast<lz_double>(rand_complexity) * g - h_hat);
    }

    ReadOffLadder(result, raw, args.get_shuffle_terms);
    return result;
  }

  std::pair<std::vector<lz_int>, lz_size> ShuffleFactorization(const sequence& str, utils::LZ_Args& args) {
    lz_int mm = args.block_size;
    if (mm <= 0) {
      mm = utils::max_block_size(str.size());  // the maximum number for the sum in the entropy estimation
      mm += str.size() > 50 ? 10 : 0;          // begin aggressive
    }

    std::vector<lz_int> res(mm + 3);  // Reserve the number of blocks for shuffling + 3

    // Hash the sequence content once; each per-block seed differs only by `idx`, so
    // the same input yields the same surrogates on any machine at any thread count,
    // and the same ones the Rust backend draws.
    const auto seed_base = rng::fnv1a(str.SequenceVector());

    auto fun = [&res, &str, &args, seed_base](lz_size idx) {
      // Shuffling is made for half the size of the sequence, hope that is enough
      sequence rand_seq
        = Shuffle(str, static_cast<lz_uint>(idx), str.size() / 2, rng::seed_from_base(seed_base, idx));
      auto rand_complexity = lz76Factorization(rand_seq, args);
      res[idx] = rand_complexity;
    };

    utils::parallel_for(1, mm + 1, fun);

    return {std::move(res), mm};
  }

  utils::LZ_Shuffle ShuffleEntropyCalculation(const sequence&           str,
                                              utils::LZ_Args&           args,
                                              const lz_int              complexity,
                                              const std::vector<lz_int> H_rand,
                                              lz_int                    mm) {
    utils::LZ_Shuffle result;
    result.max_block_size = mm;

    if (args.get_shuffle_terms) {
      result.summands = std::vector<lz_double>(mm);
    }

    const auto log_base = args.log_base == details::NO_ALPHABET ? str.getAlphabetSize() : args.log_base;

    if (str.size() <= 1 || mm <= 0) return result;

    // Block-entropy estimator: g = log_k(N)/N, so the original's entropy density is
    // h_hat = C_LZ(u)*g and the block entropy at scale l is H_l = l*C_LZ(u^{RS(l)})*g.
    // The rung is the subextensive part, E(l) = H_l - l*h_hat.
    const lz_double g = utils::log(str.size(), log_base) / (lz_double)str.size();
    const lz_double h_hat = (lz_double)complexity * g;

    // The projection is a left-to-right pass, so the ladder is built in order rather
    // than reduced in parallel. It is mm doubles; the cost is nil beside the
    // factorizations that produced H_rand.
    const lz_size          len = std::min<lz_size>(static_cast<lz_size>(mm), H_rand.size() - 1);
    std::vector<lz_double> raw(len);
    for (lz_size idx = 1; idx <= len; ++idx) {
      raw[idx - 1] = (lz_double)idx * ((lz_double)H_rand[idx] * g - h_hat);
    }

    ReadOffLadder(result, raw, args.get_shuffle_terms);
    return result;
  }

  utils::LZ_Shuffle ShuffleEntropyCalculation(const sequence& str,
                                              utils::LZ_Args& args,
                                              const lz_int    complexity,
                                              lz_int          mm) {
    utils::LZ_Shuffle result;
    result.max_block_size = mm;

    if (args.get_shuffle_terms) {
      result.summands = std::vector<lz_double>(mm);
    }

    const auto log_base = args.log_base == details::NO_ALPHABET ? str.getAlphabetSize() : args.log_base;

    if (str.size() <= 1 || mm <= 0) return result;

    // Block-entropy estimator (see the H_rand overload). This variant shuffles
    // inline. Each scale now needs only its own surrogate: the projection consumes
    // the whole ladder, so there is no first difference to form and no reason to
    // re-factorize scale l-1 as the previous implementation did.
    const lz_double g = utils::log(str.size(), log_base) / (lz_double)str.size();
    const lz_double h_hat = (lz_double)complexity * g;
    const auto      seed_base = rng::fnv1a(str.SequenceVector());

    const lz_size          len = static_cast<lz_size>(mm);
    std::vector<lz_double> raw(len);
    auto                   fun = [&](lz_size idx) {
      const sequence rand_seq
        = Shuffle(str, static_cast<lz_uint>(idx), str.size() / 2, rng::seed_from_base(seed_base, idx));
      const auto c_l = lz76Factorization(rand_seq, args);
      raw[idx - 1] = (lz_double)idx * ((lz_double)c_l * g - h_hat);
    };
    utils::parallel_for(1, mm + 1, fun);

    ReadOffLadder(result, raw, args.get_shuffle_terms);
    return result;
  }

  utils::LZ_Shuffle lz76PairedShuffleComplexity(const sequence& str, utils::LZ_Args& args) {
    auto [past, future] = str.Split(str.size() / 2);
    auto new_seq = internal::MergeSequences(past, future);

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

    auto factor_fun = [&complexity, new_seq, &args]() { complexity = lz76Factorization(new_seq, args); };
    auto rand_fun = [&random_run, new_seq, &args]() { random_run = ShuffleFactorization(new_seq, args); };

    utils::par_do(factor_fun, rand_fun);
    // complexity = lz76Factorization(new_seq, args);

    // for (int i = 1; i <= mm; i++) {
    //    sequence rand_seq = Shuffle(new_seq, i, new_seq.size() / 2);  // Shuffling is made for half the
    //                                                                  // size of the sequence, hope that is
    //                                                                  enough
    //    auto rand_complexity = lz76Factorization(rand_seq, args);
    //    H_rand.push_back(rand_complexity);
    // }

    auto [H_rand, mm] = random_run;

    return ShuffleEntropyCalculation(new_seq, args, complexity, H_rand, mm);
  }

  utils::LZ_Shuffle lz76RandomShuffleComplexity(const sequence& str, utils::LZ_Args& args) {
    std::pair<std::vector<lz_int>, lz_size> random_run;
    lz_int                                  complexity;

    auto factor_fun = [&]() { complexity = lz76Factorization(str, args); };
    auto rand_fun = [&]() { random_run = ShuffleFactorization(str, args); };

    utils::par_do(factor_fun, rand_fun);

    auto [H_rand, mm] = random_run;

    return ShuffleEntropyCalculation(str, args, complexity, H_rand, mm);
  }

  lz_double lz76ExcessEntropyDistance(const sequence& str, utils::LZ_Args& args) {
    std::vector<char>::size_type mid = str.size() / 2;  // the half of the sequence

    lz_double dist = 0;
    lz_int    fh_complexity = 0;
    lz_int    lh_complexity = 0;
    auto      par_seq = str.Split(mid);

    auto fh_fun = [&]() { fh_complexity = lz76Factorization(par_seq.first, args); };
    auto lh_fun = [&]() { lh_complexity = lz76Factorization(par_seq.second, args); };
    auto dist_fun = [&]() { dist = lz76InformationDistance(par_seq.first, par_seq.second, args); };

    utils::par_do(dist_fun, fh_fun, lh_fun);

    return (1.0 - dist) * std::fmax(fh_complexity, lh_complexity);
  }

  //-------------------- Distance functions ----------------------------//
  lz_double lz76InformationDistance(const sequence&            T1,
                                    const internal::LZ_Result& R1,
                                    const sequence&            T2,
                                    const internal::LZ_Result& R2) {
    lz_int C_xy = lz76Factorization(T1 + T2);
    lz_int C_yx = lz76Factorization(T2 + T1);

    auto res = (std::fmin(C_xy - R1.factorization, C_yx - R2.factorization))
      / std::fmax(R1.factorization, R2.factorization);

    return res;
  }

  lz_double lz76InformationDistance(const sequence& T1, const sequence& T2, utils::LZ_Args& args) {
    lz_int C_x, C_y, C_xy, C_yx = 0;

    auto fh_fun = [&C_x, &T1, &args]() { C_x = lz76Factorization(T1, args); };

    auto lh_fun = [&C_y, &T2, &args]() { C_y = lz76Factorization(T2, args); };

    auto xy_fun = [&C_xy, &T1, &T2, &args]() { C_xy = lz76Factorization(T1 + T2, args); };

    auto yx_fun = [&C_yx, &T1, &T2, &args]() { C_yx = lz76Factorization(T2 + T1, args); };

    utils::par_do(fh_fun, lh_fun, xy_fun, yx_fun);

    auto res = std::fmax(C_xy - C_x, C_yx - C_y) / std::fmax(C_x, C_y);

    return res;
  }

  // Mutual information using shuffle of concatenated sequence
  lz_double MutualInformation(const sequence& s1, const sequence& s2, utils::LZ_Args& args) {
    std::pair<std::vector<lz_int>, lz_size> random_run;
    lz_uint                                 complexity;
    auto                                    s_ = s1 + s2;

    auto factor_fun = [&]() { complexity = lz76Factorization(s_, args); };
    auto rand_fun = [&]() { random_run = ShuffleFactorization(s_, args); };

    utils::par_do(factor_fun, rand_fun);

#if __cplusplus >= 201703L
    lz_double random_sum = std::reduce(PAR random_run.first.begin(), random_run.first.end());
#else
    auto random_sum = std::accumulate(PAR random_run.first.begin(), random_run.first.end(), 0.0);
#endif
    return 1.0 - ((lz_double)random_run.second * (lz_double)complexity) / random_sum;
  }

  lz_double lz76RandomShuffleDistance(const sequence& T1, const sequence& T2, utils::LZ_Args& args) {
    auto MI = MutualInformation(T1, T2, args);
    return 1 - MI;
  }

  // Mutual information using shuffle of merged (Z) sequence
  lz_double MutualInformationZ(const sequence& s1, const sequence& s2, utils::LZ_Args& args) {
    auto                                    seq = internal::MergeSequences(s1, s2);
    std::pair<std::vector<lz_int>, lz_size> random_run;
    lz_uint                                 complexity;

    // auto s1_c = lz76Factorization(s1, args);
    // auto s2_c = lz76Factorization(s2, args);

    // args.alphabet = seq.getAlphabetSize();
    // args.log_base = seq.getAlphabetSize();

    auto factor_fun = [&]() { complexity = lz76Factorization(seq, args); };
    auto rand_fun = [&]() { random_run = ShuffleFactorization(seq, args); };

    utils::par_do(factor_fun, rand_fun);

#if __cplusplus >= 201703L
    lz_double random_sum = std::reduce(PAR random_run.first.begin(), random_run.first.end());
#else
    auto random_sum = std::accumulate(PAR random_run.first.begin(), random_run.first.end(), 0.0);
#endif
    return 1.0 - ((lz_double)random_run.second * (lz_double)complexity) / random_sum;
  }

  lz_double lz76RandomShuffleDistanceZ(const sequence& T1, const sequence& T2, utils::LZ_Args& args) {
    auto MI = MutualInformationZ(T1, T2, args);
    return 1 - MI;
  }

  //-------------------- Other distance functions ----------------------------//
  lz_double lz76NormalError(const sequence& seq, utils::LZ_Args& args) {
    internal::LempelZiv76 L;
    L.Factorize(seq, args);

    const auto log_base = args.log_base == details::NO_ALPHABET ? seq.getAlphabetSize() : args.log_base;
    // const auto alphabet = args.alphabet == details::NO_ALPHABET ? seq.getAlphabetSize() : args.alphabet;

    auto entropy = L.getFactorization() * utils::log(seq.size(), log_base) / static_cast<double>(seq.size());

    auto entropy_c = entropy * entropy * entropy;
    auto num = std::sqrt(entropy_c * utils::log(seq.size(), log_base) / seq.size());

    return num * L.getStddev();
  }

  lz_double lz76PoisonError(const sequence& seq, utils::LZ_Args& args) {
    auto entropy = lz76EntropyDensity(seq, args);
    return entropy / seq.size();
  }

  //-------------------------------- Extras --------------------------------//
  utils::LZ_Extra lz76ExtraMeasures(const sequence& str, utils::LZ_Args& args) {
    auto              mid = str.size() / 2;  // the half of the sequence
    auto              par_seq = str.Drop(mid);
    lz_uint           lh_complexity = 0;
    internal::LZ_Data lz;

    auto lz_fun = [&lz, &str, &args]() { lz = internal::lz76Factorization(str, args); };
    auto lh_fun = [&lh_complexity, &par_seq, &args]() { lh_complexity = lz76Factorization(par_seq, args); };

    utils::par_do(lz_fun, lh_fun);

    auto C_lz = static_cast<lz_double>(lz.factorization);
    auto fh_complexity = static_cast<lz_double>(lz.half_factorization);

    auto MI = fh_complexity + lh_complexity - C_lz;

    auto rajski = 2 - (fh_complexity + lh_complexity) / C_lz;
    auto fh_uncertainty = MI / fh_complexity;
    auto lh_uncertainty = MI / lh_complexity;
    auto redundancy = MI / (fh_complexity + lh_complexity);
    auto pearson = MI / std::sqrt(fh_complexity * lh_complexity);

    return {rajski, redundancy, fh_uncertainty, lh_uncertainty, pearson};
  }
}  // namespace lz
