

set(Python_ADDITIONAL_VERSIONS 3.7 3.6)
find_package(PythonInterp)
find_package(PythonLibs)

if (PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND)

    add_library(janapy SHARED janapy.cc)

else()

    add_library(janapy SHARED EXCLUDE_FROM_ALL janapy.cc)
    message(WARNING "Skipping compilation of janapy.so")

    if (NOT PYTHONINTERP_FOUND)
        message(WARNING "Unable to find PythonInterp")
    endif()

    if (NOT PYTHONLIBS_FOUND)
        message(WARNING "Unable to find PythonLibs")
    endif()

endif()

target_include_directories(janapy PUBLIC ${PYTHON_INCLUDE_DIRS})
target_link_libraries(janapy jana2 ${PYTHON_LIBRARIES})
set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS janapy DESTINATION plugins)


