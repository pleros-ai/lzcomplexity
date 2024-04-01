#pragma once

#include <lz/sa_structures.h>

#include "general.h"

namespace lz {

   namespace suffixarray {
      class CaPS_SA;
      class SAIS;
   }  // namespace suffixarray

   namespace utils {

      typedef struct {
         lz_size line;
         std::vector<lz_double> terms;
      } shuffle_terms;

      typedef struct {
         lz_int block_size;
         lz_double value;
         shuffle_terms terms;
      } shuffle_info;

      typedef struct {
         lz_double lz_rajski_distance;
         lz_double redundancy;
         lz_double fh_uncertainty;
         lz_double lh_uncertainty;
         lz_double lz_pearson_coefficient;
      } LZ_Extra;

      template <typename... Ts>
      struct overload : Ts... {
         using Ts::operator()...;
      };

#ifdef __cpp_lib_concepts
#define __requires(...) requires __VA_ARGS__
#else  // concepts defined
#define __requires(...)
#endif  // concepts defined

#if defined(__cpp_lib_concepts) && defined(__cpp_lib_variant)
      //---------------------- Concepts -----------------------//
      template <typename SAImpl>
      concept sa_empty_construct = requires(SAImpl sa) {
         { sa.construct() } -> std::same_as<LZ_SuffixArray>;
      };

      template <typename SAImpl>
      concept sa_string_construct = requires(SAImpl sa, const std::string& str) {
         { sa.construct(str) } -> std::same_as<LZ_SuffixArray>;
      };

      template <typename SAImpl>
      concept sa_char_pointer_construct = requires(SAImpl sa, const char* str, lz_int n) {
         { sa.construct(str, n) } -> std::same_as<LZ_SuffixArray>;
      };

      template <typename... SAImpl>
          requires((sa_empty_construct<SAImpl>) || ...) || (sa_string_construct<SAImpl> || ...) struct sa_type {
         using type = std::variant<suffixarray::CaPS_SA, suffixarray::SAIS, SAImpl...>;
      };

      template <typename... SAImpl>
      using sa_type_t = typename sa_type<SAImpl...>::type;
#endif

      /// An adapter for std::invoke_result that falls back to std::result_of if the former is not available.
      template <typename F, typename... Args>
#ifdef __cpp_lib_is_invocable
      using invoke_result_t = std::invoke_result_t<F, Args...>;
#else
      using invoke_result_t = std::result_of_t<F(Args...)>;
#endif
   }  // namespace utils
}  // namespace lz
