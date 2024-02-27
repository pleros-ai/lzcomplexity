#pragma once

#include "sequence.h"

#include <lz/caps.h>
#include <lz/sais_lite.h>

namespace lz {
   namespace utils {
      template <typename... Ts>
      struct overload : Ts... { using Ts::operator()...; };

      //---------------------- Concepts -----------------------//
      template <typename SAImpl>
      concept sa_empty_construct = requires (SAImpl sa) {
         {sa.construct()} -> std::same_as<LZ_SuffixArray>;
      };

      template <typename SAImpl>
      concept sa_string_construct = requires (SAImpl sa, const std::string& str) {
         {sa.construct(str)} -> std::same_as<LZ_SuffixArray>;
      };

      template <typename SAImpl>
      concept sa_char_pointer_construct = requires (SAImpl sa, const char* str, lz_int n) {
         {sa.construct(str, n)} -> std::same_as<LZ_SuffixArray>;
      };

      #if __cplusplus >= 201703L
      template<typename ...SAImpl>
      struct sa_type {
         using type = std::variant<suffixarray::CaPS_SA, suffixarray::SAIS, SAImpl...>;
      };
      #endif
   } // namespace utils
} // namespace lz
