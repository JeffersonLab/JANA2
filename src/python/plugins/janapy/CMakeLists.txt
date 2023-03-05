

include_directories(${PYTHON_INCLUDE_DIRS} ${pybind11_INCLUDE_DIRS} ../../common)
link_libraries(${PYTHON_LIBRARIES} pybind11::embed jana2)

add_library(janapy_plugin SHARED janapy_plugin.cc)

set_target_properties(janapy_plugin PROPERTIES PREFIX "" SUFFIX ".so" LIBRARY_OUTPUT_NAME "janapy")
install(TARGETS janapy_plugin DESTINATION plugins)





