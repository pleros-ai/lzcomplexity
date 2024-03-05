#pragma once

#include <lz/structures.h>
#include <lz/types.h>

#include <vector>

namespace lz {
   namespace utils {

      //...............................................
      //          Free structures declaration
      //...............................................
      struct LZ_Output {
         // Post process parameters
         std::vector<lz_int> complexity;                 //!> complexity of the sequence
         std::vector<lz_int> excess_entropy_mi;          //!> excess of entropy by mutual information of the sequence
         std::vector<lz_double> entropy_density;         //!> entropy density of the sequence
         std::vector<lz_double> excess_entropy_dist;     //!> excess of entropy by distance of the sequence
         std::vector<lz_double> excess_entropy_shuffle;  //!> excess of entropy by shuffling of the sequence
         std::vector<lz_double> info_distance;           //!> information distance of the two consecutive sequences
         std::vector<lz_double> sequence_info_distance;  //!> information distance of each sequences
         std::vector<lz_double> multi_information;       //!> multi information value of each sequence

         //? Variables found in Excess fo entropy by shuffling
         std::vector<lz_double> excess_entropy_terms;  //!> excess of entropy of each permutation of the sequence
         lz_double multi_info;                         //!> multi information value

         LZ_Output() = default;
         LZ_Output(const LZ_Output& lz) = default;
         LZ_Output(LZ_Output&& lz) = default;

         LZ_Output& operator=(LZ_Output rhs) {
            std::swap(this->complexity, rhs.complexity);
            std::swap(this->excess_entropy_mi, rhs.excess_entropy_mi);
            std::swap(this->entropy_density, rhs.entropy_density);
            std::swap(this->excess_entropy_dist, rhs.excess_entropy_dist);
            std::swap(this->excess_entropy_shuffle, rhs.excess_entropy_shuffle);
            std::swap(this->info_distance, rhs.info_distance);
            std::swap(this->sequence_info_distance, rhs.sequence_info_distance);
            std::swap(this->multi_information, rhs.multi_information);

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
         std::vector<sequence> input;  //!> Input set of sequences

         // In case the selected algorithm is CaPS_SA
         lz_int alphabet_size;  //!> Length of the alphabet of input sequences
         LZ_Args sa_args;       //!> Suffix-array extra arguments for LZ core library functions

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
             : input(std::move(flags.input)),
               alphabet_size(std::exchange(flags.alphabet_size, 0)),
               sa_args(std::move(flags.sa_args)){};

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