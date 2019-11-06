

add_executable(jana_cli
    jana.cc
    JBenchmarker.h
    JBenchmarker.cc
    MyProcessor.cc
    MyProcessor.h
    )

target_link_libraries(jana_cli jana2 Threads::Threads)

