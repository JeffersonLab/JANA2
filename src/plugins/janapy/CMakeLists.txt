

set(Python_ADDITIONAL_VERSIONS 3.7 3.6)
find_package(PythonInterp 3.6)
find_package(PythonLibs 3.6)

if (PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND)

    add_library(janapy SHARED janapy.cc)
    target_include_directories(janapy PUBLIC ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(janapy jana2 ${PYTHON_LIBRARIES})
    set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS janapy DESTINATION plugins)

else()

    message(WARNING "Skipping compilation of janapy.so")

    if (NOT PYTHONINTERP_FOUND)
        message(WARNING "Unable to find PythonInterp")
    endif()

    if (NOT PYTHONLIBS_FOUND)
        message(WARNING "Unable to find PythonLibs")
    endif()

endif()



