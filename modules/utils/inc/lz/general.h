#pragma once

// Language includes
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#ifdef __cpp_lib_ranges
#include <ranges>
#endif

#ifdef __cpp_lib_concepts
#include <concepts>
#endif

#if __cpp_lib_variant
#include <variant>
#endif

namespace lz {

  using lz_char = char;
  using lz_int = int;
  using lz_uint = unsigned int;
  using lz_size = std::size_t;
  using lz_double = double;
  using lz_bool = bool;
  using lz_str = std::string;

  template<typename _Ret, typename... _Args>
  class lz_fun : public std::function<_Ret(_Args...)> {};

  namespace details {
    constexpr lz_uint ALPHABET_SIZE = 0x02;
    constexpr lz_uint NO_ALPHABET = std::numeric_limits<lz_uint>::max();

    static constexpr lz_int ALL_LINES = -2;  ///< Sentinel value indicating all lines should be processed.
    static constexpr lz_int UNDEFINED_LINES = -1;  ///< Sentinel value for undefined line count.
    struct LineRange {
      lz::lz_int init = UNDEFINED_LINES;
      lz::lz_int end = UNDEFINED_LINES;
    };
  }  // namespace details

}  // namespace lz