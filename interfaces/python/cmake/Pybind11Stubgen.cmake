function(pybind11_stubgen target)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "$<CONFIG>/${PY_BUILD_CMAKE_MODULE_NAME}")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${Python3_EXECUTABLE} -m pybind11_stubgen
                ${PY_BUILD_CMAKE_MODULE_NAME}.$<TARGET_FILE_BASE_NAME:${target}>
                --numpy-array-use-type-var
                --exit-code
                --enum-class-locations Sign:LBFGS
                -o ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${target}>/..
        USES_TERMINAL)

endfunction()

function(pybind11_stubgen_install target destination)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PY_BUILD_CMAKE_MODULE_NAME}/$<TARGET_FILE_BASE_NAME:${target}>/
        EXCLUDE_FROM_ALL
        COMPONENT python_stubs
        DESTINATION ${destination}/$<TARGET_FILE_BASE_NAME:${target}>
        FILES_MATCHING REGEX "\.pyi$")

endfunction()
