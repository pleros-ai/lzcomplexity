#include <lz/sequence.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py = pybind11;

char& first(lz::sequence& seq) {
   return seq.first();
};
char& last(lz::sequence& seq) {
   return seq.last();
};
char& at(lz::sequence& seq, lz::lz_size idx) {
   return seq.at(idx);
};

char Min(lz::sequence& seq) {
   return seq.Min();
};
char Min(lz::sequence& seq, lz::lz_size start, lz::lz_size final) {
   return seq.Min(start, final);
};
char Max(lz::sequence& seq) {
   return seq.Max();
};
char Max(lz::sequence& seq, lz::lz_size start, lz::lz_size final) {
   return seq.Max(start, final);
};

lz::lz_size push(lz::sequence& seq, char c) {
   return seq.push(c);
};
lz::lz_size pop(lz::sequence& seq) {
   return seq.pop();
};
char back(lz::sequence& seq) {
   return seq.back();
};

lz::lz_size size(lz::sequence& seq) {
   return seq.size();
};
lz::lz_size length(lz::sequence& seq) {
   return seq.length();
};

lz::sequence Take(lz::sequence& seq, lz::lz_size l) {
   return seq.Take(l);
};
lz::sequence Drop(lz::sequence& seq, lz::lz_size l) {
   return seq.Drop(l);
};
std::pair<lz::sequence, lz::sequence> Split(lz::sequence& seq, lz::lz_size l) {
   return seq.Split(l);
};
lz::sequence Granularity(lz::sequence& seq, lz::lz_uint gr) {
   return seq.Granularity(gr);
};

lz::sequence& pi(lz::sequence& seq) {
   return seq.pi();
};
lz::sequence& reverse(lz::sequence& seq) {
   return seq.reverse();
};
lz::sequence& rightShift(lz::sequence& seq, lz::lz_uint ls = 1) {
   return seq.rightShift(ls);
};
lz::sequence& leftShift(lz::sequence& seq, lz::lz_uint ls = 1) {
   return seq.leftShift(ls);
};

void clear(lz::sequence& seq) {
   seq.clear();
};
lz::sequence map(lz::sequence& seq, std::function<char(char)> f) {
   return seq.map(f);
};

std::vector<char> SequenceVector(lz::sequence& seq) {
   return seq.SequenceVector();
};
std::vector<char> DetermineAlphabet(lz::sequence seq) {
   return seq.DetermineAlphabet();
};

std::string toString(const lz::sequence& seq) {
   return seq.toString();
}

// void Shuffle1(lz::sequence& seq, lz::lz_uint block) { lz::Shuffle(seq, block); }
// lz::sequence Shuffle2(lz::sequence& seq, lz::lz_uint block, lz::lz_uint times) {
//    return lz::Shuffle(seq, block, times);
// }

void PySequence(py::module& m) {
   using namespace pybind11::literals;

   py::class_<lz::sequence> sequence(m, "sequence");
   // Define constructors and methods
   sequence.def(py::init())
      .def(py::init<lz::lz_int>(), "alphabet_size"_a)
      .def(py::init<const std::string>(), "str"_a)
      .def(py::init<const std::vector<char>>(), "vec"_a)
      .def(py::init<const std::string, lz::lz_int>(), "str"_a, "aph"_a)
      .def(py::init<const std::vector<char>, lz::lz_int>(), "vec"_a, "aph"_a)
      .def(py::self + py::self)
      .def(py::self == py::self)
      .def(py::self == std::string())
      .def(py::self != py::self)
      .def(py::self != std::string())
      .def(py::self > py::self)
      .def(py::self > std::string())
      .def(py::self >= py::self)
      .def(py::self >= std::string())
      .def(py::self < py::self)
      .def(py::self < std::string())
      .def(py::self <= py::self)
      .def(py::self <= std::string())
      .def(py::self += py::self)
      .def(py::self += std::string())
      .def(py::self += std::vector<lz::lz_char>())
      .def("__str__", &::toString)
      .def("__getitem__", py::overload_cast<lz::lz_size>(&lz::sequence::operator[]))
      .def("first", &::first, "Get first character of the sequence")
      .def("last", &::last, "Get last character of the sequence")
      .def("back", &::back, "Get last character of the sequence")
      .def("at", &::at, "idx"_a, "Get character at index idx")
      .def("Min", py::overload_cast<lz::sequence&>(&::Min), "Min character of the sequence")
      .def("Min",
           py::overload_cast<lz::sequence&, lz::lz_size, lz::lz_size>(&::Min),
           "Min character between a range",
           "start"_a,
           "final"_a)
      .def("Max", py::overload_cast<lz::sequence&>(&::Max), "Max character of the sequence")
      .def("Max",
           py::overload_cast<lz::sequence&, lz::lz_size, lz::lz_size>(&::Max),
           "Max character between a range",
           "start"_a,
           "final"_a)
      .def("push", &::push, "c"_a, "Push a character to the back of the sequence")
      .def("pop", &::pop, "Remove the las character of the sequence")
      .def("size", &::size, "Size of the sequence")
      .def("length", &::length, "Length of the sequence")
      .def("Take", &::Take, "l"_a, "Take the first l characters of the sequence")
      .def("Drop", &::Drop, "l"_a, "Drop the first l characters of the sequence")
      .def("Split", &::Split, "l"_a, "Split the sequence at the first l characters of the sequence")
      .def("Granularity", &::Granularity, "gr"_a)
      .def("pi", &::pi, "Get the largest prefix of the sequence")
      .def("reverse", &::reverse, "Reverse the sequence")
      .def("rightShift", &::rightShift, "ls"_a = 1, "Right shift the sequence by ls characters")
      .def("leftShift", &::leftShift, "ls"_a = 1, "Left shift the sequence by ls characters")
      .def("clear", &::clear, "Clear the sequence")
      .def("map", &::map, "f"_a, "Execute a function over all elements of the sequence")
      .def("DetermineAlphabet", &::DetermineAlphabet)
      .def("__copy__", [](const lz::sequence& self) { return lz::sequence(self); })
      .def(
         "__deepcopy__", [](const lz::sequence& self, py::dict) { return lz::sequence(self); }, "memo"_a);

   sequence
      .def_property_readonly("alphabet_size", &lz::sequence::getAlphabetSize, "Size of the alphabet of the sequence")
      .def_property_readonly("seq", &::SequenceVector, "Vector of characters of the sequence");

   m.def("Shuffle", py::overload_cast<lz::sequence&, lz::lz_uint>(&lz::Shuffle), "seq"_a, "block_size"_a);
   m.def("Shuffle",
         py::overload_cast<const lz::sequence&, lz::lz_uint, lz::lz_uint>(&lz::Shuffle),
         "seq"_a,
         "block_size"_a,
         "times"_a);
}