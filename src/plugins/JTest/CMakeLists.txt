

file(GLOB JTEST_HEADERS "*.h*")

add_jana_plugin(JTest SOURCES JTestMain.cc PUBLIC_HEADER ${JTEST_HEADERS})

find_package(Threads REQUIRED)
target_link_libraries(JTest PUBLIC Threads::Threads)

