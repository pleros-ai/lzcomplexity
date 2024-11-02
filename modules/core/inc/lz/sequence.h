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

      constexpr lz_uint getAlphabetSize(void) const { return alphabet_size; };
      lz_uint           setAlphabetSize(void);
      void              setAlphabetSize(lz_uint);

      std::vector<char> getAlphabet(void) const { return alphabet; };
      std::vector<char> setAlphabet(void);
      void              setAlphabet(std::vector<char>);

      lz_uint     NoZeroes(void) const;
      std::string toString(void) const;

      std::map<char, lz_double> charDensity(void) const;

      char& operator[](lz_size);
      const char& operator[](lz_size) const;

      char&       first(void);
      const char& first(void) const;

      char&       last(void);
      const char& last(void) const;

      char&       at(lz_size);
      const char& at(lz_size index) const;

      char Min(void) const;
      char Min(lz_size start, lz_size final) const;
      char Max(void) const;
      char Max(lz_size start, lz_size final) const;

      lz_size push(char);
      lz_size pop(void);
      char    back(void) const;

      lz_size size(void) const;
      lz_size length(void) const;

      std::vector<char> SequenceVector(void) const;

      sequence                      Take(lz_size l) const;
      sequence                      Drop(lz_size l) const;
      std::pair<sequence, sequence> Split(lz_size l) const;
      sequence                      Granularity(lz_uint gr) const;

      sequence& pi(void);
      // sequence& negate(void);
      sequence&      reverse(void);
      sequence       reverseCopy(void);
      const sequence reverseCopy(void) const;
      sequence&      rightShift(lz_uint ls = 1);
      sequence&      leftShift(lz_uint ls = 1);

      sequence& operator=(sequence&);
      sequence& operator=(const sequence&);
      sequence& operator=(sequence&&);
      sequence& operator=(const std::vector<char>&);

      sequence& operator+=(const std::vector<char>& s);
      sequence& operator+=(const sequence& s);

      sequence& operator^=(const sequence& s);

      std::vector<char> DetermineAlphabet(void);

      void clear();

      friend std::ostream& operator<<(std::ostream&, const sequence&);
      friend std::istream& operator>>(std::istream&, sequence&);

      friend void swap(sequence&, sequence&);
      friend void swap(sequence&, lz_size start1, lz_size start2, lz_size length);

      friend sequence operator+(const sequence&, const sequence&);
      friend bool     operator==(const sequence&, const sequence&);
      friend bool     operator==(const sequence&, const std::string&);
      friend bool     operator!=(const sequence&, const sequence&);
      friend bool     operator!=(const sequence&, const std::string&);
      friend bool     operator>(const sequence&, const sequence&);
      friend bool     operator>(const sequence&, const std::string&);
      friend bool     operator>=(const sequence&, const sequence&);
      friend bool     operator>=(const sequence&, const std::string&);
      friend bool     operator<(const sequence&, const sequence&);
      friend bool     operator<(const sequence&, const std::string&);
      friend bool     operator<=(const sequence&, const sequence&);
      friend bool     operator<=(const sequence&, const std::string&);

      sequence        map(std::function<char(char)> fn);
      const sequence  map(std::function<char(char)> fn) const;
      friend sequence map(std::function<char(char)> fn, const sequence& s);
      friend sequence map(const sequence& s, std::function<char(char)> fn);
   };

   void     Shuffle(sequence& s, lz_uint block_size);
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
