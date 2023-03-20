
# See https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes

add_library(jana_python_module SHARED jana_module.cc)
target_compile_options(jana_python_module PRIVATE -fvisibility=hidden)
target_include_directories(jana_python_module PRIVATE ${PYTHON_INCLUDE_DIRS} ${pybind11_INCLUDE_DIRS} ../../common)
target_include_directories(jana_python_module PRIVATE ${CMAKE_BINARY_DIR}/src/libraries)
target_link_libraries(jana_python_module PRIVATE ${PYTHON_LIBRARIES} jana2)

set_target_properties(jana_python_module PROPERTIES PREFIX "" SUFFIX ".so" LIBRARY_OUTPUT_NAME "jana")
install(TARGETS jana_python_module DESTINATION python)
