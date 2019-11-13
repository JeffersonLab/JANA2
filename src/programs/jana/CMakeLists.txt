

add_executable(jana
    jana.cc
    JBenchmarker.h
    JBenchmarker.cc
    MyProcessor.cc
    MyProcessor.h
    )

find_package(Threads REQUIRED)
target_link_libraries(jana jana2 Threads::Threads)
install(TARGETS jana DESTINATION bin)

