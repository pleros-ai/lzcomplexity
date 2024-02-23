/***************************************************************************
								  sequence.h  -  description
									  -------------------
	 begin                : 16 Nov 2013
	 last modified        : 20 Feb 2024
	 email                : estevez@fisica.uh.cu
 ***************************************************************************/

 /***************************************************************************
  *   Copyright (C) 2013-2018 by Ernesto Estevez Rams   						    *
  *   estevez@fisica.uh.cu   															    *
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

#include <utility>
#include <set>
#include <tbb/concurrent_set.h>

#include <lz/myexceptions.h>
#include <lz/utils.h>

namespace lz {

	constexpr int ALPHABET_SIZE = 0x02;

	// The error condition exceptions
	class SequenceError : Errors {};
	class SequenceBadAlloc : public SequenceError {};
	class SequenceNoMatchSize : public SequenceError {};
	class SequenceOutOfBounds : public SequenceError {};

	//.....................................................
	// The structure that stores the actual sequence data
	//.....................................................
	/// \brief structure that stores the actual sequence data
	class sequence {
	protected:
		std::vector<char> seq; //!< the sequence
		int alphabet_size;

	public:
		sequence(void) :alphabet_size(ALPHABET_SIZE) {};
		sequence(int alphsize) :alphabet_size(alphsize) {};
		sequence(const std::vector<char> vec);
		sequence(const std::string str);
		sequence(const std::vector<char> vec, int aph) : seq(vec), alphabet_size(aph) {};
		sequence(const std::string str, int aph) : seq(str.begin(), str.end()), alphabet_size(aph) {};
		sequence(const sequence& s) : seq(s.seq), alphabet_size(s.alphabet_size) {};
		sequence(sequence&& s) : seq(std::move(s.seq)), alphabet_size(s.alphabet_size) {};
		~sequence() { alphabet_size = ALPHABET_SIZE; seq.clear(); };

		constexpr int alphabetSize(void) const { return alphabet_size; };
		int SetAlphabetSize(void);

		constexpr unsigned int NoZeroes(void) const;
		constexpr std::string toString(void) const;

		char& operator[](std::vector<char>::size_type);
		const char& operator[](std::vector<char>::size_type) const;

		char& first(void);
		const char& first(void)const;

		char& last(void);
		const char& last(void)const;

		char& at(std::vector<char>::size_type);
		char const_at(std::vector<char>::size_type index) const;

		char Min(void)const;
		char Min(std::vector<char>::size_type start, std::vector<char>::size_type final) const;
		char Max(void)const;
		char Max(std::vector<char>::size_type start, std::vector<char>::size_type final) const;

		std::vector<char>::size_type  push(char);
		std::vector<char>::size_type  pop(void);
		char back(void)const;

		std::vector<char>::size_type size(void) const;

		std::vector<char> SequenceVector(void) const;

		sequence Take(std::vector<char>::size_type l) const;
		sequence Drop(std::vector<char>::size_type l) const;
		std::pair<sequence, sequence> Split(std::vector<char>::size_type l) const;
		sequence Granularity(unsigned int gr) const;

		sequence& pi(void);
		sequence& reverse(void);
		sequence& rightShift(unsigned int ls = 1);
		sequence& leftShift(unsigned int ls = 1);

		sequence& operator=(sequence);
		sequence& operator=(const std::vector<char>&);
		sequence& operator+=(const std::vector<char>& s);
		sequence& operator+=(const sequence& s);

		sequence& operator^=(const sequence& s);

		std::vector<char> DetermineAlphabet(void);

		void clear();

		friend std::ostream& operator<<(std::ostream&, const sequence&);
		friend std::istream& operator>>(std::istream&, sequence&);

		friend void swap(sequence&, sequence&);
		friend void swap(sequence&, std::vector<char>::size_type start1, std::vector<char>::size_type start2, std::vector<char>::size_type length);

		friend sequence operator+(const sequence&, const sequence&);
		friend sequence& operator+(sequence&, const sequence&);
		friend bool operator==(const sequence&, const sequence&);
		friend bool operator!=(const sequence&, const sequence&);
		friend bool operator>(const sequence&, const sequence&);
		friend bool operator>=(const sequence&, const sequence&);
		friend bool operator<(const sequence&, const sequence&);
		friend bool operator<=(const sequence&, const sequence&);

		sequence map(std::function<char(char)> fn);
		friend sequence map(std::function<char(char)> fn, const sequence& s);
		friend sequence map(const sequence& s, std::function<char(char)> fn);

	};

	void Shuffle(sequence& s, unsigned int block_size);
	sequence Shuffle(sequence& s, unsigned int block_size, unsigned int times);


	//.............................................................................................................

	inline sequence::sequence(const std::vector<char> vec) {
		seq = vec;
		tbb::concurrent_set<char> aph;

		auto fun = [&](auto i) { aph.insert(vec[i]); };
		utils::parallel_for(0, vec.size(), fun);

		alphabet_size = aph.size();
	}

	inline sequence::sequence(const std::string str) {
		seq = std::vector<char>(str.begin(), str.end());
		tbb::concurrent_set<char> aph;

		auto fun = [&](auto i) { aph.insert(str[i]); };
		utils::parallel_for(0, str.size(), fun);

		alphabet_size = aph.size();
	}


	inline constexpr unsigned int sequence::NoZeroes(void) const {
		unsigned int acum = 0;

		for (auto s : seq)
			acum += (s == 0) ? 1 : 0;

		return acum;
	}

	inline constexpr std::string sequence::toString(void) const {
		return std::string{ seq.begin(), seq.end() };
	}

	inline sequence& sequence::operator=(sequence s) {
		swap(*this, s);

		return *this;
	}

	inline sequence& sequence::operator=(const std::vector<char>& s) {
		try {
			seq = s;
		}
		catch (std::bad_alloc& ba) {
			throw SequenceBadAlloc();
		}
		catch (...) {
			throw SequenceError();
		}

		return *this;
	}

	inline sequence operator+(const sequence& lhs, const sequence& rhs) {
		sequence result(std::move(lhs));
		result += rhs.SequenceVector();

		return result;
	}

	inline sequence& operator+(sequence& s1, const sequence& s2) {
		try {
			s1 += s2.SequenceVector();
			return s1;
		}
		catch (std::bad_alloc& ba) {
			throw SequenceBadAlloc();
		}
		catch (...) {
			throw SequenceError();
		}
	}

	inline sequence& sequence::operator+=(const std::vector<char>& s) {
		try {
			seq.insert(seq.end(), s.begin(), s.end());
		}
		catch (std::bad_alloc& ba) {
			throw SequenceBadAlloc();
		}
		catch (...) {
			throw SequenceError();
		}

		return *this;
	}

	inline sequence& sequence::operator^=(const sequence& s) {
		std::vector<char>::const_iterator iterseq = s.seq.begin();

		if (s.seq.size() != seq.size())
			throw SequenceNoMatchSize();

		for (auto c : seq) {
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
		}
		catch (std::bad_alloc& ba) {
			throw SequenceBadAlloc();
		}
		catch (...) {
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

		return al;
	}

	inline int sequence::SetAlphabetSize(void) {
		std::vector<char> al;

		al = DetermineAlphabet();

		return alphabet_size = al.size();
	}

	inline std::vector<char> sequence::SequenceVector(void)const {
		return seq;
	}

	inline char& sequence::operator[](std::vector<char>::size_type index) {
		if (index >= seq.size()) {
			throw SequenceOutOfBounds();
		}

		return seq[index];
	}

	inline const char& sequence::operator[](std::vector<char>::size_type index) const {
		if (index >= seq.size()) {
			throw SequenceOutOfBounds();
		}
		return seq[index];
	}

	inline char& sequence::at(std::vector<char>::size_type index) {
		return seq.at(index);
	}

	// .............................................................................
	// Name: const_at
	//
	// Synopsis: indexing operator. No bounds check
	//
	// Parameters:
	//			 std::vector<bool>::size_type index      -------> the index of the requested value
	//
	// Returns:
	//         bool ------> the index value of the sequence
	//
	// Exceptions:
	//            None
	//..............................................................................
	inline char sequence::const_at(std::vector<char>::size_type index)const {
		return seq.at(index);
	}

	inline char& sequence::first(void) {
		return seq.front();
	}

	inline const char& sequence::first(void)const {
		return seq.front();
	}

	inline char& sequence::last(void) {
		return seq.back();
	}

	inline const char& sequence::last()const {
		return seq.back();
	}

	inline char sequence::back(void)const {
		return last();
	}

	inline char sequence::Max(void)const {
		return *std::max_element(seq.begin(), seq.end());
	}


	inline char sequence::Min(std::vector<char>::size_type start, std::vector<char>::size_type final)const {
		return  *std::min_element(seq.begin() + start, seq.begin() + final);
	}

	inline char sequence::Max(std::vector<char>::size_type start, std::vector<char>::size_type final)const {
		return  *std::max_element(seq.begin() + start, seq.begin() + final);
	}


	inline char sequence::Min(void)const {
		return  *std::min_element(seq.begin(), seq.end());
	}

	inline std::vector<char>::size_type sequence::push(char c) {
		try {
			seq.push_back(c);
		}
		catch (std::bad_alloc& ba) {
			throw SequenceBadAlloc();
		}

		return seq.size();
	}


	inline std::vector<char>::size_type sequence::pop(void) {
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

	inline sequence& sequence::reverse(void) {
#ifdef __cpp_lib_ranges
		std::ranges::reverse(seq);
#else
		std::reverse(seq.begin(), seq.end());
#endif

		return *this;
	}

	inline sequence& sequence::rightShift(unsigned int ls) {
#ifdef __cpp_lib_ranges
		std::ranges::rotate(seq.begin(), seq.begin() + (ls % seq.size()), seq.end());
#else
		std::rotate(seq.begin(), seq.begin() + (ls % seq.size()), seq.end());
#endif

		return *this;
	}

	inline sequence& sequence::leftShift(unsigned int ls) {
#ifdef __cpp_lib_ranges
		std::ranges::rotate(seq.begin(), seq.begin() + seq.size() - (ls % seq.size()), seq.end());
#else
		std::rotate(seq.begin(), seq.begin() + seq.size() - (ls % seq.size()), seq.end());
#endif

		return *this;
	}

	inline std::vector<char>::size_type sequence::size(void) const {
		return seq.size();
	}

	inline sequence sequence::Take(std::vector<char>::size_type l) const {
		std::vector<char> temp = seq;

		temp.resize(l);

		return sequence(temp, alphabet_size);
	}

	inline sequence sequence::Drop(std::vector<char>::size_type l) const {
		std::vector<char> temp;

		if (l < seq.size()) {
			std::vector<char>::const_iterator iterseq = seq.cbegin() + l;
			char c = 0;

			while (iterseq != seq.cend()) {
				c = *iterseq++;
				temp.push_back(c);
			}
		}

		return sequence(temp, alphabet_size);
	}

	inline std::pair<sequence, sequence> sequence::Split(std::vector<char>::size_type l) const {
		std::vector<char> lhs{ seq.begin(), seq.begin() + l };
		std::vector<char> rhs{ seq.begin() + l, seq.end() };

		// std::ranges::split(seq, l);

		return std::make_pair(lhs, rhs);
	}

	inline sequence sequence::Granularity(unsigned int gr) const {
		char temp = 0;
		std::vector<char> ns;
		std::set<char> alphabet;
		unsigned int count = 0;

		for (auto c : seq) {
			if (count == gr - 1) {
				ns.push_back(temp);
				alphabet.insert(temp);
				temp = 0;
				count = 0;
			}
			temp += c;
			count++;
		}

		return sequence(ns, alphabet.size());
	}

	// .......................................................
	//                friend functions
	//........................................................
	inline void swap(sequence& first, sequence& second) {
		std::swap(first.alphabet_size, second.alphabet_size);
		std::swap(first.seq, second.seq);
	}

	inline void swap(sequence& s, std::vector<char>::size_type start1, std::vector<char>::size_type start2, std::vector<char>::size_type length) {
#ifdef __cpp_lib_ranges
		std::ranges::swap_ranges(
			s.seq.begin() + start1, s.seq.begin() + start1 + length,
			s.seq.begin() + start2, s.seq.begin() + start2 + length
		);
#else
		std::swap_ranges(s.seq.begin() + start1, s.seq.begin() + start1 + length, s.seq.begin() + start2);
#endif
	}

	inline sequence sequence::map(std::function<char(char)> fn) {
		sequence transformed_sequence;

		transformed_sequence.seq.resize(seq.size());

#ifdef __cpp_lib_ranges
		std::ranges::copy(std::views::transform(seq, fn), std::back_inserter(transformed_sequence.seq));
#else
		std::transform(seq.begin(), seq.end(), transformed_sequence.seq.begin(), fn);
#endif

		transformed_sequence.alphabet_size = alphabet_size;

		return transformed_sequence;
	}

	inline sequence map(std::function<char(char)> fn, const sequence& s) {
		sequence transformed_sequence;

		transformed_sequence.seq.resize(s.seq.size());

#ifdef __cpp_lib_ranges
		std::ranges::copy(std::views::transform(s.seq, fn), std::back_inserter(transformed_sequence.seq));
#else
		std::transform(s.seq.begin(), s.seq.end(), transformed_sequence.seq.begin(), fn);
#endif

		transformed_sequence.alphabet_size = s.alphabet_size;

		return transformed_sequence;
	}


	inline sequence map(const sequence& s, std::function<char(char)> fn) {
		sequence transformed_sequence;

		transformed_sequence.seq.resize(s.seq.size());

#ifdef __cpp_lib_ranges
		std::ranges::copy(std::views::transform(s.seq, fn), std::back_inserter(transformed_sequence.seq));
#else
		std::transform(s.seq.begin(), s.seq.end(), transformed_sequence.seq.begin(), fn);
#endif

		transformed_sequence.alphabet_size = s.alphabet_size;

		return transformed_sequence;
	}

	inline std::ostream& operator<<(std::ostream& os, const sequence& obj) {

		for (auto c : obj.seq)
			os << c;

		return os;
	}

	inline std::istream& operator>>(std::istream& is, sequence& obj) {
		std::string line;
		obj.clear();

		std::getline(is, line);

		try {
			obj.seq.reserve(line.size());

			for (auto c : line)
				obj.seq.push_back(c);
		}
		catch (std::bad_alloc& ba) {
			throw SequenceBadAlloc();
		}
		catch (...) {
			throw SequenceError();
		}

		return is;
	}

	inline bool operator==(const sequence& lhs, const sequence& rhs) {
		return lhs.seq == rhs.seq;
	}

	inline bool operator!=(const sequence& lhs, const sequence& rhs) {
		return !operator==(lhs, rhs);
	}

	inline bool operator<(const sequence& lhs, const sequence& rhs) {
		return lhs.seq < rhs.seq;
	}

	inline bool operator<=(const sequence& lhs, const sequence& rhs) {
		return !operator> (lhs, rhs);
	}

	inline bool operator>(const sequence& lhs, const sequence& rhs) {
		return  operator< (rhs, lhs);
	}

	inline bool operator>=(const sequence& lhs, const sequence& rhs) {
		return !operator< (lhs, rhs);
	}

	inline void Shuffle(sequence& s, unsigned int block_size) {
		static std::random_device rd_seed;
		static std::mt19937 random_engine(rd_seed());

		std::uniform_int_distribution<> dis(0, (s.size() - block_size - 0x01) / block_size);
		unsigned int op1 = s.size() + 0x03, op2 = s.size() + 0x03;

		while (op1 > s.size() - block_size - 0x01) // this goes on until we get a valid index
			op1 = block_size * dis(random_engine); // the index for the first block

		while (true) { // this goes on until we get a valid index
			op2 = block_size * dis(random_engine); // the index for the second block
			if ((op2 < op1 || op2 > op1 + block_size) && op2 < s.size() - block_size - 0x01) // it does not overlap with the previous block and is not to large the block size can not be feed
				break;
		}

		swap(s, op1, op2, block_size);
	}

	inline sequence Shuffle(const sequence& s, unsigned int block_size, unsigned int times) {
		sequence seq(s);

		for (unsigned int i = 0; i < times; i++)
			Shuffle(seq, block_size);

		return seq;
	}

}