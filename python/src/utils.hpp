#include <lz/lempelziv.h>
#include <lz/utils.h>
#include <pybind11/stl.h>

#include <type_traits>
#include <variant>

namespace py  = pybind11;
namespace utl = lz::utils;

// helper type for the visitor #4
template<class... Ts>
struct overload : Ts... {
   using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

using seq_type  = std::variant<lz::sequence, std::string, std::vector<char>, std::vector<int>>;
using seq_type2 = std::variant<std::string, std::vector<char>, std::vector<int>>;

template<typename T, typename VARIANT_T>
struct isVariantMember;

template<typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>> : public std::disjunction<std::is_same<T, ALL_T>...> {};

template<typename T, typename Fun>
auto generateFunctionWithoutArgs(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq) {
      T cpx;

      std::visit(overload{[&](auto&& s) { cpx = fun(s); },
                          [&](std::vector<int> s) {
                             auto string_view = s | std::views::transform([](int num) { return std::to_string(num); });
                             auto str         = std::accumulate(
                                std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                             cpx = fun(str);
                          }},
                 seq);
      return cpx;
   };
}
template<typename T, typename Fun>
auto generateFunctionSequenceWithoutArgs(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq) { return fun(seq); };
}

template<typename T, typename Seq, typename... Args>
struct return_function {
   template<typename Fun>
   auto operator()(Fun&& fun) const {
      return [fun = std::forward<Fun>(fun)](const Seq& seq, Args... params) {
         T cpx;

         utl::LZ_Args args(params...);

         std::cout << "variant: " << std::is_same_v<Seq, seq_type> << std::endl;
         std::cout << "seq: " << std::is_same_v<Seq, lz::sequence> << std::endl;
         if (std::is_same_v<Seq, lz::sequence> == true) {
            return fun(seq, args);
         } else {
            std::visit(overload{[&](auto&& s) { cpx = fun(s, args); },
                                [&](std::vector<int> s) {
                                   auto string_view =
                                      s | std::views::transform([](int num) { return std::to_string(num); });
                                   auto str = std::accumulate(
                                      std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                                   cpx = fun(str, args);
                                }},
                       seq);
            return cpx;
         }
      };
   };
};

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

template<typename T, typename Fun>
auto generateFunctionWithArgs(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq, utl::LZ_Args& args) {
      T cpx;

      std::visit(overload{[&](auto&& s) { cpx = fun(s, args); },
                          [&](std::vector<int> s) {
                             auto string_view = s | std::views::transform([](int num) { return std::to_string(num); });
                             auto str         = std::accumulate(
                                std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                             cpx = fun(str, args);
                          }},
                 seq);
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionSequenceWithArgs(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq, utl::LZ_Args& args) { return fun(seq, args); };
}

template<typename T, typename Fun>
auto generateFunctionWithArgsAndFlags(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq,
                                         lz::lz_int      partitions = 1,
                                         lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                                         lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                                         lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
      utl::LZ_Args args;
      args.chunks   = partitions;
      args.alphabet = alphabet;
      args.log_base = log_base;
      T cpx;

      utl::EnabledMT(jobs);
      std::visit(overload{[&](auto&& s) { cpx = fun(s, args); },
                          [&](std::vector<int> s) {
                             auto string_view = s | std::views::transform([](int num) { return std::to_string(num); });
                             auto str         = std::accumulate(
                                std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                             cpx = fun(str, args);
                          }},
                 seq);
      utl::DisabledMT();
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionSequenceWithArgsAndFlags(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq,
                                         lz::lz_int          partitions = 1,
                                         lz::lz_uint         alphabet   = lz::ALPHABET_SIZE,
                                         lz::lz_uint         log_base   = lz::ALPHABET_SIZE,
                                         lz::lz_uint         jobs       = std::thread::hardware_concurrency()) {
      utl::LZ_Args args;
      args.chunks   = partitions;
      args.alphabet = alphabet;
      args.log_base = log_base;
      utl::EnabledMT(jobs);
      auto res = fun(seq, args);
      utl::DisabledMT();
      return res;
   };
}

template<typename T, typename Fun>
auto generateFunctionWithArgsAndFlagsForShuffle(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq,
                                         lz::lz_int      partitions     = 1,
                                         lz::lz_uint     alphabet       = lz::ALPHABET_SIZE,
                                         lz::lz_uint     log_base       = lz::ALPHABET_SIZE,
                                         lz::lz_int      max_block_size = -1,
                                         lz::lz_uint     jobs           = std::thread::hardware_concurrency()) {
      utl::LZ_Args args;
      args.chunks     = partitions;
      args.alphabet   = alphabet;
      args.log_base   = log_base;
      args.block_size = max_block_size;
      T cpx;

      utl::EnabledMT(jobs);
      std::visit(overload{[&](auto&& s) { cpx = fun(s, args); },
                          [&](std::vector<int> s) {
                             auto string_view = s | std::views::transform([](int num) { return std::to_string(num); });
                             auto str         = std::accumulate(
                                std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                             cpx = fun(str, args);
                          }},
                 seq);
      utl::DisabledMT();
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionSequenceWithArgsAndFlagsForShuffle(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq,
                                         lz::lz_int          partitions     = 1,
                                         lz::lz_uint         alphabet       = lz::ALPHABET_SIZE,
                                         lz::lz_uint         log_base       = lz::ALPHABET_SIZE,
                                         lz::lz_int          max_block_size = -1,
                                         lz::lz_uint         jobs           = std::thread::hardware_concurrency()) {
      utl::LZ_Args args;
      args.chunks     = partitions;
      args.alphabet   = alphabet;
      args.log_base   = log_base;
      args.block_size = max_block_size;
      utl::EnabledMT(jobs);
      auto res = fun(seq, args);
      utl::DisabledMT();
      return res;
   };
}

template<typename T, typename Fun>
auto generateFunctionSequenceWithoutArgsForDistance(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq1, const lz::sequence& seq2) {
      T cpx;

      cpx = fun(seq1, seq2);
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionWithoutArgsForDistance(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq1, const seq_type& seq2) {
      T cpx;

      std::visit(
         overload{[&](auto&& s1, auto&& s2) { cpx = fun(s1, s2); },
                  [&](auto&& s1, std::vector<int> s2) {
                     auto string_view = s2 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str =
                        std::accumulate(std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});
                     cpx = fun(s1, str);
                  },
                  [&](std::vector<int> s1, auto&& s2) {
                     auto string_view = s1 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str =
                        std::accumulate(std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});
                     cpx = fun(str, s2);
                  },
                  [&](std::vector<int> s1, std::vector<int> s2) {
                     auto string_view1 = s1 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str1         = std::accumulate(
                        std::ranges::begin(string_view1), std::ranges::end(string_view1), std::string{});

                     auto string_view2 = s2 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str2         = std::accumulate(
                        std::ranges::begin(string_view2), std::ranges::end(string_view2), std::string{});

                     cpx = fun(str1, str2);
                  }},
         seq1,
         seq2);
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionSequenceWithArgsForDistance(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq1, const lz::sequence& seq2, utl::LZ_Args& args) {
      T cpx;

      cpx = fun(seq1, seq2, args);
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionWithArgsForDistance(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq1, const seq_type& seq2, utl::LZ_Args& args) {
      T cpx;

      std::visit(
         overload{[&](auto&& s1, auto&& s2) { cpx = fun(s1, s2, args); },
                  [&](auto&& s1, std::vector<int> s2) {
                     auto string_view = s2 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str =
                        std::accumulate(std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});
                     cpx = fun(s1, str, args);
                  },
                  [&](std::vector<int> s1, auto&& s2) {
                     auto string_view = s1 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str =
                        std::accumulate(std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});
                     cpx = fun(str, s2, args);
                  },
                  [&](std::vector<int> s1, std::vector<int> s2) {
                     auto string_view1 = s1 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str1         = std::accumulate(
                        std::ranges::begin(string_view1), std::ranges::end(string_view1), std::string{});

                     auto string_view2 = s2 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str2         = std::accumulate(
                        std::ranges::begin(string_view2), std::ranges::end(string_view2), std::string{});

                     cpx = fun(str1, str2, args);
                  }},
         seq1,
         seq2);
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionSequenceWithArgsAndFlagsForDistance(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const lz::sequence& seq1,
                                         const lz::sequence& seq2,
                                         lz::lz_int          partitions = 1,
                                         lz::lz_uint         alphabet   = lz::ALPHABET_SIZE,
                                         lz::lz_uint         log_base   = lz::ALPHABET_SIZE,
                                         lz::lz_uint         jobs       = std::thread::hardware_concurrency()) {
      utl::LZ_Args args;
      args.chunks   = partitions;
      args.alphabet = alphabet;
      args.log_base = log_base;
      T cpx;

      utl::EnabledMT(jobs);
      cpx = fun(seq1, seq2, args);
      utl::DisabledMT();
      return cpx;
   };
}

template<typename T, typename Fun>
auto generateFunctionWithArgsAndFlagsForDistance(Fun&& fun) {
   return [fun = std::forward<Fun>(fun)](const seq_type& seq1,
                                         const seq_type& seq2,
                                         lz::lz_int      partitions = 1,
                                         lz::lz_uint     alphabet   = lz::ALPHABET_SIZE,
                                         lz::lz_uint     log_base   = lz::ALPHABET_SIZE,
                                         lz::lz_uint     jobs       = std::thread::hardware_concurrency()) {
      utl::LZ_Args args;
      args.chunks   = partitions;
      args.alphabet = alphabet;
      args.log_base = log_base;
      T cpx;

      utl::EnabledMT(jobs);
      std::visit(
         overload{[&](auto&& s1, auto&& s2) { cpx = fun(s1, s2, args); },
                  [&](auto&& s1, std::vector<int> s2) {
                     auto string_view = s2 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str =
                        std::accumulate(std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});
                     cpx = fun(s1, str, args);
                  },
                  [&](std::vector<int> s1, auto&& s2) {
                     auto string_view = s1 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str =
                        std::accumulate(std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});
                     cpx = fun(str, s2, args);
                  },
                  [&](std::vector<int> s1, std::vector<int> s2) {
                     auto string_view1 = s1 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str1         = std::accumulate(
                        std::ranges::begin(string_view1), std::ranges::end(string_view1), std::string{});

                     auto string_view2 = s2 | std::views::transform([](int num) { return std::to_string(num); });
                     auto str2         = std::accumulate(
                        std::ranges::begin(string_view2), std::ranges::end(string_view2), std::string{});

                     cpx = fun(str1, str2, args);
                  }},
         seq1,
         seq2);
      utl::DisabledMT();
      return cpx;
   };
}