
macro(add_jana_plugin plugin_name)

    # Parse remaining arguments
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCES PUBLIC_HEADER TESTS)

    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Figure out install namespacce
    if (NOT DEFINED INSTALL_NAMESPACE)
        set(INSTALL_NAMESPACE ${PROJECT_NAME} CACHE STRING "Project-specific namespace for installation paths, e.g. /lib/PROJECT_NAMESPACE/plugins")
    endif()

    # Set up target
    add_library(${plugin_name} SHARED ${PLUGIN_SOURCES})

    set_target_properties(${plugin_name} PROPERTIES
        EXPORT_NAME ${plugin_name}
        PREFIX ""
        SUFFIX ".so"
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/${INSTALL_NAMESPACE}/plugins"
    )

    target_link_libraries(${plugin_name} PUBLIC JANA::jana2_static_lib)

    # Handle public headers
    if (PLUGIN_PUBLIC_HEADER)
        set_target_properties(${plugin_name} PROPERTIES 
            PUBLIC_HEADER "${PLUGIN_PUBLIC_HEADER}"
        )
        target_include_directories(${plugin_name}
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                $<INSTALL_INTERFACE:include/${INSTALL_NAMESPACE}/plugins>
        )
    endif()

    # Install target
    install(TARGETS ${plugin_name}
        EXPORT jana2_targets
        PUBLIC_HEADER DESTINATION include/${INSTALL_NAMESPACE}/plugins/${plugin_name}
        LIBRARY DESTINATION lib/${INSTALL_NAMESPACE}/plugins
    )

    # Handle tests
    if (PLUGIN_TESTS)
        add_executable(${plugin_name}_tests ${PLUGIN_TESTS})
        target_link_libraries(${plugin_name}_tests PRIVATE ${plugin_name} VendoredCatch2)
        set_target_properties(${plugin_name}_tests PROPERTIES
            SKIP_BUILD_RPATH FALSE
            BUILD_WITH_INSTALL_RPATH FALSE
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/${INSTALL_NAMESPACE}/plugins"
        )
        install(TARGETS ${plugin_name}_tests RUNTIME DESTINATION bin)
        add_test(NAME ${plugin_name}_tests COMMAND ${CMAKE_INSTALL_PREFIX}/bin/${plugin_name}_tests)
    endif()
endmacro()


