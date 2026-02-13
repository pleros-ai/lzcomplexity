#pragma once

#include <lz/exceptions.h>
#include <lz/utils.h>

#include <vector>

/**
 * @file sa_structures.h
 * @brief Data structures for suffix array construction and storage.
 */
namespace lz {
  namespace utils {

    /**
     * @brief Enumeration of available suffix array construction algorithms.
     */
    enum SA_ALG {
      sais,  ///< SA-IS (Suffix Array by Induced Sorting) algorithm.
      caps   ///< CaPS (Cache-friendly Parallel Suffix array) algorithm.
    };

    /**
     * @brief Configuration parameters for suffix array construction.
     *
     * Contains parameters that control the behavior of suffix array
     * construction algorithms, particularly CaPS.
     */
    struct SA_Args {
      lz_int chunks = 0;       ///< Number of subproblems for parallel construction (0 = auto).
      lz_int max_context = 0;  ///< Maximum prefix length for suffix comparison (0 = unlimited).

      /**
       * @brief Default constructor.
       */
      SA_Args() = default;

      /**
       * @brief Constructs with specified chunk count.
       * @param chunks Number of subproblems for parallelization.
       */
      SA_Args(lz_int chunks)
        : chunks(chunks) {};

      /**
       * @brief Constructs with chunk count and max context.
       * @param chunks Number of subproblems for parallelization.
       * @param max_context Maximum prefix length for comparisons.
       */
      SA_Args(lz_int chunks, lz_int max_context)
        : chunks(chunks), max_context(max_context) {};

      /**
       * @brief Copy constructor.
       */
      SA_Args(const SA_Args& sa) = default;

      /**
       * @brief Move constructor.
       */
      SA_Args(SA_Args&& sa) = default;

      /**
       * @brief Copy-and-swap assignment operator.
       * @param rhs The arguments to assign from.
       * @return Reference to this object.
       */
      SA_Args& operator=(SA_Args rhs) {
        std::swap(this->chunks, rhs.chunks);
        std::swap(this->max_context, rhs.max_context);
        return *this;
      };

      /**
       * @brief Equality comparison operator.
       * @return true if all parameters match.
       */
      friend bool operator==(SA_Args sa1, SA_Args sa2) {
        return sa1.chunks == sa2.chunks && sa1.max_context == sa2.max_context;
      };

      /**
       * @brief Inequality comparison operator.
       * @return true if any parameter differs.
       */
      friend bool operator!=(SA_Args sa1, SA_Args sa2) { return !(sa1 == sa2); };

      /**
       * @brief Swaps the contents of two SA_Args objects.
       * @param sa1 First object.
       * @param sa2 Second object.
       */
      friend void swap(SA_Args& sa1, SA_Args& sa2) {
        std::swap(sa1.chunks, sa2.chunks);
        std::swap(sa1.max_context, sa2.max_context);
      };
    };

    /**
     * @brief Container for suffix array and LCP array data.
     *
     * Stores the suffix array (SA) and Longest Common Prefix (LCP) array
     * computed for a given text, along with the text length.
     */
    struct LZ_SuffixArray {
      std::vector<lz_uint> SA;  ///< Suffix array: SA[i] is the starting position of the i-th smallest suffix.
      std::vector<lz_uint> LCP;  ///< LCP array: LCP[i] is the longest common prefix of SA[i] and SA[i-1].
      lz_uint              n;    ///< Length of the original text.

      /**
       * @brief Default constructor. Creates empty arrays with n=0.
       */
      LZ_SuffixArray(void)
        : SA(), LCP(), n(0) {};

      /**
       * @brief Constructs from vectors of any integral type.
       * @tparam Int Integral type for input vectors.
       * @param SA_ Suffix array values.
       * @param LCP_ LCP array values.
       * @param n_ Length of the original text.
       * @throws BadInitialization If Int is not an integral type.
       */
      template<typename Int>
#ifdef __cpp_concepts
        requires std::is_integral_v<Int>
#endif
      LZ_SuffixArray(std::vector<Int> SA_, std::vector<Int> LCP_, const lz_uint n_)
        : n(n_) {
        if (std::numeric_limits<Int>::is_integer) {
          SA = std::vector<lz_uint>{SA_.begin(), SA_.end()};
          LCP = std::vector<lz_uint>{LCP_.begin(), LCP_.end()};
        } else {
          throw BadInitialization("LZ_SuffixArray: std::vector of type" + std::string(typeid(Int).name())
                                  + " is not integral");
        }
      };

      /**
       * @brief Constructs with suffix array only (no LCP).
       * @param SA_ Suffix array values.
       * @param n_ Length of the original text.
       */
      LZ_SuffixArray(std::vector<lz_uint> SA_, const lz_uint n_)
        : LZ_SuffixArray(SA_, std::vector<lz_uint>{}, n_) {};

      /**
       * @brief Constructs from raw unsigned integer pointers.
       * @param SA_ Pointer to suffix array data.
       * @param LCP_ Pointer to LCP array data.
       * @param n_ Length of the arrays.
       */
      LZ_SuffixArray(lz_uint* const SA_, lz_uint* const LCP_, lz_uint n_)
        : n(n_) {
        SA = std::vector<lz_uint>(SA_, SA_ + n_);
        LCP = std::vector<lz_uint>(LCP_, LCP_ + n_);
      };

      /**
       * @brief Constructs from raw signed integer pointers.
       * @param SA_ Pointer to suffix array data.
       * @param LCP_ Pointer to LCP array data.
       * @param n_ Length of the arrays.
       */
      LZ_SuffixArray(lz_int* const SA_, lz_int* const LCP_, lz_int n_)
        : n(n_) {
        SA = std::vector<lz_uint>(SA_, SA_ + n_);
        LCP = std::vector<lz_uint>(LCP_, LCP_ + n_);
      };

      /**
       * @brief Copy constructor.
       * @param oth The LZ_SuffixArray to copy.
       */
      LZ_SuffixArray(const LZ_SuffixArray& oth)
        : SA(oth.SA), LCP(oth.LCP), n(oth.n) {};

      /**
       * @brief Move constructor.
       * @param oth The LZ_SuffixArray to move from.
       */
      LZ_SuffixArray(LZ_SuffixArray&& oth) noexcept { *this = std::move(oth); };

      /**
       * @brief Clears all data and resets n to 0.
       */
      void Clear() {
        SA.clear();
        LCP.clear();
        n = 0;
      };

      /**
       * @brief Copy assignment operator (swap-based).
       * @param rhs The object to assign from.
       * @return Reference to this object.
       */
      LZ_SuffixArray& operator=(LZ_SuffixArray& rhs) {
        if (*this != rhs) {
          std::swap(*this, rhs);
        }

        return *this;
      };

      /**
       * @brief Move assignment operator.
       * @param rhs The object to move from.
       * @return Reference to this object.
       */
      LZ_SuffixArray& operator=(LZ_SuffixArray&& rhs) {
        SA = std::move(rhs.SA);
        LCP = std::move(rhs.LCP);
        n = std::exchange(rhs.n, std::numeric_limits<lz_uint>::max());

        return *this;
      };

      /**
       * @brief Equality comparison based on suffix array content.
       * @param arr The object to compare with.
       * @return true if both have same length and identical SA values.
       */
      bool operator==(const LZ_SuffixArray& arr) const {
        bool same = n == arr.n;

        if (!same) return false;

        lz_uint i = 0;
        while ((same = SA[i] == arr.SA[i]) && i < n) i++;
        return same;
      };

      /**
       * @brief Inequality comparison.
       * @param arr The object to compare with.
       * @return true if objects differ.
       */
      bool operator!=(const LZ_SuffixArray& arr) const { return !operator==(arr); };

      /**
       * @brief Swaps the contents of two LZ_SuffixArray objects.
       * @param first First object.
       * @param second Second object.
       */
      friend void swap(LZ_SuffixArray& first, LZ_SuffixArray& second) {
        std::swap(first.SA, second.SA);
        std::swap(first.LCP, second.LCP);
        std::swap(first.n, second.n);
      };
    };
  }  // namespace utils
}  // namespace lz
