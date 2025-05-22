

add_jana_plugin(jana2_tut_lw_02_random_hit_source)

add_test(NAME jana-tut-lw-02randhitsource
    COMMAND jana -Pplugins=Tutorial -Pjana:nevents=50)


