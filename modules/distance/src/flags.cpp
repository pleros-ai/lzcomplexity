/**
 * @file flags.cpp
 * @brief Implementation of LZ_Output distance matrix setters and allocation functions.
 *
 * Provides efficient storage operations for information distance, shuffle distance,
 * and directed graph matrices used in LZ-based sequence comparison.
 */

#include <lzDistance/flags.hpp>

#include "lz/general.h"

namespace lz {
  namespace dist {

    // ═══════════════════════════════════════════════════════════════════════════════
    // Info Distance Setters
    // ═══════════════════════════════════════════════════════════════════════════════

    auto LZ_Output::setDistance(lz_size idx, lz_double dist) -> void { info_distance[idx].push_back(dist); }

    auto LZ_Output::setDistance(lz_size idx, std::vector<lz_double> distances) -> void {
      info_distance[idx] = std::move(distances);
    }

    auto LZ_Output::setDistance(lz_size i, lz_size j, lz_double dist) -> void {
      if (i >= info_distance.size() || j >= info_distance[i].size()) [[unlikely]] {
        throw lz::LZOutOfBounds();
      }
      info_distance[i][j] = dist;
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // Shuffle Distance Setters
    // ═══════════════════════════════════════════════════════════════════════════════

    auto LZ_Output::setDistanceShuffle(lz_size idx, lz_double dist) -> void {
      shuffle_distance[idx].push_back(dist);
    }

    auto LZ_Output::setDistanceShuffle(lz_size idx, std::vector<lz_double> distances) -> void {
      shuffle_distance[idx] = std::move(distances);
    }

    auto LZ_Output::setDistanceShuffle(lz_size i, lz_size j, lz_double dist) -> void {
      if (i >= shuffle_distance.size() || j >= shuffle_distance[i].size()) [[unlikely]] {
        throw lz::LZOutOfBounds();
      }
      shuffle_distance[i][j] = dist;
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // Directed Matrix Setters
    // ═══════════════════════════════════════════════════════════════════════════════

    auto LZ_Output::setDirectionValue(lz_size idx, lz_int val) -> void {
      directed_matrix[idx].push_back(val);
    }

    auto LZ_Output::setDirectionValue(lz_size idx, std::vector<lz_int> vals) -> void {
      directed_matrix[idx] = std::move(vals);
    }

    auto LZ_Output::setDirectionValue(lz_size i, lz_size j, lz_int val) -> void {
      if (i >= directed_matrix.size() || j >= directed_matrix[i].size()) [[unlikely]] {
        throw lz::LZOutOfBounds();
      }
      directed_matrix[i][j] = val;
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // Matrix Allocation Functions
    // ═══════════════════════════════════════════════════════════════════════════════

    auto LZ_Output::reserveInfoDistance(lz_size size) -> void {
      info_distance.assign(size, std::vector<lz_double>(size));
    }

    auto LZ_Output::reserveInfoDistance(lz_size rows, lz_size cols) -> void {
      info_distance.assign(rows, std::vector<lz_double>(cols));
    }

    auto LZ_Output::reserveShuffleDistance(lz_size size) -> void {
      shuffle_distance.assign(size, std::vector<lz_double>(size));
    }

    auto LZ_Output::reserveShuffleDistance(lz_size rows, lz_size cols) -> void {
      shuffle_distance.assign(rows, std::vector<lz_double>(cols));
    }

    auto LZ_Output::reserveDirectionGraph(lz_size size) -> void {
      directed_matrix.assign(size, std::vector<lz_int>(size));
    }

    auto LZ_Output::reserveDirectionGraph(lz_size rows, lz_size cols) -> void {
      directed_matrix.assign(rows, std::vector<lz_int>(cols));
    }

  }  // namespace dist
}  // namespace lz
