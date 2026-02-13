/**
 * @file PySequence.cpp
 * @brief Python bindings for the sequence class.
 */

#include "inc/PySequence.hpp"

#include <nanobind/operators.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace py = nanobind;

char& first(lz::sequence& seq) { return seq.first(); };
char& last(lz::sequence& seq) { return seq.last(); };
char& at(lz::sequence& seq, lz::lz_size idx) { return seq.at(idx); };

char Min(lz::sequence& seq) { return seq.Min(); };
char Min(lz::sequence& seq, lz::lz_size start, lz::lz_size final) { return seq.Min(start, final); };
char Max(lz::sequence& seq) { return seq.Max(); };
char Max(lz::sequence& seq, lz::lz_size start, lz::lz_size final) { return seq.Max(start, final); };

lz::lz_size push(lz::sequence& seq, char c) { return seq.push(c); };
lz::lz_size pop(lz::sequence& seq) { return seq.pop(); };
char        back(lz::sequence& seq) { return seq.back(); };

lz::lz_size size(lz::sequence& seq) { return seq.size(); };
lz::lz_size length(lz::sequence& seq) { return seq.length(); };

lz::sequence                          Take(lz::sequence& seq, lz::lz_size l) { return seq.Take(l); };
lz::sequence                          Drop(lz::sequence& seq, lz::lz_size l) { return seq.Drop(l); };
std::pair<lz::sequence, lz::sequence> Split(lz::sequence& seq, lz::lz_size l) { return seq.Split(l); };
lz::sequence Granularity(lz::sequence& seq, lz::lz_uint gr) { return seq.Granularity(gr); };

lz::sequence& pi(lz::sequence& seq) { return seq.pi(); };
lz::sequence& reverse(lz::sequence& seq) { return seq.reverse(); };
lz::sequence& rightShift(lz::sequence& seq, lz::lz_uint ls = 1) { return seq.rightShift(ls); };
lz::sequence& leftShift(lz::sequence& seq, lz::lz_uint ls = 1) { return seq.leftShift(ls); };

void         clear(lz::sequence& seq) { seq.clear(); };
lz::sequence map(lz::sequence& seq, std::function<char(char)> f) { return seq.map(f); };

std::vector<char> SequenceVector(lz::sequence& seq) { return seq.SequenceVector(); };
std::vector<char> DetermineAlphabet(lz::sequence seq) { return seq.DetermineAlphabet(); };

std::string toString(const lz::sequence& seq) { return seq.toString(); }

// void Shuffle1(lz::sequence& seq, lz::lz_uint block) { lz::Shuffle(seq, block); }
// lz::sequence Shuffle2(lz::sequence& seq, lz::lz_uint block, lz::lz_uint times) {
//    return lz::Shuffle(seq, block, times);
// }

void PySequence(py::module_& m) {
  using namespace py::literals;

  // =========================================================================
  // sequence class - Symbolic sequence container
  // =========================================================================
  py::class_<lz::sequence> sequence(m, "sequence", R"pbdoc(
A container for symbolic sequences with manipulation methods.

The sequence class is the fundamental data structure for representing
symbolic sequences in lzcomplexity. It supports various input types
(strings, character lists) and provides methods for sequence manipulation,
comparison, and analysis.

Parameters
----------
str : str, optional
    Initialize sequence from a string.
alphabet_size : int, optional
    Size of the symbol alphabet. If not provided, it is automatically
    determined from the unique symbols in the sequence.

Attributes
----------
alphabet_size : int
    The size of the alphabet (number of unique symbols).
seq : List[char]
    The sequence as a list of characters.

Examples
--------
>>> import lzcomplexity as lz
>>> # Create from string
>>> seq = lz.sequence("ABRACADABRA")
>>> print(seq.size())  # 11
>>> print(seq.alphabet_size)  # Automatically determined

>>> # Create with explicit alphabet size
>>> binary_seq = lz.sequence("01010101", 2)

>>> # Sequence operations
>>> first_half = seq.Take(5)  # First 5 characters
>>> second_half = seq.Drop(5)  # Characters after position 5
>>> reversed_seq = seq.reverse()

>>> # Concatenation
>>> combined = seq + lz.sequence("XYZ")

Notes
-----
- Sequences are immutable in terms of their internal representation
- Use Take/Drop/Split for creating subsequences
- The alphabet is automatically determined if not specified
)pbdoc");

  // Constructors
  sequence.def(py::init(), "Create an empty sequence.")
    .def(
      py::init<lz::lz_int>(), "alphabet_size"_a, "Create an empty sequence with a specified alphabet size.")
    .def(py::new_(generateSequenceConstructor()),
         "str"_a,
         "Create a sequence from a string. Alphabet size is auto-determined.")
    .def(py::new_(generateSequenceConstructorWithAlphabet()),
         "str"_a,
         "alphabet_size"_a,
         "Create a sequence from a string with explicit alphabet size.");

  // Operators
  sequence.def(py::self + py::self, "Concatenate two sequences and return a new sequence.")
    .def(py::self += py::self, "Append another sequence to this sequence in-place.")
    .def(py::self += std::string(), "Append a string to this sequence in-place.")
    .def(py::self += std::vector<lz::lz_char>(), "Append a character vector to this sequence in-place.")
    .def(py::self == py::self, "Check if two sequences are equal.")
    .def(py::self == std::string(), "Check if sequence equals a string.")
    .def(py::self != py::self, "Check if two sequences are not equal.")
    .def(py::self != std::string(), "Check if sequence does not equal a string.")
    .def(py::self > py::self, "Lexicographic greater-than comparison.")
    .def(py::self > std::string(), "Lexicographic greater-than comparison with string.")
    .def(py::self >= py::self, "Lexicographic greater-than-or-equal comparison.")
    .def(py::self >= std::string(), "Lexicographic greater-than-or-equal comparison with string.")
    .def(py::self < py::self, "Lexicographic less-than comparison.")
    .def(py::self < std::string(), "Lexicographic less-than comparison with string.")
    .def(py::self <= py::self, "Lexicographic less-than-or-equal comparison.")
    .def(py::self <= std::string(), "Lexicographic less-than-or-equal comparison with string.")
    .def("__str__", &::toString, "Convert sequence to string representation.")
    .def(
      "__repr__",
      [](const lz::sequence& self) { return "sequence('" + self.toString() + "')"; },
      "Return string representation for debugging.")
    .def("__len__", &::size, "Return the length of the sequence.")
    .def("__getitem__",
         py::overload_cast<lz::lz_size>(&lz::sequence::operator[]),
         "Get character at specified index.");

  // Element access methods
  sequence
    .def("first",
         &::first,
         R"pbdoc(
Get the first character of the sequence.

Returns
-------
char
    The first character.

Raises
------
IndexError
    If the sequence is empty.
)pbdoc")
    .def("last",
         &::last,
         R"pbdoc(
Get the last character of the sequence.

Returns
-------
char
    The last character.

Raises
------
IndexError
    If the sequence is empty.
)pbdoc")
    .def("back", &::back, "Get the last character (alias for last()).")
    .def("at",
         &::at,
         "idx"_a,
         R"pbdoc(
Get character at specified index with bounds checking.

Parameters
----------
idx : int
    The index of the character to retrieve.

Returns
-------
char
    The character at the specified index.

Raises
------
IndexError
    If index is out of bounds.
)pbdoc");

  // Min/Max methods
  sequence
    .def("Min",
         py::overload_cast<lz::sequence&>(&::Min),
         "Return the minimum (smallest) character in the sequence.")
    .def("Min",
         py::overload_cast<lz::sequence&, lz::lz_size, lz::lz_size>(&::Min),
         "start"_a,
         "final"_a,
         "Return the minimum character in the range [start, final).")
    .def("Max",
         py::overload_cast<lz::sequence&>(&::Max),
         "Return the maximum (largest) character in the sequence.")
    .def("Max",
         py::overload_cast<lz::sequence&, lz::lz_size, lz::lz_size>(&::Max),
         "start"_a,
         "final"_a,
         "Return the maximum character in the range [start, final).");

  // Modification methods
  sequence
    .def("push",
         &::push,
         "c"_a,
         R"pbdoc(
Append a character to the end of the sequence.

Parameters
----------
c : char
    The character to append.

Returns
-------
int
    The new size of the sequence.
)pbdoc")
    .def("pop",
         &::pop,
         R"pbdoc(
Remove and return the last character of the sequence.

Returns
-------
int
    The new size of the sequence.
)pbdoc")
    .def("clear", &::clear, "Remove all characters from the sequence.");

  // Size methods
  sequence.def("size", &::size, "Return the number of characters in the sequence.")
    .def("length", &::length, "Return the length of the sequence (alias for size()).");

  // Subsequence methods
  sequence
    .def("Take",
         &::Take,
         "l"_a,
         R"pbdoc(
Return a new sequence containing the first l characters.

Parameters
----------
l : int
    Number of characters to take from the beginning.

Returns
-------
sequence
    A new sequence with the first l characters.

Examples
--------
>>> seq = lz.sequence("ABRACADABRA")
>>> seq.Take(5)  # Returns sequence("ABRAC")
)pbdoc")
    .def("Drop",
         &::Drop,
         "l"_a,
         R"pbdoc(
Return a new sequence with the first l characters removed.

Parameters
----------
l : int
    Number of characters to drop from the beginning.

Returns
-------
sequence
    A new sequence without the first l characters.

Examples
--------
>>> seq = lz.sequence("ABRACADABRA")
>>> seq.Drop(5)  # Returns sequence("ADABRA")
)pbdoc")
    .def("Split",
         &::Split,
         "l"_a,
         R"pbdoc(
Split the sequence at position l.

Parameters
----------
l : int
    Position at which to split the sequence.

Returns
-------
Tuple[sequence, sequence]
    A tuple containing (first_part, second_part).

Examples
--------
>>> seq = lz.sequence("ABRACADABRA")
>>> first, second = seq.Split(5)
>>> # first = "ABRAC", second = "ADABRA"
)pbdoc")
    .def("Granularity",
         &::Granularity,
         "gr"_a,
         R"pbdoc(
Reduce sequence granularity by grouping characters.

Parameters
----------
gr : int
    Granularity factor (number of characters per group).

Returns
-------
sequence
    A new sequence with reduced granularity.
)pbdoc");

  // Transformation methods
  sequence.def("pi", &::pi, "Return the largest proper prefix that is also a suffix.")
    .def("reverse",
         &::reverse,
         R"pbdoc(
Reverse the sequence in-place.

Returns
-------
sequence
    Reference to this sequence (reversed).
)pbdoc")
    .def("rightShift",
         &::rightShift,
         "ls"_a = 1u,
         R"pbdoc(
Circular right shift of the sequence.

Parameters
----------
ls : int, optional
    Number of positions to shift (default=1).

Returns
-------
sequence
    Reference to this sequence (shifted).
)pbdoc")
    .def("leftShift",
         &::leftShift,
         "ls"_a = 1u,
         R"pbdoc(
Circular left shift of the sequence.

Parameters
----------
ls : int, optional
    Number of positions to shift (default=1).

Returns
-------
sequence
    Reference to this sequence (shifted).
)pbdoc")
    .def("map",
         &::map,
         "f"_a,
         R"pbdoc(
Apply a function to each character and return a new sequence.

Parameters
----------
f : Callable[[char], char]
    Function to apply to each character.

Returns
-------
sequence
    A new sequence with the function applied to each character.
)pbdoc")
    .def("DetermineAlphabet",
         &::DetermineAlphabet,
         "Return a list of unique characters in the sequence (the alphabet).");

  // Copy support
  sequence.def("__copy__", [](const lz::sequence& self) { return lz::sequence(self); })
    .def("__deepcopy__", [](const lz::sequence& self, py::dict) { return lz::sequence(self); }, "memo"_a);

  // Properties
  sequence
    .def_prop_ro("alphabet_size",
                 &lz::sequence::getAlphabetSize,
                 "int: The size of the alphabet (number of unique symbols).")
    .def_prop_ro("seq", &::SequenceVector, "List[char]: The sequence as a list of characters.");

  // =========================================================================
  // Shuffle function
  // =========================================================================
  m.def("Shuffle",
        py::overload_cast<lz::sequence&, lz::lz_uint>(&lz::Shuffle),
        "seq"_a,
        "block_size"_a,
        R"pbdoc(
Randomly shuffle blocks of a sequence in-place.

Parameters
----------
seq : sequence
    The sequence to shuffle (modified in-place).
block_size : int
    Size of blocks to shuffle. Larger blocks preserve more local structure.

Notes
-----
This function modifies the sequence in-place. Use the three-argument
version to create shuffled copies without modifying the original.
)pbdoc");

  m.def("Shuffle",
        py::overload_cast<const lz::sequence&, lz::lz_uint, lz::lz_uint>(&lz::Shuffle),
        "seq"_a,
        "block_size"_a,
        "times"_a,
        R"pbdoc(
Create multiple shuffled copies of a sequence.

Parameters
----------
seq : sequence
    The original sequence (not modified).
block_size : int
    Size of blocks to shuffle.
times : int
    Number of shuffle iterations to perform.

Returns
-------
sequence
    A new shuffled sequence.
)pbdoc");
}