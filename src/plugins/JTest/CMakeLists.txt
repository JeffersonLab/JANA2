
add_library(jtest SHARED
    JTestMain.cc
    JTestDataObjects.h
    JTestParser.h
    JTestDisentangler.h
    JTestTracker.h
    JTestPlotter.h
    JTestFactoryGenerator.h
    JTestCalibrationService.h
    )

find_package(Threads REQUIRED)
target_link_libraries(jtest jana2)
target_link_libraries(jtest Threads::Threads)
set_target_properties(jtest PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS jtest DESTINATION plugins)
