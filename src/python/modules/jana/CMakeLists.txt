

add_library(jana_python_module SHARED jana_module.cc)
include_directories(${PYTHON_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIR} ../../common)
target_link_libraries(jana_python_module PRIVATE ${PYTHON_LIBRARIES} jana2)

set_target_properties(jana_python_module PROPERTIES PREFIX "" SUFFIX ".so" LIBRARY_OUTPUT_NAME "jana")
install(TARGETS jana_python_module DESTINATION python)
