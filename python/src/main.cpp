#include <nanobind/nanobind.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)
#define VERSION_INFO 0.9.5

int add(int i, int j) {
   return i + j;
}

namespace py = nanobind;

void PyUtils(py::module_&);
void PySequence(py::module_&);
void PySaStructure(py::module_&);
void PyStructures(py::module_&);
void PyCaps(py::module_&);
// void PySais(py::module_&);
void PyLempelZiv(py::module_&);
void PySpectral(py::module_&);

NB_MODULE(lzcomplexity, m) {
   m.doc() = R"pbdoc(
    _    ____  ____ __                      LempelZiv  -  description
   | |  |_  /_|__  / /                    -----------------------------
   | |__ / /___|/ / _ \      LempelZiv analysis engine v0.9.5 2025 by Efren Aragon Perez.
   |____/___|  /_/\___/  Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com.

    )pbdoc";

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
   PySpectral(m);

#ifdef VERSION_INFO
   m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
   m.attr("__version__") = "dev";
#endif
}