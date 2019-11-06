

add_library(janapy SHARED janapy.cc)

set(Python_ADDITIONAL_VERSIONS 3.7 3.6)

find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)

include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(janapy ${PYTHON_LIBRARIES})
target_link_libraries(janapy jana2)
set_target_properties(janapy PROPERTIES PREFIX "")

