

set(Python_ADDITIONAL_VERSIONS 3.7 3.6)
find_package(PythonInterp)
find_package(PythonLibs)

if(PythonInterp_FOUND AND PythonLibs_FOUND)

    add_library(janapy SHARED janapy.cc)
    target_include_directories(janapy PUBLIC ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(janapy jana2 ${PYTHON_LIBRARIES})
    set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so")

else()

    message(WARNING "Unable to find PythonInterp/PythonLibs! Skipping compilation of janapy.so")

endif()

