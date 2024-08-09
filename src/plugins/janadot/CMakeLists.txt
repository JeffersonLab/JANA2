
add_jana_plugin(janadot)

find_package(Threads REQUIRED)
target_link_libraries(janadot PUBLIC Threads::Threads)

add_test(NAME jana-plugin-janadot-tests
    COMMAND jana -Pplugins=JTest,janadot -Pjana:nevents=10)

# TODO: Test that file is not empty!

