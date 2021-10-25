

if(USE_ROOT)
    find_package(ROOT REQUIRED)

    add_library(janaview SHARED
            JEventProcessor_janaview.cc
            jv_mainframe.cc
        )

    find_package(Threads REQUIRED)
    target_include_directories(janaview PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janaview jana2)
    target_link_libraries(janaview Threads::Threads)
    target_link_libraries(janaview ${ROOT_LIBRARIES})
    set_target_properties(janaview PROPERTIES PREFIX "" OUTPUT_NAME "janaview" SUFFIX ".so")
    install(TARGETS janaview DESTINATION plugins)

else()
message(STATUS "Skipping plugins/janaview because USE_ROOT=Off")

endif() # ROOT_FOUND
