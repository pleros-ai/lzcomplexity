#pragma once

#include <vector>
#include <lz/types.h>

namespace lz {
    namespace utils {

//...............................................
      //          Free structures declaration
      //...............................................
      struct LZ_Output {
         // Post process parameters
         std::vector<lz_int> complexity;              //!> complexity of the sequence
         std::vector<lz_int> excess_entropy_mi;       //!> excess of entropy by mutual information of the sequence
         std::vector<double> entropy_density;         //!> entropy density of the sequence
         std::vector<double> excess_entropy_dist;     //!> excess of entropy by distance of the sequence
         std::vector<double> excess_entropy_shuffle;  //!> excess of entropy by distance of the sequence
         std::vector<double> info_distance;           //!> information distance of the two consecutive sequences
         std::vector<double> sequence_info_distance;  //!> information distance of each sequences
         std::vector<double> multi_information;       //!> multi information value of each sequence

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
         SA_ALG algorithm;                   //!> Type of algorithm for construct the suffix array
         #if __cplusplus >= 201703L
         sa_type<>::type sa_impl;            //!> Implementation of the suffix array
         #endif
         std::vector<sequence> input;        //!> Input set of sequences

         // In case the selected algorithm is CaPS_SA
         lz_int alphabet_size;      //!> Length of the alphabet of input sequences
         LZ_Args sa_args;           //!> Suffix-array extra arguments for LZ core library functions

         // Post process parameters
         std::vector<lz_int> complexity;              //!> complexity of the sequence
         std::vector<lz_int> excess_entropy_mi;       //!> excess of entropy by mutual information of the sequence
         std::vector<double> entropy_density;         //!> entropy density of the sequence
         std::vector<double> excess_entropy_dist;     //!> excess of entropy by distance of the sequence
         std::vector<double> info_distance;           //!> information distance of the two consecutive sequences
         std::vector<double> sequence_info_distance;  //!> information distance of each sequences

         LZ_Flags(std::string text, LZ_Args _sa_args) :
            algorithm(SA_ALG::caps),
            sa_impl(lz::suffixarray::CaPS_SA(_sa_args.chunks, _sa_args.max_context)),
            input({ text }),
            alphabet_size(2),
            sa_args(_sa_args) {};
         LZ_Flags(sequence text, LZ_Args _sa_args) :
            algorithm(SA_ALG::caps),
            sa_impl(lz::suffixarray::CaPS_SA(_sa_args.chunks, _sa_args.max_context)),
            input({ text }),
            alphabet_size(2),
            sa_args(_sa_args) {};
         LZ_Flags(std::vector<sequence> data, LZ_Args _sa_args) :
            algorithm(SA_ALG::caps),
            sa_impl(lz::suffixarray::CaPS_SA(_sa_args.chunks, _sa_args.max_context)),
            input(data),
            alphabet_size(2),
            sa_args(_sa_args) {};
         /** @deprecated */
         #if __cplusplus >= 201703L
         LZ_Flags(sa_type<>::type& sa_impl_, std::string text, LZ_Args _sa_args) :
            sa_impl(sa_impl_),
            input({ text }),
            alphabet_size(2),
            sa_args(_sa_args) {
            if (std::holds_alternative<lz::suffixarray::CaPS_SA>(sa_impl_)) { algorithm = SA_ALG::caps; }
            else { algorithm = SA_ALG::sais; }
         };
         LZ_Flags(sa_type<>::type& sa_impl_, sequence text, LZ_Args _sa_args) :
            sa_impl(sa_impl_),
            input({ text }),
            alphabet_size(2),
            sa_args(_sa_args) {
            if (std::holds_alternative<lz::suffixarray::CaPS_SA>(sa_impl_)) { algorithm = SA_ALG::caps; }
            else { algorithm = SA_ALG::sais; }
         };
         LZ_Flags(sa_type<>::type& sa_impl_, std::vector<sequence> data, LZ_Args _sa_args) :
            sa_impl(sa_impl_),
            input(data),
            alphabet_size(2),
            sa_args(_sa_args) {
            if (std::holds_alternative<lz::suffixarray::CaPS_SA>(sa_impl_)) { algorithm = SA_ALG::caps; }
            else { algorithm = SA_ALG::sais; }
         };
         #endif
         /** @deprecated */
         LZ_Flags(SA_ALG algorithm, std::string text, LZ_Args _sa_args) :
            algorithm(algorithm),
            input({ text }),
            alphabet_size(2),
            sa_args(_sa_args) {};
         // Copy construct
         LZ_Flags(const LZ_Flags& flags) :
            algorithm(flags.algorithm),
            sa_impl(flags.sa_impl),
            input(flags.input),
            alphabet_size(flags.alphabet_size),
            sa_args(flags.sa_args),
            complexity(flags.complexity),
            excess_entropy_mi(flags.excess_entropy_mi),
            entropy_density(flags.entropy_density),
            excess_entropy_dist(flags.excess_entropy_dist),
            info_distance(flags.info_distance),
            sequence_info_distance(flags.sequence_info_distance) {};
         // Move constructor
         LZ_Flags(LZ_Flags&& flags) :
            algorithm(std::move(flags.algorithm)),
            sa_impl(std::move(flags.sa_impl)),
            input(std::move(flags.input)),
            alphabet_size(std::exchange(flags.alphabet_size, 0)),
            sa_args(std::move(flags.sa_args)),
            complexity(std::move(flags.complexity)),
            excess_entropy_mi(std::move(flags.excess_entropy_mi)),
            entropy_density(std::move(flags.entropy_density)),
            excess_entropy_dist(std::move(flags.excess_entropy_dist)),
            info_distance(std::move(flags.info_distance)),
            sequence_info_distance(std::move(flags.sequence_info_distance)) {};

         ~LZ_Flags() {};

         auto addData(std::string data) -> void { input.push_back(data); };
         auto addData(std::vector<std::string> data) -> void {
#ifdef __cpp_lib_ranges
            std::ranges::for_each(data, [&](auto&& elem) { input.push_back(elem); });
#else
            std::for_each(data.begin(), data.end(), [&](auto&& elem) { input.push_back(elem); });
#endif
         };

         LZ_Flags& operator=(LZ_Flags rhs) {
            std::swap(this->algorithm, rhs.algorithm);
            std::swap(this->input, rhs.input);
            std::swap(this->sa_args, rhs.sa_args);

            return *this;
         };

         LZ_Flags& operator=(LZ_Flags& rhs) {
            if (this != &rhs) {
               this->~LZ_Flags();
               new (this) LZ_Flags(rhs);
            }
            return *this;
         };

         friend constexpr bool operator==(const LZ_Flags& lhs, const LZ_Flags& rhs) {
            return lhs.algorithm == rhs.algorithm &&
               lhs.input == rhs.input &&
               lhs.sa_args == rhs.sa_args;
         };
         friend constexpr bool operator!=(const LZ_Flags& lhs, const LZ_Flags& rhs) { return !operator==(lhs, rhs); };
      };

    }
}