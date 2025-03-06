#include "utils.hpp"

using constructor_param = std::variant<std::string, std::vector<lz::lz_char>, std::vector<lz::lz_int>>;

auto generateSequenceConstructor() {
   return [](const constructor_param& seq) {
      return std::visit(overload{[](auto&& s) { return lz::sequence(s); },
                                 [](std::vector<lz::lz_int> s) {
                                    auto string_view =
                                       s | std::views::transform([](lz::lz_int num) { return std::to_string(num); });
                                    auto res = std::accumulate(
                                       std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                                    return lz::sequence(res);
                                 }},
                        seq);
   };
}

auto generateSequenceConstructorWithAlphabet() {
   return [](const constructor_param& seq, lz::lz_uint alphabet) {
      return std::visit(overload{[&](auto&& s) { return lz::sequence(s, alphabet); },
                                 [&](std::vector<lz::lz_int> s) {
                                    auto string_view =
                                       s | std::views::transform([](lz::lz_int num) { return std::to_string(num); });
                                    auto res = std::accumulate(
                                       std::ranges::begin(string_view), std::ranges::end(string_view), std::string{});

                                    return lz::sequence(res, alphabet);
                                 }},
                        seq);
   };
}