

add_jana_plugin(lw_random_hit_source)

add_test(NAME jana-tut-lw-02randhitsource
    COMMAND jana -Pplugins=Tutorial -Pjana:nevents=50)


