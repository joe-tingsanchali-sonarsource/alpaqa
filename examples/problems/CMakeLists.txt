if (TARGET alpaqa::dl-api)
    alpaqa_add_dl_problem_module("sparse-logistic-regression" LINK_ALPAQA)
endif()

option(ALPAQA_WITH_PYTHON_PROBLEM_LOADER
    "Build the dynamic loader for problems defined in Python" Off)
if (ALPAQA_WITH_PYTHON_PROBLEM_LOADER)
    # Find the Pybind11 headers
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed)
    include(../../interfaces/python/cmake/QueryPythonForPybind11.cmake)
    find_pybind11_python_first()
    alpaqa_add_dl_problem_module("python-loader" LINK_ALPAQA VISIBILITY_DEFAULT)
    target_link_libraries(python-loader PRIVATE pybind11::embed)
endif()
