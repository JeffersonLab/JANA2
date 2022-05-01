
add_library(jtest SHARED
    JTestMain.cc
    JTestDataObjects.h
    JTestParser.h
    JTestDisentangler.h
    JTestTracker.h
    JTestPlotter.h
    JTestCalibrationService.h
    )

find_package(Threads REQUIRED)
target_link_libraries(jtest jana2)
target_link_libraries(jtest Threads::Threads)
set_target_properties(jtest PROPERTIES PREFIX "" OUTPUT_NAME "JTest" SUFFIX ".so")
install(TARGETS jtest DESTINATION plugins)
        
file(GLOB my_headers "*.h*")
install(FILES ${my_headers} DESTINATION include/JTest)
