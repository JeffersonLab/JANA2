
# See https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes

add_library(janapy_plugin SHARED janapy_plugin.cc)
target_compile_options(janapy_plugin PRIVATE "-fvisibility=hidden")
target_include_directories(janapy_plugin PRIVATE ${PYTHON_INCLUDE_DIRS} ${pybind11_INCLUDE_DIRS} ../../common)
target_link_libraries(janapy_plugin PRIVATE ${PYTHON_LIBRARIES} pybind11::embed jana2)

set_target_properties(janapy_plugin PROPERTIES PREFIX "" SUFFIX ".so" LIBRARY_OUTPUT_NAME "janapy")
install(TARGETS janapy_plugin DESTINATION plugins)





