

add_jana_plugin(JTest)

add_test(NAME jana-plugin-jtest-tests
    COMMAND $<TARGET_FILE:jana> -Pplugins=JTest -Pjana:nevents=20)


