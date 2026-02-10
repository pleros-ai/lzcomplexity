#pragma once
/**
 * @file lzcomplexity.h
 * @brief Header for lzcomplexity standalone application
 */

#include "config_utils.hpp"

// Re-export common utilities for backward compatibility
using lz::standalone::time_point_t;
using lz::standalone::now;
using lz::standalone::duration_sec;
using lz::standalone::getColor;
using lz::standalone::print_msg;
using lz::standalone::read_input;
using lz::standalone::multiLineToOneLine;

// Backward compatibility alias for duration
constexpr inline auto duration = [](const std::chrono::nanoseconds& d) {
   return lz::standalone::duration_sec(d);
};
