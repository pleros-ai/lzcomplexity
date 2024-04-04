#pragma once

#include <lz/sequence.h>
#include <lz/structures.h>
#include <lz/types.h>

#include <vector>

namespace lz {
   namespace utils {

      //...............................................
      //          Free structures declaration
      //...............................................
      struct LZ_Output {
         std::vector<LempelZiv> data;

         std::vector<lz_bool> calculated_complexity;  //!> complexity calculated for the sequence in the index i
         // Post process parameters
         // std::vector<lz_uint>   complexity;               //!> complexity of the sequence
         std::vector<lz_uint>   half_complexity;          //!> complexity of the first half of the sequence
         std::vector<lz_double> lz_effective_complexity;  //!> excess of entropy by mutual information of the sequence
         // std::vector<lz_double> entropy_density;          //!> entropy density of the sequence
         std::vector<lz_double> excess_entropy_dist;  //!> excess of entropy by distance of the sequence
         // std::vector<shuffle_info> whole_random_shuffle_complexity;  //!> excess of entropy by shuffling of the
         // sequence std::vector<shuffle_info>
         //   random_shuffle_complexity;  //!> excess of entropy by shuffling of the merged sequence
         std::vector<lz_double> mutual_information;  //!> mutual information of two half of the sequences
         std::vector<lz_double> info_distance;  //!> information distance of the two consecutive sequences (using the
                                                //! merged sequence of both)
         std::vector<lz_double> random_shuffle_distance;  //!> information distance of the two consecutive sequences
                                                          //!(using MI estimated by random shuffle)
         std::vector<lz_double> sequence_info_distance;   //!> information distance of each sequences
         // std::vector<lz_double> multi_information;        //!> multi information value of each sequence

         std::vector<LZ_Extra> extra;

         LZ_Output() = default;
         LZ_Output(lz_size size) {
            data                  = std::vector<LempelZiv>{size};
            calculated_complexity = std::vector<bool>(size, false);
         };
         LZ_Output(const LZ_Output& lz) = default;
         LZ_Output(LZ_Output&& lz)      = default;

         auto setComplexity(lz_size, lz_uint) -> void;
         auto setEntropyDensity(lz_size, lz_double) -> void;
         auto setFactors(lz_size, std::vector<lz_uint>) -> void;
         auto setRandomShuffleComplexity(lz_size, LZ_Shuffle) -> void;
         auto setWholeRandomShuffleComplexity(lz_size, LZ_Shuffle) -> void;
         auto setEpsilon(lz_size, lz_double) -> void;
         auto setFactorsStddev(lz_size, lz_double) -> void;
         auto setNormalError(lz_size, lz_double) -> void;
         auto setPoisonError(lz_size, lz_double) -> void;
         auto setExtras(lz_size, LZ_Extra) -> void;

         auto checkCapacity(lz_size) -> void;

         LZ_Output& operator=(LZ_Output rhs) {
            std::swap(this->lz_effective_complexity, rhs.lz_effective_complexity);
            std::swap(this->excess_entropy_dist, rhs.excess_entropy_dist);
            std::swap(this->info_distance, rhs.info_distance);
            std::swap(this->sequence_info_distance, rhs.sequence_info_distance);
            std::swap(this->data, rhs.data);

            return *this;
         };

         LZ_Output& operator=(LZ_Output& rhs) {
            if (this != &rhs) {
               this->~LZ_Output();
               new (this) LZ_Output(rhs);
            }
            return *this;
         };
      };

      struct LZ_Flags {
         lz_int  shuffle_init_line = LZ_Args::UNDEFINED_LINES;  //!> Line to start the shuffle entropy deficit
         lz_int  shuffle_end_line  = LZ_Args::UNDEFINED_LINES;  //!> Line to end the shuffle entropy deficit
         lz_int  alphabet_size;                                 //!> Length of the alphabet of input sequences
         LZ_Args sa_args;  //!> Extra arguments for Suffix-array object and core functions

         std::vector<sequence> input;  //!> Input set of sequences

         LZ_Flags(std::string text, LZ_Args _sa_args)
           : input({text}), alphabet_size(2), sa_args(_sa_args){};
         LZ_Flags(sequence text, LZ_Args _sa_args)
           : input({text}), alphabet_size(2), sa_args(_sa_args){};
         LZ_Flags(std::vector<sequence> data, LZ_Args _sa_args)
           : input(data), alphabet_size(2), sa_args(_sa_args){};
         // Copy construct
         LZ_Flags(const LZ_Flags& flags)
           : input(flags.input), alphabet_size(flags.alphabet_size), sa_args(flags.sa_args){};
         // Move constructor
         LZ_Flags(LZ_Flags&& flags)
           : input(std::move(flags.input))
           , alphabet_size(std::exchange(flags.alphabet_size, 0))
           , sa_args(std::move(flags.sa_args)){};

         ~LZ_Flags(){};

         auto addData(std::string data) -> void { input.push_back(data); };
         auto addData(std::vector<std::string> data) -> void {
#ifdef __cpp_lib_ranges
            std::ranges::for_each(data, [&](auto&& elem) { input.push_back(elem); });
#else
            std::for_each(data.begin(), data.end(), [&](auto&& elem) { input.push_back(elem); });
#endif
         };

         LZ_Flags& operator=(LZ_Flags rhs) {
            std::swap(this->input, rhs.input);
            std::swap(this->sa_args, rhs.sa_args);
            std::swap(this->alphabet_size, rhs.alphabet_size);
            std::swap(this->shuffle_init_line, rhs.shuffle_init_line);
            std::swap(this->shuffle_end_line, rhs.shuffle_end_line);

            return *this;
         };

         LZ_Flags& operator=(LZ_Flags& rhs) {
            if (this != &rhs) {
               this->~LZ_Flags();
               new (this) LZ_Flags(rhs);
            }
            return *this;
         };

         friend bool operator==(const LZ_Flags& lhs, const LZ_Flags& rhs) {
            return lhs.input == rhs.input && lhs.sa_args == rhs.sa_args;
         };
         friend bool operator!=(const LZ_Flags& lhs, const LZ_Flags& rhs) { return !operator==(lhs, rhs); };
      };

   }  // namespace utils
}  // namespace lz