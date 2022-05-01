

if(USE_ROOT)
    find_package(ROOT REQUIRED COMPONENTS Gui Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore Thread MultiProc)

    include(${ROOT_USE_FILE})
    ROOT_GENERATE_DICTIONARY(G__jv_mainframe jv_mainframe.h)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(janaview SHARED
            JEventProcessor_janaview.cc
            jv_mainframe.cc
            G__jv_mainframe.cxx
        )

    find_package(Threads REQUIRED)
    target_include_directories(janaview PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janaview jana2)
    target_link_libraries(janaview Threads::Threads)
    target_link_libraries(janaview ${ROOT_LIBRARIES})
    set_target_properties(janaview PROPERTIES PREFIX "" OUTPUT_NAME "janaview" SUFFIX ".so")
    install(TARGETS janaview DESTINATION plugins)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjv_mainframe_rdict.pcm DESTINATION plugins)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjv_mainframe.rootmap DESTINATION plugins)
        
    file(GLOB my_headers "*.h*")
    install(FILES ${my_headers} DESTINATION include/janaview)

else()
message(STATUS "Skipping plugins/janaview because USE_ROOT=Off")

endif() # ROOT_FOUND
