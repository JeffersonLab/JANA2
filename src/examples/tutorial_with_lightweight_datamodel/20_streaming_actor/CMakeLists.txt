

add_jana_plugin(lw_streaming_actor)

add_test(NAME jana2-tut-lw-streaming-actor
    COMMAND jana -Pplugins=lw_streaming_actor)

