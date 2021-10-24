
add_library(janaview SHARED
        JEventProcessor_janaview.cc
        jv_mainframe.cc
    )

find_package(Threads REQUIRED)
target_link_libraries(janaview jana2)
target_link_libraries(janaview Threads::Threads)
set_target_properties(janaview PROPERTIES PREFIX "" OUTPUT_NAME "janaview" SUFFIX ".so")
install(TARGETS janaview DESTINATION plugins)
