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

   constexpr lz_uint ALPHABET_SIZE = 0x02;

   template <typename _Ret, typename... _Args>
   class lz_fun : public std::function<_Ret(_Args...)> {};

   const lz_str RED_COLOR = "\033[1;31m";
   const lz_str GREEN_COLOR = "\033[1;32m";
   const lz_str YELLOW_COLOR = "\033[1;33m";
   const lz_str BLUE_COLOR = "\033[1;34m";
   const lz_str MAGENTA_COLOR = "\033[1;35m";
   const lz_str END_COLOR = "\033[0m";

}  // namespace lz