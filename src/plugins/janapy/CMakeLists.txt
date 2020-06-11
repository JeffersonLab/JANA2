

#find_package(pybind11 REQUIRED)  # or `add_subdirectory(pybind11)`
add_subdirectory(pybind11-2.2.4)

add_library(janapy SHARED janapy.cc)
target_link_libraries(janapy PRIVATE pybind11::embed jana2)
set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS janapy DESTINATION plugins)

#set(Python_ADDITIONAL_VERSIONS 3.7 3.6)
#find_package(PythonInterp 3.6)
#find_package(PythonLibs 3.6)

#if (PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND)
#
#    add_library(janapy SHARED janapy.cc)
#    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")  # see https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes
#    #add_link_options("-undefined dynamic_lookup")
#    target_include_directories(janapy PUBLIC ${PYTHON_INCLUDE_DIRS})
#    target_link_libraries(janapy jana2 ${PYTHON_LIBRARIES})
#    set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so")
#    install(TARGETS janapy DESTINATION plugins)
#
#else()
#
#    message(WARNING "Skipping compilation of janapy.so")
#
#    if (NOT PYTHONINTERP_FOUND)
#        message(WARNING "Unable to find PythonInterp")
#    endif()
#
#    if (NOT PYTHONLIBS_FOUND)
#        message(WARNING "Unable to find PythonLibs")
#    endif()
#
#endif()



