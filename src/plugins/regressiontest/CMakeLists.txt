
add_jana_plugin(regressiontest 
    SOURCES JEventProcessor_regressiontest.cc 
    PUBLIC_HEADER JEventProcessor_regressiontest.h)

find_package(Threads REQUIRED)
target_link_libraries(regressiontest PUBLIC Threads::Threads)

