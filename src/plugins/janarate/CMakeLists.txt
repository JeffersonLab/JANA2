

if (${USE_ROOT})

    add_jana_plugin(janarate)

else()
    message(STATUS "Skipping plugins/janarate because USE_ROOT=Off")

endif()

