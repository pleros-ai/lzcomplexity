#pragma once

#include <lz/sequence.h>
#include <lz/structures.h>
#include <lz/types.h>

#include <vector>

namespace lz {
   namespace dist {

      //...............................................
      //          Free structures declaration
      //...............................................
      struct LZ_Output {
         std::vector<lz_bool> first_calculated_complexity;   //!> complexity calculated for the sequence in the index i
                                                             //! from first data source
         std::vector<lz_bool> second_calculated_complexity;  //!> complexity calculated for the sequence in the index i
                                                             //! from second data source

         std::vector<std::vector<double>> info_distance;
         std::vector<std::vector<double>> shuffle_distance;

         LZ_Output() = default;
         LZ_Output(lz_size size) {
            // first_calculated_complexity  = std::vector<bool>(size, false);
            // second_calculated_complexity = std::vector<bool>(size, false);

            info_distance = std::vector<std::vector<double>>(size);
            for (auto i = 0ul; i < size; i++) {
               info_distance[i] = std::vector<double>(size);
            }

            shuffle_distance = std::vector<std::vector<double>>(size);
            for (auto i = 0ul; i < size; i++) {
               shuffle_distance[i] = std::vector<double>(size);
            }
         };
         LZ_Output(lz_size first_size, lz_size second_size) {
            // first_calculated_complexity  = std::vector<bool>(size, false);
            // second_calculated_complexity = std::vector<bool>(size, false);

            info_distance = std::vector<std::vector<double>>(first_size);
            for (auto i = 0ul; i < first_size; i++) {
               info_distance[i] = std::vector<double>(second_size);
            }

            shuffle_distance = std::vector<std::vector<double>>(first_size);
            for (auto i = 0ul; i < first_size; i++) {
               shuffle_distance[i] = std::vector<double>(second_size);
            }
         };
         LZ_Output(const LZ_Output& lz) = default;
         LZ_Output(LZ_Output&& lz)      = default;

         auto setComplexity(lz_size, lz_uint) -> void;
         //// Put an information distance value for a data with the index i
         auto setDistance(lz_size, lz_double) -> void;
         //// Put a set of information distances for a data with the index i
         auto setDistance(lz_size, std::vector<lz_double>) -> void;
         //// Put a set of information distances between two data with the index i and j
         auto setDistance(lz_size, lz_size, lz_double) -> void;
         //// Put a shuffle information distance value for a data with the index i
         auto setDistanceShuffle(lz_size, lz_double) -> void;
         //// Put a set of shuffle information distances for a data with the index i
         auto setDistanceShuffle(lz_size, std::vector<lz_double>) -> void;
         //// Put a set of shuffle information distances between two data with the index i and j
         auto setDistanceShuffle(lz_size, lz_size, lz_double) -> void;

         auto checkCapacity(lz_size) -> void;

         LZ_Output& operator=(LZ_Output rhs) {
            // std::swap(this->data, rhs.data);

            std::swap(this->first_calculated_complexity, rhs.first_calculated_complexity);
            std::swap(this->second_calculated_complexity, rhs.second_calculated_complexity);
            std::swap(this->info_distance, rhs.info_distance);
            std::swap(this->shuffle_distance, rhs.shuffle_distance);

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

      enum INPUT_DATA { FIRST, SECOND };

      struct LZ_Flags {
         lz_int first_dist_init = utils::LZ_Args::UNDEFINED_LINES;  //!> Line to start the distance calculation
         lz_int first_dist_end  = utils::LZ_Args::UNDEFINED_LINES;  //!> Line to end the distance calculation

         lz_int second_dist_init = utils::LZ_Args::UNDEFINED_LINES;  //!> Line to start the distance calculation
         lz_int second_dist_end  = utils::LZ_Args::UNDEFINED_LINES;  //!> Line to end the distance calculation

         utils::LZ_Args sa_args;  //!> Extra arguments for Suffix-array object and core functions

         std::vector<sequence> first_input;   //!> First input set of sequences
         std::vector<sequence> second_input;  //!> Second input set of sequences

         lz::lz_bool revert_     = false;  //!> Revert the seconds sequences
         lz::lz_bool text_       = false;  //!> Revert the seconds sequences
         lz::lz_bool binary_     = false;  //!> Apply distance for binary sequences --> 4 options
         lz::lz_bool adn_        = false;  //!> Apply distance between ADN sequences --> 6 options
         lz::lz_bool arn_        = false;  //!> Apply distance between ARN sequences --> 6 options
         lz::lz_bool trajectory_ = false;  //!> Apply distance between trajectories sequences --> 17 options

         LZ_Flags(sequence text, utils::LZ_Args _sa_args)
           : first_input({text}), second_input({text}), sa_args(_sa_args){};
         LZ_Flags(sequence text1, sequence text2, utils::LZ_Args _sa_args)
           : first_input({text1}), second_input({text2}), sa_args(_sa_args){};
         LZ_Flags(std::vector<sequence> data, utils::LZ_Args _sa_args)
           : first_input(data), second_input(data), sa_args(_sa_args){};
         LZ_Flags(std::vector<sequence> data1, std::vector<sequence> data2, utils::LZ_Args _sa_args)
           : first_input(data1), second_input(data2), sa_args(_sa_args){};
         // Copy construct
         LZ_Flags(const LZ_Flags& flags)
           : first_input(flags.first_input), second_input(flags.second_input), sa_args(flags.sa_args){};
         // Move constructor
         LZ_Flags(LZ_Flags&& flags)
           : first_input(std::move(flags.first_input))
           , second_input(std::move(flags.second_input))
           , sa_args(std::move(flags.sa_args)){};

         ~LZ_Flags(){};

         auto addData(INPUT_DATA in, std::string data) -> void {
            if (in == FIRST)
               first_input.push_back(data);
            else
               second_input.push_back(data);
         };
         auto addData(INPUT_DATA in, std::vector<std::string> data) -> void {
#ifdef __cpp_lib_ranges
            std::ranges::for_each(data, [&](auto&& elem) {
               if (in == FIRST)
                  first_input.push_back(elem);
               else
                  second_input.push_back(elem);
            });
#else
            std::for_each(data.begin(), data.end(), [&](auto&& elem) {
               if (in == FIRST)
                  first_input.push_back(elem);
               else
                  second_input.push_back(elem);
            });
#endif
         };

         LZ_Flags& operator=(LZ_Flags rhs) {
            std::swap(this->first_input, rhs.first_input);
            std::swap(this->second_input, rhs.second_input);
            std::swap(this->sa_args, rhs.sa_args);

            std::swap(this->revert_, rhs.revert_);
            std::swap(this->adn_, rhs.adn_);
            std::swap(this->arn_, rhs.arn_);
            std::swap(this->text_, rhs.text_);
            std::swap(this->binary_, rhs.binary_);
            std::swap(this->trajectory_, rhs.trajectory_);

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
            return lhs.first_input == rhs.first_input && lhs.second_input == rhs.second_input &&
                   lhs.sa_args == rhs.sa_args;
         };
         friend bool operator!=(const LZ_Flags& lhs, const LZ_Flags& rhs) { return !operator==(lhs, rhs); };
      };

   }  // namespace dist
}  // namespace lz