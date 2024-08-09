
macro(add_jana_plugin plugin_name)

    # Parse remaining arguments
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCES PUBLIC_HEADER TESTS)

    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT PLUGIN_SOURCES AND NOT PLUGIN_PUBLIC_HEADER AND NOT PLUGIN_TESTS)
        # If no arguments provided, glob everything
        file(GLOB HEADERS_IN_SUBDIR "include/*")
        file(GLOB SOURCES_IN_SUBDIR "src/*")
        file(GLOB TESTS_IN_SUBDIR "test*/*")
        file(GLOB HEADERS_IN_CWD "*.h*")
        set(SOURCES_IN_CWD)
        set(TESTS_IN_CWD)

        file(GLOB ALL_SOURCES_IN_CWD "*.c*")
        foreach(file IN LISTS ALL_SOURCES_IN_CWD)
            string(TOLOWER "${file}" file_lower)
            if(NOT file_lower MATCHES ".*/test[^/]*$|.*test$|.*tests$")
                list(APPEND SOURCES_IN_CWD ${file})
            else()
                list(APPEND TESTS_IN_CWD ${file})
            endif()
        endforeach()

        set(PLUGIN_SOURCES ${SOURCES_IN_CWD} ${SOURCES_IN_SUBDIR})
        set(PLUGIN_PUBLIC_HEADER ${HEADERS_IN_CWD} ${HEADERS_IN_SUBDIR})
        set(PLUGIN_TESTS ${TESTS_IN_CWD} ${TESTS_IN_SUBDIR})
        message(STATUS "Plugin ${plugin_name}: found sources: ${PLUGIN_SOURCES}")
        message(STATUS "Plugin ${plugin_name}: found headers: ${PLUGIN_PUBLIC_HEADER}")
        message(STATUS "Plugin ${plugin_name}: found tests: ${PLUGIN_TESTS}")
    endif()

    # Set up target
    add_library(${plugin_name} SHARED ${PLUGIN_SOURCES})

    set_target_properties(${plugin_name} PROPERTIES
        EXPORT_NAME ${plugin_name}
        PREFIX ""
        SUFFIX ".so"
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/JANA/plugins"
    )

    target_link_libraries(${plugin_name} PUBLIC jana2_static_lib)

    # Handle public headers
    if (PLUGIN_PUBLIC_HEADER)
        set_target_properties(${plugin_name} PROPERTIES 
            PUBLIC_HEADER "${PLUGIN_PUBLIC_HEADER}"
        )
        target_include_directories(${plugin_name}
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/JANA/plugins/${plugin_name}>
        )
    endif()

    # Install target
    install(TARGETS ${plugin_name}
        EXPORT ParentTargets
        PUBLIC_HEADER DESTINATION include/JANA/plugins/${plugin_name}
        LIBRARY DESTINATION lib/JANA/plugins
    )

    # Handle tests
    if (PLUGIN_TESTS)
        add_executable(${plugin_name}_tests ${PLUGIN_TESTS})
        target_link_libraries(${plugin_name}_tests PRIVATE ${plugin_name} VendoredCatch2)
        set_target_properties(${plugin_name}_tests PROPERTIES
            SKIP_BUILD_RPATH FALSE
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/JANA/plugins"
        )
        install(TARGETS ${plugin_name}_tests RUNTIME DESTINATION bin)
        add_test(NAME ${plugin_name}_tests COMMAND ${CMAKE_INSTALL_PREFIX}/bin/jana-${plugin_name}-tests)
    endif()
endmacro()


