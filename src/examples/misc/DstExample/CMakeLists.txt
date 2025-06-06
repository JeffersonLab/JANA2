

add_jana_plugin(DstExample)

add_test(
    NAME jana-example-dst-tests 
    COMMAND jana -Pplugins=DstExample)
