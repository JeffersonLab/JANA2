

if (${USE_ROOT})

    add_jana_plugin(janarate)

    target_include_directories(janarate PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janarate PUBLIC ${ROOT_LIBRARIES})

else()
    message(STATUS "Skipping plugins/janarate because USE_ROOT=Off")

endif()

