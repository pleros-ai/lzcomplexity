#pragma once

#include <lz/caps.h>
#include <lz/sais_lite.h>

#include "sequence.h"

namespace lz {
   namespace utils {
      template <typename... Ts>
      struct overload : Ts... {
         using Ts::operator()...;
      };

#ifdef __cpp_concepts
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
   }  // namespace utils
}  // namespace lz
