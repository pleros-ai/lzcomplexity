#include <lz/lempelziv.h>

#include <filesystem>

#include "flags.hpp"

namespace fs = std::filesystem;

namespace lz {

   auto lz76DistanceMatrix(dist::LZ_Flags&, dist::LZ_Output&) -> lz_int;

   auto lz76ShuffleDistanceMatrix(dist::LZ_Flags&, dist::LZ_Output&) -> lz_int;

}  // namespace lz