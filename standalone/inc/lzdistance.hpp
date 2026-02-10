#pragma once
/**
 * @file lzdistance.hpp
 * @brief Header for lzdistance standalone application
 */

#include "config_utils.hpp"
#include <lz/parallel_utils.h>

// Re-export common utilities for backward compatibility
using lz::standalone::time_point_t;
using lz::standalone::now;
using lz::standalone::duration_sec;
using lz::standalone::getColor;
using lz::standalone::print_msg;
using lz::standalone::read_input;
using lz::standalone::FileInfo;
using lz::standalone::DirectoryReader;

// Backward compatibility alias for duration
constexpr inline auto duration = [](const std::chrono::nanoseconds& d) {
   return lz::standalone::duration_sec(d);
};

/**
 * @brief Read all files from a directory in parallel
 */
inline std::vector<lz::sequence> read_dir(const std::filesystem::path& path, 
                                          MagickNumber format = MagickNumber::PNM_RAWTXT) {
   if (!std::filesystem::is_directory(path)) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Provided path is not a directory.") << std::endl;
      throw FileFormatError("Provided path is not a directory.");
   }

   DirectoryReader reader(path);
   const auto& files = reader.files();
   std::vector<lz::sequence> data(files.size());

   lz::utils::parallel_for(0ul, files.size(), [&](lz::lz_size idx) {
      auto seq = read_input(files[idx].path, false, format);
      if (!seq.empty()) {
         data[idx] = std::move(seq[0]);
      }
   });

   return data;
}
