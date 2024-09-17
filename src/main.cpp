#include "BS_thread_pool.hpp"
#include <future>
#include <iostream>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <thread>
#include <vector>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;
int add(int i, int j) {
  std::thread::id id = std::this_thread::get_id();
  std::cout << "Thread id " << id << std::endl;
  return i + j;
}

std::vector<std::vector<int>> multi_thread(int input) {
  // Constructs a thread pool with as many threads as available in the hardware.
  BS::thread_pool pool;
  std::vector<std::future<int>> results;

  for (int i = 0; i < 4; ++i) {
    results.emplace_back(
        pool.submit_task([input, i]() -> int { return add(input, i); }));
  }

  std::vector<int> output;
  for (auto &&result : results)
    output.emplace_back(result.get());
  std::vector<std::vector<int>> res;
  res.push_back(output);
  res.push_back(output);
  res.push_back(output);
  res.push_back(output);
  return res;
  // return output;
}

py::array_t<uint8_t> create_numpy_array() {
  std::vector<size_t> shape = {3, 4, 5};
  size_t total = 1;
  for (auto s : shape)
    total *= s;
  std::vector<uint8_t> data(total, 1);
  return py::array_t<uint8_t>(shape, data.data());
}

std::vector<py::array_t<uint8_t>> create_numpy_vector() {
  std::vector<size_t> shape = {3, 4, 5};
  size_t total = 1;
  for (auto s : shape)
    total *= s;
  std::vector<uint8_t> data(total, 1);
  auto x = py::array_t<uint8_t>(shape, data.data());
  std::vector<py::array_t<uint8_t>> res;
  res.push_back(x);
  res.push_back(x);
  res.push_back(x);
  res.push_back(x);
  res.push_back(x);
  return res;
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

  m.def("create_numpy_vector", &create_numpy_vector,
        "Create a (3,4,5) NumPy vector");

  m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");
  m.def("multi_thread", &multi_thread,
        "Submit four add function calls to the thread pool");

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}
