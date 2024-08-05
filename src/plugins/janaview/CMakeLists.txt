

if(USE_ROOT)

    find_package(ROOT REQUIRED COMPONENTS Gui Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore Thread MultiProc)
    ROOT_GENERATE_DICTIONARY(G__jv_mainframe jv_mainframe.h)

    file(GLOB JANAVIEW_HEADERS "*.h*")

    add_jana_plugin(janaview 
        SOURCES JEventProcessor_janaview.cc jv_mainframe.cc G__jv_mainframe.cxx
        PUBLIC_HEADER ${JANAVIEW_HEADERS}
    )

    target_include_directories(janaview PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janaview PUBLIC ${ROOT_LIBRARIES})

    find_package(Threads REQUIRED)
    target_link_libraries(janaview PUBLIC Threads::Threads)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjv_mainframe_rdict.pcm DESTINATION lib/JANA/plugins)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjv_mainframe.rootmap DESTINATION lib/JANA/plugins)

else()

    message(STATUS "Skipping plugins/janaview because USE_ROOT=Off")

endif() # ROOT_FOUND
