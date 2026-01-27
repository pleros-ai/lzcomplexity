/***************************************************************************
                                                                  sequence.h  -
 description
                                                                          -------------------
         begin                : 16 Nov 2013
         last modified        : 20 Feb 2024
         email                : estevez@fisica.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013-2018 by Ernesto Estevez Rams
 ** estevez@fisica.uh.cu
 **
 *																		   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <lz/exceptions.h>
#include <lz/general.h>

#include <set>

/**
 * @file sequence.h
 * @brief This file contains the declaration of the `sequence` class and related functions.
 */
namespace lz {
   // The error condition exceptions
   class SequenceError : public Errors {};
   class SequenceBadAlloc : public SequenceError {};
   class SequenceNoMatchSize : public SequenceError {};
   class SequenceOutOfBounds : public SequenceError {};

   //.....................................................
   // The structure that stores the actual sequence data
   //.....................................................
   /// \brief structure that stores the actual sequence data
   class sequence {
  protected:
      std::vector<char> seq;  //!< the sequence
      std::vector<char> alphabet;
      lz_uint           alphabet_size;

  public:
      /**
       * @brief Default constructor.
       */
      sequence(void)
        : alphabet_size(ALPHABET_SIZE){};

      /**
       * @brief Constructor that sets the alphabet size.
       * @param alphsize The size of the alphabet.
       */
      sequence(lz_int alphsize)
        : alphabet_size(alphsize){};

      /**
       * @brief Constructor that initializes the sequence with a vector of characters.
       * @param vec The vector of characters.
       */
      sequence(const std::vector<char> vec);

      /**
       * @brief Constructor that initializes the sequence with a string.
       * @param str The string.
       */
      sequence(const std::string str);
      sequence(const std::string_view str);

      /**
       * @brief Constructor that initializes the sequence with a vector of characters and a specified alphabet size.
       * @param vec The vector of characters.
       * @param aph The alphabet size.
       */
      sequence(const std::vector<char> vec, lz_uint aph)
        : seq(vec), alphabet_size(aph) {
         DetermineAlphabet();
      };

      /**
       * @brief Constructor that initializes the sequence with a string and a specified alphabet size.
       * @param str The string.
       * @param aph The alphabet size.
       */
      sequence(const std::string str, lz_uint aph)
        : seq(str.begin(), str.end()), alphabet_size(aph) {
         DetermineAlphabet();
      };

      /**
       * @brief Copy constructor.
       * @param s The sequence to be copied.
       */
      sequence(const sequence& s)
        : seq(s.seq), alphabet(s.alphabet), alphabet_size(s.alphabet_size){};

      /**
       * @brief Move constructor.
       * @param s The sequence to be moved.
       */
      sequence(sequence&& s)
        : seq(std::move(s.seq)), alphabet(std::move(s.alphabet)), alphabet_size(std::exchange(s.alphabet_size, 0x01)){};

      /**
       * @brief Destructor.
       */
      ~sequence() {
         alphabet_size = ALPHABET_SIZE;
         seq.clear();
         alphabet.clear();
      };

      /**
       * @brief Returns the current alphabet size.
       * @return The number of distinct symbols in the alphabet.
       */
      constexpr lz_uint getAlphabetSize(void) const { return alphabet_size; };

      /**
       * @brief Computes and sets the alphabet size based on the current sequence content.
       * @return The computed alphabet size.
       */
      lz_uint setAlphabetSize(void);

      /**
       * @brief Manually sets the alphabet size to a specified value.
       * @param _alphabet_size The new alphabet size.
       */
      void setAlphabetSize(lz_uint);

      /**
       * @brief Returns a copy of the current alphabet.
       * @return A vector containing the distinct characters in the alphabet.
       */
      std::vector<char> getAlphabet(void) const { return alphabet; };

      /**
       * @brief Computes and sets the alphabet from the current sequence content.
       * @return A vector containing the computed alphabet.
       */
      std::vector<char> setAlphabet(void);

      /**
       * @brief Manually sets the alphabet to a specified vector of characters.
       * @param alph The new alphabet.
       */
      void setAlphabet(std::vector<char>);

      /**
       * @brief Counts the number of zero-valued characters in the sequence.
       * @return The count of characters equal to '\0' (null character).
       */
      lz_uint NoZeroes(void) const;

      /**
       * @brief Converts the sequence to a standard string.
       * @return A string representation of the sequence.
       */
      std::string toString(void) const;

      /**
       * @brief Computes the frequency of each character in the sequence.
       * @return A map where keys are characters and values are their counts.
       */
      std::map<char, lz_double> charDensity(void) const;

      /**
       * @brief Accesses the character at the specified index (bounds-checked).
       * @param index The position of the character to access.
       * @return A reference to the character at the given index.
       * @throws SequenceOutOfBounds If the index is out of range.
       */
      char& operator[](lz_size);

      /**
       * @brief Accesses the character at the specified index (bounds-checked, const version).
       * @param index The position of the character to access.
       * @return A const reference to the character at the given index.
       * @throws SequenceOutOfBounds If the index is out of range.
       */
      const char& operator[](lz_size) const;

      /**
       * @brief Returns a reference to the first character in the sequence.
       * @return A reference to the first character.
       * @note Behavior is undefined if the sequence is empty.
       */
      char& first(void);

      /**
       * @brief Returns a const reference to the first character in the sequence.
       * @return A const reference to the first character.
       * @note Behavior is undefined if the sequence is empty.
       */
      const char& first(void) const;

      /**
       * @brief Returns a reference to the last character in the sequence.
       * @return A reference to the last character.
       * @note Behavior is undefined if the sequence is empty.
       */
      char& last(void);

      /**
       * @brief Returns a const reference to the last character in the sequence.
       * @return A const reference to the last character.
       * @note Behavior is undefined if the sequence is empty.
       */
      const char& last(void) const;

      /**
       * @brief Accesses the character at the specified index using std::vector::at().
       * @param index The position of the character to access.
       * @return A reference to the character at the given index.
       * @throws std::out_of_range If the index is out of range.
       */
      char& at(lz_size);

      /**
       * @brief Accesses the character at the specified index using std::vector::at() (const version).
       * @param index The position of the character to access.
       * @return A const reference to the character at the given index.
       * @throws std::out_of_range If the index is out of range.
       */
      const char& at(lz_size index) const;

      /**
       * @brief Finds the minimum character value in the entire sequence.
       * @return The smallest character in the sequence.
       */
      char Min(void) const;

      /**
       * @brief Finds the minimum character value within a specified range.
       * @param start The starting index (inclusive).
       * @param final The ending index (exclusive).
       * @return The smallest character in the range [start, final).
       */
      char Min(lz_size start, lz_size final) const;

      /**
       * @brief Finds the maximum character value in the entire sequence.
       * @return The largest character in the sequence.
       */
      char Max(void) const;

      /**
       * @brief Finds the maximum character value within a specified range.
       * @param start The starting index (inclusive).
       * @param final The ending index (exclusive).
       * @return The largest character in the range [start, final).
       */
      char Max(lz_size start, lz_size final) const;

      /**
       * @brief Appends a character to the end of the sequence.
       * @param c The character to append.
       * @return The new size of the sequence after insertion.
       * @throws SequenceBadAlloc If memory allocation fails.
       */
      lz_size push(char);

      /**
       * @brief Removes the last character from the sequence.
       * @return The new size of the sequence after removal.
       * @note Behavior is undefined if the sequence is empty.
       */
      lz_size pop(void);

      /**
       * @brief Returns a copy of the last character in the sequence.
       * @return The last character.
       * @note Behavior is undefined if the sequence is empty.
       */
      char back(void) const;

      /**
       * @brief Returns the number of characters in the sequence.
       * @return The size of the sequence.
       */
      lz_size size(void) const;

      /**
       * @brief Returns the number of characters in the sequence (alias for size()).
       * @return The length of the sequence.
       */
      lz_size length(void) const;

      /**
       * @brief Returns a copy of the internal character vector.
       * @return A vector containing all characters in the sequence.
       */
      std::vector<char> SequenceVector(void) const;

      /**
       * @brief Creates a new sequence containing the first l characters.
       * @param l The number of characters to take from the beginning.
       * @return A new sequence with the first l characters.
       * @note If l exceeds the sequence size, the result is padded or truncated.
       */
      sequence Take(lz_size l) const;

      /**
       * @brief Creates a new sequence with the first l characters removed.
       * @param l The number of characters to drop from the beginning.
       * @return A new sequence without the first l characters.
       */
      sequence Drop(lz_size l) const;

      /**
       * @brief Splits the sequence at position l into two sequences.
       * @param l The position at which to split.
       * @return A pair where first contains [0, l) and second contains [l, end).
       */
      std::pair<sequence, sequence> Split(lz_size l) const;

      /**
       * @brief Creates a coarse-grained version of the sequence.
       * @param gr The granularity factor.
       * @return A new sequence with reduced resolution.
       * @todo [REVIEW] Clarify the exact transformation applied by this method.
       */
      sequence Granularity(lz_uint gr) const;

      /**
       * @brief Removes the last character from the sequence and returns a reference to this sequence.
       * @return A reference to the modified sequence.
       * @note This is equivalent to pop() but returns *this for chaining.
       */
      sequence& pi(void);

      // sequence& negate(void);

      /**
       * @brief Reverses the sequence in place.
       * @return A reference to the reversed sequence.
       */
      sequence& reverse(void);

      /**
       * @brief Creates a reversed copy of the sequence.
       * @return A new sequence with characters in reverse order.
       */
      sequence reverseCopy(void);

      /**
       * @brief Creates a reversed copy of the sequence (const version).
       * @return A new sequence with characters in reverse order.
       */
      const sequence reverseCopy(void) const;

      /**
       * @brief Shifts all characters to the right by a specified amount. The operation is circular shift.
       * @param ls The number of positions to shift (default: 1).
       * @return A reference to the modified sequence.
       */
      sequence& rightShift(lz_uint ls = 1);

      /**
       * @brief Shifts all characters to the left by a specified amount. The operation is circular shift.
       * @param ls The number of positions to shift (default: 1).
       * @return A reference to the modified sequence.
       */
      sequence& leftShift(lz_uint ls = 1);

      /**
       * @brief Copy assignment operator (swap-based).
       * @param s The sequence to copy from.
       * @return A reference to this sequence.
       */
      sequence& operator=(sequence&);

      /**
       * @brief Copy assignment operator (const version).
       * @param s The sequence to copy from.
       * @return A reference to this sequence.
       */
      sequence& operator=(const sequence&);

      /**
       * @brief Move assignment operator.
       * @param s The sequence to move from.
       * @return A reference to this sequence.
       */
      sequence& operator=(sequence&&);

      /**
       * @brief Assignment operator from a character vector.
       * @param s The vector to assign from.
       * @return A reference to this sequence.
       * @throws SequenceBadAlloc If memory allocation fails.
       */
      sequence& operator=(const std::vector<char>&);

      /**
       * @brief Appends a character vector to the end of this sequence.
       * @param s The vector to append.
       * @return A reference to this sequence.
       * @throws SequenceBadAlloc If memory allocation fails.
       */
      sequence& operator+=(const std::vector<char>& s);

      /**
       * @brief Appends another sequence to the end of this sequence.
       * @param s The sequence to append.
       * @return A reference to this sequence.
       * @throws SequenceBadAlloc If memory allocation fails.
       */
      sequence& operator+=(const sequence& s);

      /**
       * @brief Element-wise XOR comparison with another sequence.
       *
       * Sets each position to 0 if characters match, 1 otherwise.
       *
       * @param s The sequence to compare against.
       * @return A reference to this sequence.
       * @throws SequenceNoMatchSize If the sequences have different sizes.
       * @note The result is a binary sequence (0s and 1s).
       */
      sequence& operator^=(const sequence& s);

      /**
       * @brief Analyzes the sequence and determines the unique alphabet.
       *
       * Scans all characters, removes duplicates, sorts in descending order,
       * and updates both the alphabet and alphabet_size members.
       *
       * @return A vector containing the unique characters (sorted descending).
       */
      std::vector<char> DetermineAlphabet(void);

      /**
       * @brief Clears the sequence and resets the alphabet size to default.
       */
      void clear();

      /**
       * @brief Outputs the sequence to an output stream.
       *
       * Prints the alphabet, alphabet size, and the sequence content.
       *
       * @param os The output stream.
       * @param obj The sequence to output.
       * @return A reference to the output stream.
       */
      friend std::ostream& operator<<(std::ostream&, const sequence&);

      /**
       * @brief Reads a sequence from an input stream.
       *
       * Reads a single line and stores each character in the sequence.
       *
       * @param is The input stream.
       * @param obj The sequence to populate.
       * @return A reference to the input stream.
       * @throws SequenceBadAlloc If memory allocation fails.
       */
      friend std::istream& operator>>(std::istream&, sequence&);

      /**
       * @brief Swaps the contents of two sequences.
       * @param first The first sequence.
       * @param second The second sequence.
       */
      friend void swap(sequence&, sequence&);

      /**
       * @brief Swaps two ranges within a single sequence.
       * @param s The sequence to modify.
       * @param start1 The starting index of the first range.
       * @param start2 The starting index of the second range.
       * @param length The length of each range to swap.
       */
      friend void swap(sequence&, lz_size start1, lz_size start2, lz_size length);

      /**
       * @brief Concatenates two sequences.
       * @param lhs The left-hand sequence.
       * @param rhs The right-hand sequence.
       * @return A new sequence containing lhs followed by rhs.
       */
      friend sequence operator+(const sequence&, const sequence&);

      /**
       * @brief Compares two sequences for equality.
       * @return true if the sequences contain identical characters.
       */
      friend bool operator==(const sequence&, const sequence&);

      /**
       * @brief Compares a sequence with a string for equality.
       * @return true if the sequence matches the string content.
       */
      friend bool operator==(const sequence&, const std::string&);

      /**
       * @brief Compares two sequences for inequality.
       * @return true if the sequences differ.
       */
      friend bool operator!=(const sequence&, const sequence&);

      /**
       * @brief Compares a sequence with a string for inequality.
       * @return true if the sequence differs from the string.
       */
      friend bool operator!=(const sequence&, const std::string&);

      /**
       * @brief Lexicographically compares two sequences (greater than).
       * @return true if lhs is lexicographically greater than rhs.
       */
      friend bool operator>(const sequence&, const sequence&);

      /**
       * @brief Lexicographically compares a sequence with a string (greater than).
       * @return true if the sequence is lexicographically greater than the string.
       */
      friend bool operator>(const sequence&, const std::string&);

      /**
       * @brief Lexicographically compares two sequences (greater than or equal).
       * @return true if lhs is lexicographically greater than or equal to rhs.
       */
      friend bool operator>=(const sequence&, const sequence&);

      /**
       * @brief Lexicographically compares a sequence with a string (greater than or equal).
       * @return true if the sequence is lexicographically greater than or equal to the string.
       */
      friend bool operator>=(const sequence&, const std::string&);

      /**
       * @brief Lexicographically compares two sequences (less than).
       * @return true if lhs is lexicographically less than rhs.
       */
      friend bool operator<(const sequence&, const sequence&);

      /**
       * @brief Lexicographically compares a sequence with a string (less than).
       * @return true if the sequence is lexicographically less than the string.
       */
      friend bool operator<(const sequence&, const std::string&);

      /**
       * @brief Lexicographically compares two sequences (less than or equal).
       * @return true if lhs is lexicographically less than or equal to rhs.
       */
      friend bool operator<=(const sequence&, const sequence&);

      /**
       * @brief Lexicographically compares a sequence with a string (less than or equal).
       * @return true if the sequence is lexicographically less than or equal to the string.
       */
      friend bool operator<=(const sequence&, const std::string&);

      /**
       * @brief Applies a transformation function to each character in the sequence.
       * @param fn A function that takes a char and returns a char.
       * @return A new sequence with the transformed characters.
       */
      sequence map(std::function<char(char)> fn);

      /**
       * @brief Applies a transformation function to each character (const version).
       * @param fn A function that takes a char and returns a char.
       * @return A new sequence with the transformed characters.
       */
      const sequence map(std::function<char(char)> fn) const;

      /**
       * @brief Applies a transformation function to a sequence (friend version).
       * @param fn A function that takes a char and returns a char.
       * @param s The sequence to transform.
       * @return A new sequence with the transformed characters.
       */
      friend sequence map(std::function<char(char)> fn, const sequence& s);

      /**
       * @brief Applies a transformation function to a sequence (friend version, alternate order).
       * @param s The sequence to transform.
       * @param fn A function that takes a char and returns a char.
       * @return A new sequence with the transformed characters.
       */
      friend sequence map(const sequence& s, std::function<char(char)> fn);
   };

   /**
    * @brief Shuffles the sequence in place using block-based permutation.
    * @param s The sequence to shuffle.
    * @param block_size The size of blocks to permute.
    */
   void Shuffle(sequence& s, lz_uint block_size);

   /**
    * @brief Creates a shuffled copy of the sequence.
    * @param s The sequence to shuffle.
    * @param block_size The size of blocks to permute.
    * @param times The number of shuffle iterations to perform.
    * @return A new shuffled sequence.
    */
   sequence Shuffle(const sequence& s, lz_uint block_size, lz_uint times);

   //.............................................................................................................

   inline std::map<char, lz_double> sequence::charDensity(void) const {
      std::map<char, lz_double> res;

      for (auto& ch: seq) {
         if (res.contains(ch)) {
            res[ch] += 1;
         } else {
            res[ch] = 1;
         }
      }

      return res;
   }

   inline lz_uint sequence::NoZeroes(void) const {
      lz_uint acum = 0;

      for (auto s: seq)
         acum += (s == 0) ? 1 : 0;

      return acum;
   }

   inline std::string sequence::toString(void) const {
      return {seq.begin(), seq.end()};
   }

   inline sequence& sequence::operator=(sequence& s) {
      if (this != &s) {
         swap(*this, s);
      }

      return *this;
   }

   inline sequence& sequence::operator=(const sequence& s) {
      if (this != &s) {
         this->~sequence();
         new (this) sequence(s);
      }

      return *this;
   }

   inline sequence& sequence::operator=(sequence&& s) {
      seq           = std::move(s.seq);
      alphabet      = std::move(s.alphabet);
      alphabet_size = std::exchange(s.alphabet_size, std::numeric_limits<lz_uint>::max());

      return *this;
   }

   inline sequence& sequence::operator=(const std::vector<char>& s) {
      try {
         seq = s;
      } catch (std::bad_alloc& ba) {
         throw SequenceBadAlloc();
      } catch (...) {
         throw SequenceError();
      }

      return *this;
   }

   inline sequence operator+(const sequence& lhs, const sequence& rhs) {
      sequence result = lhs;
      result += rhs.SequenceVector();

      return result;
   }

   inline sequence& sequence::operator+=(const std::vector<char>& s) {
      try {
         seq.insert(seq.end(), s.begin(), s.end());
      } catch (std::bad_alloc& ba) {
         throw SequenceBadAlloc();
      } catch (...) {
         throw SequenceError();
      }

      return *this;
   }

   inline sequence& sequence::operator^=(const sequence& s) {
      std::vector<char>::const_iterator iterseq = s.seq.begin();

      if (s.seq.size() != seq.size())
         throw SequenceNoMatchSize();

      for (auto c: seq) {
         if (c == *iterseq)
            c = 0;
         else
            c = 1;
         iterseq++;
      }

      return *this;
   }

   inline sequence& sequence::operator+=(const sequence& s) {
      try {
         seq.insert(seq.end(), s.seq.begin(), s.seq.end());
      } catch (std::bad_alloc& ba) {
         throw SequenceBadAlloc();
      } catch (...) {
         throw SequenceError();
      }

      return *this;
   }

   inline std::vector<char> sequence::DetermineAlphabet(void) {
      std::vector<char> al = seq;

#ifdef __cpp_lib_ranges
      std::ranges::sort(al, std::ranges::greater());
      const auto [first, last] = std::ranges::unique(al);
      al.erase(first, last);
#else
      std::sort(al.begin(), al.end(), std::greater<char>());
      std::vector<char>::iterator last = std::unique(al.begin(), al.end());
      al.resize(last - al.begin());
#endif

      alphabet_size   = al.size();
      return alphabet = al;
   }

   inline lz_uint sequence::setAlphabetSize(void) {
      DetermineAlphabet();

      if (alphabet.size() < alphabet_size) {
         // See with what complete the alphabet
      } else {
         alphabet_size = alphabet.size();
      }

      return alphabet_size;
   }

   inline void sequence::setAlphabetSize(lz_uint _alphabet_size) {
      alphabet_size = _alphabet_size;
   }

   inline std::vector<char> sequence::setAlphabet(void) {
      DetermineAlphabet();

      return alphabet;
   }

   inline void sequence::setAlphabet(std::vector<char> alph) {
      alphabet = alph;
   }

   inline std::vector<char> sequence::SequenceVector(void) const {
      return seq;
   }

   inline char& sequence::operator[](lz_size index) {
      if (index >= seq.size()) {
         throw SequenceOutOfBounds();
      }

      return seq[index];
   }

   inline const char& sequence::operator[](lz_size index) const {
      if (index >= seq.size()) {
         throw SequenceOutOfBounds();
      }
      return seq[index];
   }

   inline char& sequence::at(lz_size index) {
      return seq.at(index);
   }

   // .............................................................................
   // Name: const_at
   //
   // Synopsis: indexing operator. No bounds check
   //
   // Parameters:
   //			 std::vector<bool>::size_type index      -------> the
   // index of the requested value
   //
   // Returns:
   //         bool ------> the index value of the sequence
   //
   // Exceptions:
   //            None
   //..............................................................................
   inline const char& sequence::at(lz_size index) const {
      return seq.at(index);
   }

   inline char& sequence::first(void) {
      return seq.front();
   }

   inline const char& sequence::first(void) const {
      return seq.front();
   }

   inline char& sequence::last(void) {
      return seq.back();
   }

   inline const char& sequence::last() const {
      return seq.back();
   }

   inline char sequence::back(void) const {
      return last();
   }

   inline char sequence::Max(void) const {
      return *std::max_element(seq.begin(), seq.end());
   }

   inline char sequence::Min(lz_size start, lz_size final) const {
      return *std::min_element(seq.begin() + start, seq.begin() + final);
   }

   inline char sequence::Max(lz_size start, lz_size final) const {
      return *std::max_element(seq.begin() + start, seq.begin() + final);
   }

   inline char sequence::Min(void) const {
      return *std::min_element(seq.begin(), seq.end());
   }

   inline lz_size sequence::push(char c) {
      try {
         seq.push_back(c);
      } catch (std::bad_alloc& ba) {
         throw SequenceBadAlloc();
      }

      return seq.size();
   }

   inline lz_size sequence::pop(void) {
      seq.pop_back();

      return seq.size();
   }

   inline void sequence::clear(void) {
      alphabet_size = ALPHABET_SIZE;
      seq.clear();
   }

   inline sequence& sequence::pi(void) {
      seq.pop_back();
      return *this;
   }

   inline lz_size sequence::size(void) const {
      return seq.size();
   }

   inline lz_size sequence::length(void) const {
      return seq.size();
   }

   inline sequence sequence::Take(lz_size l) const {
      std::vector<char> temp = seq;

      temp.resize(l);

      return sequence(temp, alphabet_size);
   }

   // .......................................................
   //                friend functions
   //........................................................
   inline void swap(sequence& first, sequence& second) {
      std::swap(first.alphabet_size, second.alphabet_size);
      std::swap(first.seq, second.seq);
   }

   inline void swap(sequence& s, lz_size start1, lz_size start2, lz_size length) {
#ifdef __cpp_lib_ranges
      std::ranges::swap_ranges(s.seq.begin() + start1,
                               s.seq.begin() + start1 + length,
                               s.seq.begin() + start2,
                               s.seq.begin() + start2 + length);
#else
      std::swap_ranges(s.seq.begin() + start1, s.seq.begin() + start1 + length, s.seq.begin() + start2);
#endif
   }

   inline std::ostream& operator<<(std::ostream& os, const sequence& obj) {
      os << " Alphabet: [ ";
      for (auto c: obj.alphabet)
         os << c << " ";
      os << "] "
         << " size: " << obj.alphabet_size << std::endl;
      for (auto c: obj.seq)
         os << c;

      return os;
   }

   inline std::istream& operator>>(std::istream& is, sequence& obj) {
      std::string line;
      obj.clear();

      std::getline(is, line);

      try {
         obj.seq.reserve(line.size());

         for (auto c: line)
            obj.seq.push_back(c);
      } catch (std::bad_alloc& ba) {
         throw SequenceBadAlloc();
      } catch (...) {
         throw SequenceError();
      }

      return is;
   }

   inline bool operator==(const sequence& lhs, const sequence& rhs) {
      return lhs.seq == rhs.seq;
   }
   inline bool operator==(const sequence& lhs, const std::string& rhs) {
      return lhs == sequence(rhs);
   }

   inline bool operator!=(const sequence& lhs, const sequence& rhs) {
      return !operator==(lhs, rhs);
   }
   inline bool operator!=(const sequence& lhs, const std::string& rhs) {
      return lhs.seq != sequence(rhs).seq;
   }

   inline bool operator<(const sequence& lhs, const sequence& rhs) {
      return lhs.seq < rhs.seq;
   }
   inline bool operator<(const sequence& lhs, const std::string& rhs) {
      return lhs < sequence(rhs);
   }

   inline bool operator<=(const sequence& lhs, const sequence& rhs) {
      return !operator>(lhs, rhs);
   }
   inline bool operator<=(const sequence& lhs, const std::string& rhs) {
      return !operator>(lhs, sequence(rhs));
   }

   inline bool operator>(const sequence& lhs, const sequence& rhs) {
      return operator<(rhs, lhs);
   }
   inline bool operator>(const sequence& lhs, const std::string& rhs) {
      return operator<(sequence(rhs), lhs);
   }

   inline bool operator>=(const sequence& lhs, const sequence& rhs) {
      return !operator<(lhs, rhs);
   }
   inline bool operator>=(const sequence& lhs, const std::string& rhs) {
      return !operator<(lhs, sequence(rhs));
   }

}  // namespace lz
