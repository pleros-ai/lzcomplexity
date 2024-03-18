#include <pybind11/pybind11.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)
// #define VERSION_INFO 0.7.0

int add(int i, int j) { return i + j; }

namespace py = pybind11;

void PyUtils(py::module&);
void PySequence(py::module&);
void PySaStructure(py::module&);
void PyStructures(py::module&);
void PyCaps(py::module&);
void PySais(py::module&);
void PyLempelZiv(py::module&);

PYBIND11_MODULE(lzcomplexity, m) {
   m.doc() = R"pbdoc(
    _    ____  ____ __                      LempelZiv  -  description
   | |  |_  /_|__  / /                    -----------------------------
   | |__ / /___|/ / _ \      LempelZiv analysis engine v0.7 2024 by Efren Aragon Perez.
   |____/___|  /_/\___/  Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com.

    )pbdoc";

   m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

   // Utils bindings
   PyUtils(m);
   PySaStructure(m);
   PyStructures(m);
   PySequence(m);
   // Algorithms bindings
   PyCaps(m);
   // PySais(m);
   // Lempel-ziv 76 functions
   PyLempelZiv(m);

#ifdef VERSION_INFO
   m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
   m.attr("__version__") = "dev";
#endif
}