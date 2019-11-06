
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

target_link_libraries(jtest jana2)
target_link_libraries(jtest Threads::Threads)
set_target_properties(jtest PROPERTIES PREFIX "" SUFFIX ".so")
