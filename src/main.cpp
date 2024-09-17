#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <vector>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) {
  return i + j;
}

namespace py = pybind11;

py::array_t<uint8_t> create_numpy_array() {
  std::vector<size_t> shape = {3, 4, 5};
  size_t total = 1;
  for (auto s : shape)
    total *= s;
  std::vector<uint8_t> data(total, 1);
  return py::array_t<uint8_t>(shape, data.data());
}

PYBIND11_MODULE(cmake_example, m) {
  m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: cmake_example

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

  m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");
  m.def("create_numpy_array", &create_numpy_array,
        "Create a (3,4,5) NumPy array");

  m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}
