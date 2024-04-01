#pragma once

#include <lz/utils.h>

#include <vector>

namespace lz {
   namespace utils {
      enum SA_ALG { sais, caps };

      struct SA_Args {
         /* Args for CaPS class */
         lz_int chunks      = 0;  //?> Number of chunks to divide the sequence (use by CaPS)
         lz_int max_context = 0;  //?> Max length to compare prefix for generate the LCP (use by CaPS)

         SA_Args() = default;
         SA_Args(lz_int chunks)
           : chunks(chunks){};
         SA_Args(lz_int chunks, lz_int max_context)
           : chunks(chunks), max_context(max_context){};
         SA_Args(const SA_Args& sa) = default;
         SA_Args(SA_Args&& sa)      = default;

         SA_Args& operator=(SA_Args rhs) {
            std::swap(this->chunks, rhs.chunks);
            std::swap(this->max_context, rhs.max_context);
            return *this;
         };

         friend bool operator==(SA_Args sa1, SA_Args sa2) {
            return sa1.chunks == sa2.chunks && sa1.max_context == sa2.max_context;
         };
         friend bool operator!=(SA_Args sa1, SA_Args sa2) { return !(sa1 == sa2); };

         friend void swap(SA_Args& sa1, SA_Args& sa2) {
            std::swap(sa1.chunks, sa2.chunks);
            std::swap(sa1.max_context, sa2.max_context);
         };
      };

      struct LZ_SuffixArray {
         std::vector<lz_uint> SA;   //!> Suffix array values
         std::vector<lz_uint> LCP;  //!> Longest Common Prefix values
         lz_uint              n;    //!> Length of the string (n+1 size of the vectors)

         LZ_SuffixArray(void)
           : SA(), LCP(), n(0){};
         LZ_SuffixArray(std::vector<lz_uint> SA_, std::vector<lz_uint> LCP_, const lz_uint n_)
           : SA(std::move(SA_)), LCP(std::move(LCP_)), n(n_){};
         LZ_SuffixArray(std::vector<lz_uint> SA_, const lz_uint n_)
           : SA(std::move(SA_)), LCP(), n(n_){};
         LZ_SuffixArray(lz_uint* const SA_, lz_uint* const LCP_, lz_uint n_)
           : n(n_) {
            SA  = std::vector<lz_uint>(SA_, SA_ + n_);
            LCP = std::vector<lz_uint>(LCP_, LCP_ + n_);
         }
         LZ_SuffixArray(lz_int* const SA_, lz_int* const LCP_, lz_int n_)
           : n(n_) {
            SA  = std::vector<lz_uint>(SA_, SA_ + n_);
            LCP = std::vector<lz_uint>(LCP_, LCP_ + n_);
         }
         LZ_SuffixArray(const LZ_SuffixArray& oth)
           : SA(oth.SA), LCP(oth.LCP), n(oth.n) {}
         LZ_SuffixArray(LZ_SuffixArray&& oth) noexcept
           : SA(std::move(oth.SA)), LCP(std::move(oth.LCP)), n(std::move(oth.n)) {}

         void Clear() {
            SA.clear();
            LCP.clear();
            n = 0;
         };
         ~LZ_SuffixArray(){};

         LZ_SuffixArray& operator=(LZ_SuffixArray rhs) {
            std::swap((*this).SA, rhs.SA);
            std::swap((*this).LCP, rhs.LCP);
            std::swap((*this).n, rhs.n);

            return *this;
         };

         bool operator==(const LZ_SuffixArray& arr) const {
            bool same = n == arr.n;

            if (!same)
               return false;

            lz_uint i = 0;
            while ((same = SA[i] == arr.SA[i]) && i < n)
               i++;
            return same;
         };
         bool operator!=(const LZ_SuffixArray& arr) const { return !operator==(arr); };

         friend void swap(LZ_SuffixArray& first, LZ_SuffixArray& second) {
            std::swap(first.SA, second.SA);
            std::swap(first.LCP, second.LCP);
            std::swap(first.n, second.n);
         };
      };
   }  // namespace utils
}  // namespace lz