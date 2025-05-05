

if (${USE_ROOT})

    add_jana_plugin(janaroot)

else()
    message(STATUS "Skipping plugins/janaroot because USE_ROOT=Off")

endif()

