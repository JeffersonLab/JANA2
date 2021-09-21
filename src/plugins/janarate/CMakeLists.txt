

if (${USE_ROOT})
    add_library(janarate SHARED JEventProcessorJANARATE.cc)
    target_include_directories(janarate PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janarate jana2 ${ROOT_LIBRARIES})
    set_target_properties(janarate PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS janarate DESTINATION plugins)

else()
    message(STATUS "Skipping plugins/janarate because USE_ROOT=Off")

endif()

