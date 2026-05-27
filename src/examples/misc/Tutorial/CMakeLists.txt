

add_jana_plugin(Tutorial)

add_test(NAME jana-example-tutorial-tests
    COMMAND $<TARGET_FILE:jana> -Pplugins=Tutorial -Pjana:nevents=50)


