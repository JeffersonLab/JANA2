

if (${USE_ROOT})

    file(GLOB JTEST_HEADERS "*.h*")
    add_jana_plugin(janarate SOURCES JEventProcessorJANARATE.cc PUBLIC_HEADER ${JTEST_HEADERS})

    target_include_directories(janarate PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janarate jana2 ${ROOT_LIBRARIES})

else()
    message(STATUS "Skipping plugins/janarate because USE_ROOT=Off")

endif()

