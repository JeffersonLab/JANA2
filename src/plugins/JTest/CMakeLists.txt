

add_jana_plugin(JTest)

add_test(NAME jana-plugin-jtest-tests
    COMMAND jana -Pplugins=JTest -Pjana:nevents=20)


