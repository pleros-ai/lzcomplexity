#include <lz/lempelziv.h>
#include <lz/sequence.h>
#include <lz/utils.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <type_traits>
#include <variant>

namespace py  = nanobind;
namespace utl = lz::utils;

// helper type for the visitor #4
template<class... Ts>
struct overload : Ts... {
   using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

using seq_type = std::variant<lz::sequence, std::string, std::vector<char>, std::vector<int>>;

template<typename T, typename VARIANT_T>
struct isVariantMember;

template<typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>> : public std::disjunction<std::is_same<T, ALL_T>...> {};

// template<typename T, typename Fun>
// auto generateFunctionSequenceWithArgs(Fun&& fun) {
//    return return_function<T, lz::sequence, utl::LZ_Args>{}(fun);
// }

// template<typename T, typename Seq, typename Fun>
// auto generateFunctionWithArgs(Fun&& fun) {
//    return return_function<T, Seq, utl::LZ_Args>{}(fun);
// }

// template<typename T, typename Fun>
// auto test2(Fun&& fun) {
//    return return_function<T, lz::sequence, lz::lz_int, lz::lz_int, lz::lz_uint, lz::lz_uint>{}(fun);
// }
