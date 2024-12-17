import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake, cmake_layout
from conan.tools.build import can_run


class AlpaqaRecipe(ConanFile):
    name = "alpaqa"
    version = "1.0.0-alpha.20"

    # Optional metadata
    license = "LGPLv3"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/kul-optec/alpaqa"
    description = (
        "Augmented Lagrangian and PANOC solvers for nonconvex numerical optimization"
    )
    topics = ("optimization", "panoc", "alm", "mpc")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    bool_alpaqa_options = {
        "with_python": False,
        "with_matlab": False,
        "with_drivers": True,
        "with_gradient_checker": False,
        "with_casadi": True,
        "with_external_casadi": False,
        "with_cutest": False,
        "with_qpalm": False,
        "with_json": True,
        "with_lbfgsb": False,
        "with_ocp": False,
        "with_casadi_ocp": False,
        "with_openmp": False,
        "with_quad_precision": False,
        "with_single_precision": False,
        "with_long_double": False,
        "debug_checks_eigen": False,
        "dont_parallelize_eigen": True,
        "no_dlclose": False,
        "with_blas": False,
    }
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    } | {k: [True, False] for k in bool_alpaqa_options}
    default_options = {
        "shared": False,
        "fPIC": True,
    } | bool_alpaqa_options

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "cmake/*",
        "interfaces/*",
        "python/*",
        "test/*",
        "LICENSE",
        "README.md",
    )

    def requirements(self):
        self.requires("eigen/3.4.0", transitive_headers=True)
        self.test_requires("gtest/1.11.0")
        if self.options.with_external_casadi:
            self.requires("casadi/3.6.5@alpaqa", transitive_headers=True)
        if self.options.with_json:
            self.requires("nlohmann_json/3.11.2", transitive_headers=True)
        if self.options.with_python:
            self.requires("pybind11/2.13.6")
        if self.options.with_matlab:
            self.requires("utfcpp/4.0.4")
        if self.options.with_blas:
            self.requires("openblas/0.3.24")

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def validate(self):
        if self.options.with_matlab and not self.options.with_json:
            msg = "MATLAB MEX interface requires JSON. Set 'with_json=True'."
            raise ConanInvalidConfiguration(msg)
        if self.options.with_matlab and not self.options.with_external_casadi:
            msg = "MATLAB MEX interface requires CasADi. Set 'with_external_casadi=True'."
            raise ConanInvalidConfiguration(msg)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.set_property("utfcpp", "cmake_target_name", "utf8cpp::utf8cpp")
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["ALPAQA_WITH_EXAMPLES"] = False
        for k in self.bool_alpaqa_options:
            value = getattr(self.options, k, None)
            if value is not None and value.value is not None:
                tc.variables["ALPAQA_" + k.upper()] = bool(value)
        if self.options.with_python:
            tc.variables["USE_GLOBAL_PYBIND11"] = True
        if can_run(self):
            tc.variables["ALPAQA_FORCE_TEST_DISCOVERY"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "alpaqa"))
