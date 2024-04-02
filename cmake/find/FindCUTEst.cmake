set(CUTEST_MYARCH "pc64.lnx.gfo" CACHE STRING "CUTEst architecture")

# CUTEst itself

find_path(CUTEST_INCLUDE_DIR cutest.h
    HINTS
        ${CUTEST}
        $ENV{CUTEST}
)
if (CUTEST_INCLUDE_DIR)
    cmake_path(GET CUTEST_INCLUDE_DIR PARENT_PATH CUTEST_DIR)
    set(CUTEST_DIR ${CUTEST_DIR} CACHE PATH "")
endif()

option(CUTEST_SHARED "Link to libcutest.so instead of libcutest.a. \
Results in smaller PROBLEM.so file, but makes it so only one CUTEst \
problem can be loaded at once." Off)
if (CUTEST_SHARED)
    set(CUTEST_LIBRARY_NAMES libcutest.so cutest)
else()
    set(CUTEST_LIBRARY_NAMES libcutest.a)
endif()

find_library(CUTEST_LIBRARY
    NAMES
        ${CUTEST_LIBRARY_NAMES}
    HINTS
        ${CUTEST_DIR}
    PATH_SUFFIXES
        objects/${CUTEST_MYARCH}/double
)

mark_as_advanced(CUTEST_INCLUDE_DIR CUTEST_DIR CUTEST_LIBRARY)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUTEst
    REQUIRED_VARS
        CUTEST_LIBRARY
        CUTEST_INCLUDE_DIR
)

# CUTEst library target

if (CUTEst_FOUND AND NOT TARGET CUTEst::headers)
    add_library(CUTEst::headers INTERFACE IMPORTED)
    target_include_directories(CUTEst::headers
        INTERFACE ${CUTEST_INCLUDE_DIR})
endif()

if (CUTEst_FOUND AND NOT TARGET CUTEst::cutest)
    add_library(CUTEst::cutest UNKNOWN IMPORTED)
    set_target_properties(CUTEst::cutest PROPERTIES
        IMPORTED_LOCATION ${CUTEST_LIBRARY})
    if (CUTEST_LIBRARY MATCHES "\\.so$")
        set_target_properties(CUTEst::cutest PROPERTIES
            IMPORTED_SONAME "libcutest.so")
    endif()
    target_link_libraries(CUTEst::cutest INTERFACE CUTEst::headers)
endif()
