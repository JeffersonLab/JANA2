
macro(add_jana_test test_target_name)

    set(options LINK_STATIC)
    cmake_parse_arguments(JANATEST "LINK_STATIC" "" "SOURCES" ${ARGN})

    if (NOT JANATEST_SOURCES)
        file(GLOB JANATEST_SOURCES "*.c*")
    endif()

    # Set up target
    add_executable(${test_target_name} ${JANATEST_SOURCES})

    if (${PROJECT_NAME} STREQUAL "jana2")
        # This is an internal plugin
        set(JANA_NAMESPACE "")
    else()
        # This is an external plugin
        set(JANA_NAMESPACE "JANA::")
    endif()

    if (LINK_STATIC)
        set(PLUGIN_JANA_LIB jana2_static_lib)
    else()
        set(PLUGIN_JANA_LIB jana2_shared_lib)
    endif()

    target_link_libraries(${test_target_name} PRIVATE "${JANA_NAMESPACE}${PLUGIN_JANA_LIB}" VendoredCatch2)

    set_target_properties(${test_target_name} PROPERTIES
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/JANA/plugins")

    install(TARGETS ${test_target_name} RUNTIME DESTINATION bin)

    add_test(NAME ${test_target_name} COMMAND ${CMAKE_INSTALL_PREFIX}/bin/${test_target_name})

endmacro()


