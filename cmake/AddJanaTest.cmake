
macro(add_jana_test test_target_name)

    cmake_parse_arguments(JANATEST "" "" "SOURCES" ${ARGN})

    if (NOT JANATEST_SOURCES)
        file(GLOB JANATEST_SOURCES "*.c*")
    endif()

    # Set up target
    add_executable(${test_target_name} ${JANATEST_SOURCES})

    target_link_libraries(${test_target_name} PRIVATE jana2_static_lib VendoredCatch2)

    set_target_properties(${test_target_name} PROPERTIES
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/JANA/plugins")

    install(TARGETS ${test_target_name} RUNTIME DESTINATION bin/JANA/tests)

    add_test(NAME ${test_target_name} COMMAND ${CMAKE_INSTALL_PREFIX}/bin/JANA/tests/${test_target_name})

endmacro()


