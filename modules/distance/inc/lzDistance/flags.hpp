#pragma once

#include <lz/sequence.h>
#include <lz/structures.h>
#include <lz/types.h>

#include <utility>
#include <vector>

#include "lz/general.h"

namespace lz {
  namespace dist {

    /**
     * @brief Output container for LZ distance computation results.
     *
     * Stores distance matrices and complexity flags for pairwise sequence comparisons.
     * Supports information distance, shuffle distance, and directed graph representations.
     */
    struct LZ_Output {
      std::vector<lz_bool> first_calculated_complexity;  ///< Complexity computed flags for first input source
      std::vector<lz_bool>
        second_calculated_complexity;  ///< Complexity computed flags for second input source

      std::vector<std::vector<lz_double>> info_distance;     ///< Information distance matrix [i][j]
      std::vector<std::vector<lz_double>> shuffle_distance;  ///< Shuffle-based distance matrix [i][j]
      std::vector<std::vector<lz_int>>    directed_matrix;   ///< Directed graph edge weights [i][j]

      /** @brief Default constructor. Creates empty output container. */
      LZ_Output() noexcept = default;

      /**
       * @brief Constructs a square output container.
       * @param size Dimension for all matrices (size x size).
       */
      explicit LZ_Output(lz_size size)
        : info_distance(size, std::vector<lz_double>(size))
        , shuffle_distance(size, std::vector<lz_double>(size))
        , directed_matrix(size, std::vector<lz_int>(size)) {}

      /**
       * @brief Constructs a rectangular output container.
       * @param first_size  Number of rows (first input dimension).
       * @param second_size Number of columns (second input dimension).
       * @note Directed matrix uses combined dimension when inputs differ.
       */
      LZ_Output(lz_size first_size, lz_size second_size)
        : info_distance(first_size, std::vector<lz_double>(second_size))
        , shuffle_distance(first_size, std::vector<lz_double>(second_size)) {
        const auto directed_dim = (first_size != second_size) ? first_size + second_size : first_size;
        directed_matrix.assign(directed_dim, std::vector<lz_int>(directed_dim));
      }

      /** @brief Copy constructor. */
      LZ_Output(const LZ_Output&) = default;

      /** @brief Move constructor. */
      LZ_Output(LZ_Output&&) noexcept = default;

      /**
       * @brief Sets the complexity value for a sequence.
       * @param idx Sequence index.
       * @param cpx Complexity value to set.
       */
      auto setComplexity(lz_size idx, lz_uint cpx) -> void;

      /**
       * @brief Appends an information distance value for sequence at index.
       * @param idx Row index in the distance matrix.
       * @param dist Distance value to append.
       */
      auto setDistance(lz_size idx, lz_double dist) -> void;

      /**
       * @brief Sets the entire row of information distances for a sequence.
       * @param idx Row index in the distance matrix.
       * @param distances Vector of distance values (moved into storage).
       */
      auto setDistance(lz_size idx, std::vector<lz_double> distances) -> void;

      /**
       * @brief Sets a specific information distance value.
       * @param i Row index.
       * @param j Column index.
       * @param dist Distance value to set.
       * @throws LZOutOfBounds if indices are out of range.
       */
      auto setDistance(lz_size i, lz_size j, lz_double dist) -> void;

      /**
       * @brief Appends a shuffle distance value for sequence at index.
       * @param idx Row index in the shuffle distance matrix.
       * @param dist Distance value to append.
       */
      auto setDistanceShuffle(lz_size idx, lz_double dist) -> void;

      /**
       * @brief Sets the entire row of shuffle distances for a sequence.
       * @param idx Row index in the shuffle distance matrix.
       * @param distances Vector of distance values (moved into storage).
       */
      auto setDistanceShuffle(lz_size idx, std::vector<lz_double> distances) -> void;

      /**
       * @brief Sets a specific shuffle distance value.
       * @param i Row index.
       * @param j Column index.
       * @param dist Distance value to set.
       * @throws LZOutOfBounds if indices are out of range.
       */
      auto setDistanceShuffle(lz_size i, lz_size j, lz_double dist) -> void;

      /**
       * @brief Appends a directed edge value for sequence at index.
       * @param idx Row index in the directed matrix.
       * @param val Edge weight to append.
       */
      auto setDirectionValue(lz_size idx, lz_int val) -> void;

      /**
       * @brief Sets the entire row of directed edge values.
       * @param idx Row index in the directed matrix.
       * @param vals Vector of edge weights (moved into storage).
       */
      auto setDirectionValue(lz_size idx, std::vector<lz_int> vals) -> void;

      /**
       * @brief Sets a specific directed edge value.
       * @param i Row index (source node).
       * @param j Column index (target node).
       * @param val Edge weight (positive = j→i direction, negative = i→j).
       * @throws LZOutOfBounds if indices are out of range.
       */
      auto setDirectionValue(lz_size i, lz_size j, lz_int val) -> void;

      /**
       * @brief Allocates a square info distance matrix.
       * @param size Dimension (size x size).
       */
      auto reserveInfoDistance(lz_size size) -> void;

      /**
       * @brief Allocates a rectangular info distance matrix.
       * @param rows Number of rows.
       * @param cols Number of columns.
       */
      auto reserveInfoDistance(lz_size rows, lz_size cols) -> void;

      /**
       * @brief Allocates a square shuffle distance matrix.
       * @param size Dimension (size x size).
       */
      auto reserveShuffleDistance(lz_size size) -> void;

      /**
       * @brief Allocates a rectangular shuffle distance matrix.
       * @param rows Number of rows.
       * @param cols Number of columns.
       */
      auto reserveShuffleDistance(lz_size rows, lz_size cols) -> void;

      /**
       * @brief Allocates a square directed graph matrix.
       * @param size Dimension (size x size).
       */
      auto reserveDirectionGraph(lz_size size) -> void;

      /**
       * @brief Allocates a rectangular directed graph matrix.
       * @param rows Number of rows.
       * @param cols Number of columns.
       */
      auto reserveDirectionGraph(lz_size rows, lz_size cols) -> void;

      /**
       * @brief Validates capacity for the given index.
       * @param idx Index to validate.
       */
      auto checkCapacity(lz_size idx) -> void;

      /**
       * @brief Copy assignment operator using copy-and-swap idiom.
       * @param rhs Source object (passed by value for copy).
       * @return Reference to this object.
       */
      LZ_Output& operator=(LZ_Output rhs) noexcept {
        swap(*this, rhs);
        return *this;
      }

      /**
       * @brief Move assignment operator.
       * @param rhs Source object to move from.
       * @return Reference to this object.
       */
      LZ_Output& operator=(LZ_Output&& rhs) noexcept = default;

      /**
       * @brief Swaps two LZ_Output objects.
       * @param lhs First object.
       * @param rhs Second object.
       */
      friend void swap(LZ_Output& lhs, LZ_Output& rhs) noexcept {
        using std::swap;
        swap(lhs.first_calculated_complexity, rhs.first_calculated_complexity);
        swap(lhs.second_calculated_complexity, rhs.second_calculated_complexity);
        swap(lhs.info_distance, rhs.info_distance);
        swap(lhs.shuffle_distance, rhs.shuffle_distance);
        swap(lhs.directed_matrix, rhs.directed_matrix);
      }
    };

    /** @brief Selector for input data source. */
    enum INPUT_DATA {
      FIRST,  ///< First input source
      SECOND  ///< Second input source
    };

    /**
     * @brief Configuration flags and input data for LZ distance computation.
     *
     * Contains input sequences, algorithm parameters, and mode flags for
     * controlling the distance calculation behavior.
     */
    struct LZ_Flags {
      details::LineRange first_range{};   ///< Line range filter for first input
      details::LineRange second_range{};  ///< Line range filter for second input
      utils::LZ_Args     sa_args{};       ///< Suffix array and LZ algorithm parameters

      std::vector<sequence> first_input;   ///< First set of input sequences
      std::vector<sequence> second_input;  ///< Second set of input sequences

      lz_int matrix_threshold
        = std::numeric_limits<lz::lz_int>::max();  ///< Threshold use for obtain the directed graph

      lz_bool revert_     : 1 = false;  ///< Reverse the second sequences before comparison
      lz_bool text_       : 1 = false;  ///< Treat input as plain text
      lz_bool binary_     : 1 = false;  ///< Binary alphabet mode (2 symbols)
      lz_bool adn_        : 1 = false;  ///< DNA alphabet mode (4 symbols: ACGT)
      lz_bool arn_        : 1 = false;  ///< RNA alphabet mode (4 symbols: ACGU)
      lz_bool trajectory_ : 1 = false;  ///< Trajectory sequence mode

      /**
       * @brief Constructs flags with a single sequence (self-comparison).
       * @param text Input sequence.
       * @param args Algorithm parameters (optional).
       */
      explicit LZ_Flags(sequence text, utils::LZ_Args args = {})
        : sa_args(std::move(args)), first_input{std::move(text)}, second_input(first_input) {}

      /**
       * @brief Constructs flags with two sequences for pairwise comparison.
       * @param text1 First input sequence.
       * @param text2 Second input sequence.
       * @param args Algorithm parameters (optional).
       */
      LZ_Flags(sequence text1, sequence text2, utils::LZ_Args args = {})
        : sa_args(std::move(args)), first_input{std::move(text1)}, second_input{std::move(text2)} {}

      /**
       * @brief Constructs flags with a vector of sequences (all-vs-all comparison).
       * @param data Input sequences (used for both first and second input).
       * @param args Algorithm parameters (optional).
       */
      explicit LZ_Flags(std::vector<sequence> data, utils::LZ_Args args = {})
        : sa_args(std::move(args)), first_input(std::move(data)), second_input(first_input) {}

      /**
       * @brief Constructs flags with two vectors for cross-comparison.
       * @param data1 First set of input sequences.
       * @param data2 Second set of input sequences.
       * @param args Algorithm parameters (optional).
       */
      LZ_Flags(std::vector<sequence> data1, std::vector<sequence> data2, utils::LZ_Args args = {})
        : sa_args(std::move(args)), first_input(std::move(data1)), second_input(std::move(data2)) {}

      /** @brief Copy constructor. */
      LZ_Flags(const LZ_Flags&) = default;

      /** @brief Move constructor. */
      LZ_Flags(LZ_Flags&&) noexcept = default;

      /** @brief Destructor. */
      ~LZ_Flags() = default;

      /** @brief Returns the start line of the first range filter. */
      lz_int first_dist_init() const noexcept { return first_range.init; }

      /** @brief Returns the end line of the first range filter. */
      lz_int first_dist_end() const noexcept { return first_range.end; }

      /** @brief Returns the start line of the second range filter. */
      lz_int second_dist_init() const noexcept { return second_range.init; }

      /** @brief Returns the end line of the second range filter. */
      lz_int second_dist_end() const noexcept { return second_range.end; }

      /**
       * @brief Adds a single sequence to the specified input source.
       * @param in Target input source (FIRST or SECOND).
       * @param data Sequence string to add.
       */
      void addData(INPUT_DATA in, std::string data) {
        auto& target = (in == FIRST) ? first_input : second_input;
        target.emplace_back(std::move(data));
      }

      /**
       * @brief Adds multiple sequences to the specified input source.
       * @param in Target input source (FIRST or SECOND).
       * @param data Vector of sequence strings to add.
       */
      void addData(INPUT_DATA in, const std::vector<std::string>& data) {
        auto& target = (in == FIRST) ? first_input : second_input;
        target.reserve(target.size() + data.size());
        for (const auto& elem: data) {
          target.emplace_back(elem);
        }
      }

      /**
       * @brief Copy assignment operator using copy-and-swap idiom.
       * @param rhs Source object (passed by value for copy).
       * @return Reference to this object.
       */
      LZ_Flags& operator=(LZ_Flags rhs) noexcept {
        swap(*this, rhs);
        return *this;
      }

      /**
       * @brief Move assignment operator.
       * @return Reference to this object.
       */
      LZ_Flags& operator=(LZ_Flags&&) noexcept = default;

      /**
       * @brief Swaps two LZ_Flags objects.
       * @param lhs First object.
       * @param rhs Second object.
       */
      friend void swap(LZ_Flags& lhs, LZ_Flags& rhs) noexcept {
        using std::swap;
        swap(lhs.first_range, rhs.first_range);
        swap(lhs.second_range, rhs.second_range);
        swap(lhs.sa_args, rhs.sa_args);
        swap(lhs.first_input, rhs.first_input);
        swap(lhs.second_input, rhs.second_input);
        // Bit-fields require manual swap via temporaries
        auto tmp_revert = lhs.revert_;
        lhs.revert_ = rhs.revert_;
        rhs.revert_ = tmp_revert;
        auto tmp_text = lhs.text_;
        lhs.text_ = rhs.text_;
        rhs.text_ = tmp_text;
        auto tmp_binary = lhs.binary_;
        lhs.binary_ = rhs.binary_;
        rhs.binary_ = tmp_binary;
        auto tmp_adn = lhs.adn_;
        lhs.adn_ = rhs.adn_;
        rhs.adn_ = tmp_adn;
        auto tmp_arn = lhs.arn_;
        lhs.arn_ = rhs.arn_;
        rhs.arn_ = tmp_arn;
        auto tmp_traj = lhs.trajectory_;
        lhs.trajectory_ = rhs.trajectory_;
        rhs.trajectory_ = tmp_traj;
      }

      /**
       * @brief Equality comparison operator.
       * @param lhs First object.
       * @param rhs Second object.
       * @return True if inputs and arguments are equal.
       */
      friend bool operator==(const LZ_Flags& lhs, const LZ_Flags& rhs) {
        return lhs.first_input == rhs.first_input && lhs.second_input == rhs.second_input
          && lhs.sa_args == rhs.sa_args;
      }

      /**
       * @brief Inequality comparison operator.
       * @param lhs First object.
       * @param rhs Second object.
       * @return True if objects are not equal.
       */
      friend bool operator!=(const LZ_Flags& lhs, const LZ_Flags& rhs) { return !(lhs == rhs); }
    };

  }  // namespace dist
}  // namespace lz