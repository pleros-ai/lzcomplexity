#include <future>
#include <lzDistance/lzDistanceApp.hpp>

template<typename... Args>
struct overload_cast_impl {
   template<typename Return>
   constexpr auto operator()(Return (*pf)(Args...)) const noexcept -> decltype(pf) {
      return pf;
   }

   template<typename Return, typename Class>
   constexpr auto operator()(Return (Class::*pmf)(Args...), std::false_type = {}) const noexcept -> decltype(pmf) {
      return pmf;
   }

   template<typename Return, typename Class>
   constexpr auto operator()(Return (Class::*pmf)(Args...) const, std::true_type) const noexcept -> decltype(pmf) {
      return pmf;
   }
};

template<typename... Args>
static constexpr overload_cast_impl<Args...> overload_cast{};

namespace lz {
   namespace internal {
      auto canProcessTheLine(const auto idx, const auto init_rng, const auto end_rng) {
         const auto processAllLines =
            init_rng == utils::LZ_Args::ALL_LINES ||
            (init_rng == utils::LZ_Args::UNDEFINED_LINES && end_rng == utils::LZ_Args::UNDEFINED_LINES);
         const auto processOneLine = static_cast<lz_int>(idx + 1) == init_rng &&
                                     (end_rng == utils::LZ_Args::UNDEFINED_LINES || init_rng == end_rng);
         const auto processRange =
            (init_rng <= static_cast<lz_int>(idx + 1) && end_rng >= static_cast<lz_int>(idx + 1)) ||
            (init_rng <= static_cast<lz_int>(idx + 1) &&
             (end_rng == utils::LZ_Args::ALL_LINES || end_rng == utils::LZ_Args::UNDEFINED_LINES)) ||
            ((init_rng == utils::LZ_Args::ALL_LINES || init_rng == utils::LZ_Args::UNDEFINED_LINES) &&
             end_rng >= static_cast<lz_int>(idx + 1));

         return processAllLines || processOneLine || processRange;
      }

      auto DistanceMatrixDefault(dist::LZ_Flags& flags, auto& lz_dist, auto&& fun) -> lz_int {
         const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;

         const auto first_len  = flags.first_input.size();
         const auto second_len = second_input.size();

         auto for_fun = [&](auto&& idx) {
            if (!internal::canProcessTheLine(idx, flags.first_dist_init, flags.first_dist_end)) {
               return;
            }

            const auto& f_seq = flags.first_input[idx];

            for (auto j = 0ul; j < second_len; j++) {
               if (!internal::canProcessTheLine(j, flags.second_dist_init, flags.second_dist_end)) {
                  continue;
               }

               auto s_seq = second_input[j];
               auto res   = fun(f_seq, s_seq, flags.sa_args);

               lz_dist[idx][j] = res;
            }
         };

         utils::parallel_for(0ul, first_len, for_fun);
         return EXIT_SUCCESS;
      }

      auto DistanceMatrixRevert(dist::LZ_Flags& flags, auto& lz_dist, auto&& fun) -> lz_int {
         const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;

         const auto first_len  = flags.first_input.size();
         const auto second_len = second_input.size();

         auto for_fun = [&](auto&& idx) {
            if (!internal::canProcessTheLine(idx, flags.first_dist_init, flags.first_dist_end)) {
               return;
            }

            const auto& f_seq = flags.first_input[idx];

            for (auto j = 0ul; j < second_len; j++) {
               if (!internal::canProcessTheLine(j, flags.second_dist_init, flags.second_dist_end)) {
                  return;
               }

               auto s_seq = second_input[j];
               auto res   = fun(f_seq, s_seq.reverse(), flags.sa_args);

               lz_dist[idx][j] = res;
            }
         };

         utils::parallel_for(0ul, first_len, for_fun);
         return EXIT_SUCCESS;
      }

      auto DistanceMatrixBinary(dist::LZ_Flags& flags, auto& lz_dist, auto&& fun) -> lz_int {
         const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;

         const auto first_len  = flags.first_input.size();
         const auto second_len = second_input.size();

         auto for_fun = [&](auto&& idx) {
            if (!internal::canProcessTheLine(idx, flags.first_dist_init, flags.first_dist_end)) {
               return;
            }

            const auto& f_seq = flags.first_input[idx];

            for (auto j = 0ul; j < second_len; j++) {
               if (!internal::canProcessTheLine(j, flags.second_dist_init, flags.second_dist_end)) {
                  return;
               }

               const auto& s_seq = second_input[j];
               lz_double   dfl_res, inv_res, dfl_revert_res, inv_revert_res;

               auto mapped_seq = s_seq.map([](auto c) { return c == '0' ? '1' : '0'; });

               auto dfl_fun        = [&]() { dfl_res = fun(f_seq, s_seq, flags.sa_args); };
               auto inv_fun        = [&]() { inv_res = fun(f_seq, s_seq.reverseCopy(), flags.sa_args); };
               auto dfl_revert_fun = [&]() { dfl_revert_res = fun(f_seq, mapped_seq, flags.sa_args); };
               auto inv_revert_fun = [&]() { inv_revert_res = fun(f_seq, mapped_seq.reverseCopy(), flags.sa_args); };

               utils::par_do(dfl_fun, inv_fun, dfl_revert_fun, inv_revert_fun);

               auto res = std::min({dfl_res, inv_res, dfl_revert_res, inv_revert_res});

               lz_dist[idx][j] = res;
            }
         };

         utils::parallel_for(0ul, first_len, for_fun);
         return EXIT_SUCCESS;
      }

      auto DistanceMatrixADN(dist::LZ_Flags& flags, auto& lz_dist, auto&& fun) -> lz_int {
         std::unordered_map<char, char> ADN_MAP = {{'A', 'T'}, {'C', 'G'}, {'G', 'C'}, {'T', 'A'}};
         std::unordered_map<char, char> ARN_MAP = {{'A', 'U'}, {'C', 'G'}, {'G', 'C'}, {'U', 'A'}};

         const auto change_par = [&ADN_MAP, &ARN_MAP, &flags](char r_char, char ch) -> char {
            if (flags.adn_) {
               const auto par        = ADN_MAP[ch];
               const char upper_char = std::toupper(r_char);
               return upper_char == ch || upper_char == par ? std::tolower(ADN_MAP[upper_char]) : r_char;
            } else if (flags.arn_) {
               const auto par        = ARN_MAP[ch];
               const char upper_char = std::toupper(r_char);
               return upper_char == ch || upper_char == par ? std::tolower(ARN_MAP[upper_char]) : r_char;
            } else {
               return r_char;
            }
         };

         const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;

         const auto first_len  = flags.first_input.size();
         const auto second_len = second_input.size();

         auto for_fun = [&](auto&& idx) {
            if (!internal::canProcessTheLine(idx, flags.first_dist_init, flags.first_dist_end)) {
               return;
            }

            const auto& f_seq = flags.first_input[idx];

            for (auto j = 0ul; j < second_len; j++) {
               if (!internal::canProcessTheLine(j, flags.second_dist_init, flags.second_dist_end)) {
                  return;
               }

               const auto& s_seq = second_input[j];
               lz_double   dfl_res, inv_res, AT_res, AT_inv_res, CG_res, CG_inv_res;

               auto mappedAT_seq = s_seq.map([&](auto c) { return change_par(c, 'A'); });
               auto mappedCG_seq = s_seq.map([&](auto c) { return change_par(c, 'A'); });

               auto dfl_fun       = [&]() { dfl_res = fun(f_seq, s_seq, flags.sa_args); };
               auto inv_fun       = [&]() { inv_res = fun(f_seq, s_seq.reverseCopy(), flags.sa_args); };
               auto AT_fun        = [&]() { AT_res = fun(f_seq, mappedAT_seq, flags.sa_args); };
               auto AT_revert_fun = [&]() { AT_inv_res = fun(f_seq, mappedAT_seq.reverseCopy(), flags.sa_args); };
               auto CG_fun        = [&]() { CG_res = fun(f_seq, mappedCG_seq, flags.sa_args); };
               auto CG_revert_fun = [&]() { CG_inv_res = fun(f_seq, mappedCG_seq.reverse(), flags.sa_args); };

               utils::par_do(dfl_fun, inv_fun, AT_fun, AT_revert_fun, CG_fun, CG_revert_fun);

               auto res = std::min({dfl_res, inv_res, AT_res, AT_inv_res, CG_res, CG_inv_res});

               lz_dist[idx][j] = res;
            }
         };

         utils::parallel_for(0ul, first_len, for_fun);
         return EXIT_SUCCESS;
      }

      auto DistanceMatrixTrajectory(dist::LZ_Flags& flags, auto& lz_dist, auto&& fun) -> lz_int {
         const std::vector<char> trajectories{'1', '2', '3', '4', '5', '6', '7', '8'};

         const auto& second_input = flags.second_input.empty() ? flags.first_input : flags.second_input;

         const auto first_len  = flags.first_input.size();
         const auto second_len = second_input.size();

         auto tr_fun = [&](auto&& idx) {
            if (!internal::canProcessTheLine(idx, flags.first_dist_init, flags.first_dist_end)) {
               return;
            }

            for (auto j = 0ul; j < second_len; j++) {
               if (!internal::canProcessTheLine(j, flags.second_dist_init, flags.second_dist_end)) {
                  return;
               }

               std::vector<std::future<void>> functions;
               std::vector<lz_double>         results(2 * trajectories.size());

               const auto& f_seq        = flags.first_input[idx];
               auto        inner_tr_fun = [&](lz_size& tr_idx) {
                  const auto& s_seq = second_input[j].map([&](lz_char x) {
                     lz_int val = x - '0';
                     return trajectories[(val + tr_idx - 1) % trajectories.size()];
                  });

                  results[2 * tr_idx]     = fun(f_seq, s_seq, flags.sa_args);
                  results[2 * tr_idx + 1] = fun(f_seq, s_seq.reverseCopy(), flags.sa_args);
               };

               for (auto i = 0ul; i < trajectories.size(); i++) {
                  functions.push_back(std::async(std::launch::async, [&]() { inner_tr_fun(i); }));
               }

               for (auto& th: functions)
                  th.get();

               auto res = *std::min_element(results.begin(), results.end());

               lz_dist[idx][j] = res;
            }
         };

         utils::parallel_for(0ul, first_len, tr_fun);
         return EXIT_SUCCESS;
      }
   }  // namespace internal

   auto lz76DistanceMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) -> lz_int {
      if (flags.binary_)
         return internal::DistanceMatrixBinary(
            flags,
            lz_dist.info_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76InformationDistance));
      else if (flags.adn_ || flags.arn_)
         return internal::DistanceMatrixADN(
            flags,
            lz_dist.info_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76InformationDistance));
      else if (flags.trajectory_)
         return internal::DistanceMatrixTrajectory(
            flags,
            lz_dist.info_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76InformationDistance));
      else if (flags.revert_)
         return internal::DistanceMatrixRevert(
            flags,
            lz_dist.info_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76InformationDistance));
      else
         return internal::DistanceMatrixDefault(
            flags,
            lz_dist.info_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76InformationDistance));
   }

   auto lz76ShuffleDistanceMatrix(dist::LZ_Flags& flags, dist::LZ_Output& lz_dist) -> lz_int {
      if (flags.binary_)
         return internal::DistanceMatrixBinary(
            flags,
            lz_dist.shuffle_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76RandomShuffleDistance));
      else if (flags.adn_ || flags.arn_)
         return internal::DistanceMatrixADN(
            flags,
            lz_dist.shuffle_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76RandomShuffleDistance));
      else if (flags.trajectory_)
         return internal::DistanceMatrixTrajectory(
            flags,
            lz_dist.shuffle_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76RandomShuffleDistance));
      else if (flags.revert_)
         return internal::DistanceMatrixRevert(
            flags,
            lz_dist.shuffle_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76RandomShuffleDistance));
      else
         return internal::DistanceMatrixDefault(
            flags,
            lz_dist.shuffle_distance,
            overload_cast<const sequence&, const sequence&, utils::LZ_Args>(&lz::lz76RandomShuffleDistance));
   }
}  // namespace lz