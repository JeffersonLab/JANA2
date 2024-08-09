
# See https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes

add_jana_plugin(janapy SOURCES janapy_plugin.cc)

target_compile_options(janapy PRIVATE "-fvisibility=hidden")
target_include_directories(janapy PRIVATE ${PYTHON_INCLUDE_DIRS} ${pybind11_INCLUDE_DIRS} ../../common)
target_link_libraries(janapy PRIVATE ${PYTHON_LIBRARIES} pybind11::embed)






