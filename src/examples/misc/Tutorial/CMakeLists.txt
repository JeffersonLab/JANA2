

add_jana_plugin(Tutorial)

add_test(NAME jana-example-tutorial-tests
    COMMAND jana -Pplugins=Tutorial -Pjana:nevents=50)


