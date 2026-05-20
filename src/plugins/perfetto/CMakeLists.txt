if (USE_PERFETTO)
    add_jana_plugin(perfetto SOURCES perfetto_plugin.cc)
    add_test(NAME jana-plugin-perfetto-tests
        COMMAND jana -Pplugins=JTest,perfetto -Pjana:nevents=10)
else()
    message(STATUS "Skipping plugins/perfetto because USE_PERFETTO=Off")
endif()
