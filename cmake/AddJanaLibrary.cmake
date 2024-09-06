
macro(add_jana_library library_name)

    # Parse remaining arguments
    set(options)
    set(oneValueArgs EXPORT)
    set(multiValueArgs SOURCES PUBLIC_HEADER TESTS)

    cmake_parse_arguments(LIBRARY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT LIBRARY_SOURCES AND NOT LIBRARY_PUBLIC_HEADER AND NOT LIBRARY_TESTS)
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

        set(LIBRARY_SOURCES ${SOURCES_IN_CWD} ${SOURCES_IN_SUBDIR})
        set(LIBRARY_PUBLIC_HEADER ${HEADERS_IN_CWD} ${HEADERS_IN_SUBDIR})
        set(LIBRARY_TESTS ${TESTS_IN_CWD} ${TESTS_IN_SUBDIR})
        message(STATUS "Plugin ${library_name}: found sources: ${LIBRARY_SOURCES}")
        message(STATUS "Plugin ${library_name}: found headers: ${LIBRARY_PUBLIC_HEADER}")
        message(STATUS "Plugin ${library_name}: found tests: ${LIBRARY_TESTS}")
    endif()

    if (${PROJECT_NAME} STREQUAL "jana2")
        # This is an internal plugin
        set(INSTALL_NAMESPACE "JANA")
        set(JANA_NAMESPACE "")
        if (NOT LIBRARY_EXPORT)
            set(LIBRARY_EXPORT "jana2_targets")
        endif()
    else()
        # This is an external plugin
        # Figure out install namespace, which _might_ be different than PROJECT_NAME
        if (NOT DEFINED INSTALL_NAMESPACE)
            set(INSTALL_NAMESPACE ${PROJECT_NAME} CACHE STRING "Project-specific namespace for installation paths, e.g. /lib/PROJECT_NAMESPACE/plugins")
        endif()
        set(JANA_NAMESPACE "JANA::")
    endif()

    # Set up target
    add_library(${library_name} SHARED ${LIBRARY_SOURCES})

    set_target_properties(${library_name} PROPERTIES
        EXPORT_NAME ${library_name}
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/${INSTALL_NAMESPACE}/plugins"
    )

    target_link_libraries(${library_name} PUBLIC "${JANA_NAMESPACE}jana2_static_lib")

    # Handle public headers
    if (LIBRARY_PUBLIC_HEADER)
        set_target_properties(${library_name} PROPERTIES 
            PUBLIC_HEADER "${LIBRARY_PUBLIC_HEADER}"
        )
        target_include_directories(${library_name}
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include/${INSTALL_NAMESPACE}/plugins/${library_name}>
        )
    endif()

    # Install target
    install(TARGETS ${library_name}
        EXPORT ${LIBRARY_EXPORT}
        PUBLIC_HEADER DESTINATION include/${INSTALL_NAMESPACE}/plugins/${library_name}
        LIBRARY DESTINATION lib/${INSTALL_NAMESPACE}/plugins
    )

    # Handle tests
    if (LIBRARY_TESTS)
        add_executable(${library_name}_tests ${LIBRARY_TESTS})
        target_link_libraries(${library_name}_tests PRIVATE ${library_name} "${JANA_NAMESPACE}VendoredCatch2")
        set_target_properties(${library_name}_tests PROPERTIES
            SKIP_BUILD_RPATH FALSE
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/${INSTALL_NAMESPACE}/plugins"
        )
        #install(TARGETS ${library_name}_tests RUNTIME DESTINATION bin)
        add_test(NAME ${library_name}_tests COMMAND ${library_name}_tests)
    endif()
endmacro()


