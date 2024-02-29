#pragma once

#include <vector>
#include <lz/utils.h>

namespace lz {
    namespace utils {
        enum SA_ALG { sais, caps };

        struct LZ_ExcessInfo {
            lz_int max_block_size;                  //? The value used in excess of entropy by shuffling
            lz_double excess_value;                 //? The excess of entropy value
            lz_double multi_information;            //? The multi information value
            std::vector<lz_double> excess_by_terms; //? The vector of excess fo entropy for each term (size == mm_value)
        };

        struct LZ_Args {
            /* Args for CaPS class */
            lz_int chunks = 0;          //?> Number of chunks to divide the sequence (use by CaPS)
            lz_int max_context = 0;     //?> Max length to compare prefix for generate the LCP (use by CaPS)
            /* Excess entropy by shuffling parameters */
            lz_int block_size = 0;      //?> Max length of the block for excess of entropy by shuffle.
            lz_int excess_line = -1;    //?> Line where get excess entropy by terms (valid for excess of entropy by shuffling).

            LZ_Args() = default;
            LZ_Args(lz_int chunks) : chunks(chunks) {};
            LZ_Args(lz_int chunks, lz_int max_context) : chunks(chunks), max_context(max_context) {};
            LZ_Args(lz_int chunks, lz_int max_context, lz_int block_size) : chunks(chunks), max_context(max_context), block_size(block_size) {};
            LZ_Args(const LZ_Args& sa) = default;
            LZ_Args(LZ_Args&& sa) = default;

            LZ_Args& operator=(LZ_Args rhs) {
                std::swap(this->chunks, rhs.chunks);
                std::swap(this->max_context, rhs.max_context);
                std::swap(this->block_size, rhs.block_size);
                std::swap(this->excess_line, rhs.excess_line);
                return *this;
            };

            friend bool operator==(LZ_Args sa1, LZ_Args sa2) {
                return sa1.chunks == sa2.chunks && sa1.max_context == sa2.max_context && 
                       sa1.block_size == sa2.block_size && sa1.excess_line == sa2.excess_line;
            };
            friend bool operator!=(LZ_Args sa1, LZ_Args sa2) { return !(sa1 == sa2); };

            friend void swap(LZ_Args& sa1, LZ_Args& sa2) {
                std::swap(sa1.chunks, sa2.chunks);
                std::swap(sa1.max_context, sa2.max_context);
                std::swap(sa1.block_size, sa2.block_size);
                std::swap(sa1.excess_line, sa2.excess_line);
            };
        };

        struct LZ_SuffixArray {
            std::vector<lz_uint> SA;      //!> Suffix array values
            std::vector<lz_uint> LCP;     //!> Longest Common Prefix values
            lz_uint n;                          //!> Length of the string (n+1 size of the vectors)

            LZ_SuffixArray(void) :SA(), LCP(), n(0) {};
            LZ_SuffixArray(std::vector<lz_uint> SA_, std::vector<lz_uint> LCP_, const lz_uint n_) : SA(std::move(SA_)), LCP(std::move(LCP_)), n(n_){};
            LZ_SuffixArray(std::vector<lz_uint> SA_, const lz_uint n_) : SA(std::move(SA_)), LCP(), n(n_){};
            LZ_SuffixArray(lz_uint* const SA_, lz_uint* const LCP_, lz_uint n_) :n(n_) {
                SA = std::vector<lz_uint>(SA_, SA_ + n_);
                LCP = std::vector<lz_uint>(LCP_, LCP_ + n_);
            }
            LZ_SuffixArray(lz_int* const SA_, lz_int* const LCP_, lz_int n_) :n(n_) {
                SA = std::vector<lz_uint>(SA_, SA_ + n_);
                LCP = std::vector<lz_uint>(LCP_, LCP_ + n_);
            }
            LZ_SuffixArray(const LZ_SuffixArray& oth) :SA(oth.SA), LCP(oth.LCP), n(oth.n) {}
            LZ_SuffixArray(LZ_SuffixArray&& oth) noexcept :SA(std::move(oth.SA)), LCP(std::move(oth.LCP)), n(std::move(oth.n)) {}

            void Clear() { SA.clear(); LCP.clear(); n = 0; };
            ~LZ_SuffixArray() {};

            LZ_SuffixArray& operator=(LZ_SuffixArray rhs) {
                std::swap((*this).SA, rhs.SA);
                std::swap((*this).LCP, rhs.LCP);
                std::swap((*this).n, rhs.n);

                return *this;
            };

            bool operator==(const LZ_SuffixArray& arr) const {
                bool same = n == arr.n;

                if (!same) return false;

                lz_uint i = 0;
                while ((same = SA[i] == arr.SA[i]) && i < n) i++;
                return same;
            };
            bool operator!=(const LZ_SuffixArray& arr) const { return !operator==(arr); };

            friend void swap(LZ_SuffixArray& first, LZ_SuffixArray& second) {
                std::swap(first.SA, second.SA);
                std::swap(first.LCP, second.LCP);
                std::swap(first.n, second.n);
            };

        };
    }
}