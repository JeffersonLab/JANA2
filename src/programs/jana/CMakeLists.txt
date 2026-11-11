

add_executable(jana jana.cc )

find_package(Threads REQUIRED)
target_link_libraries(jana jana2_shared_lib)
target_link_options(jana PRIVATE -rdynamic)
install(TARGETS jana DESTINATION ${CMAKE_INSTALL_BINDIR})



if(EMSCRIPTEN)
    target_link_options(jana PRIVATE "-sEXPORTED_RUNTIME_METHODS=callMain,FS")
    install(FILES $<TARGET_FILE_DIR:jana>/jana.wasm DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES $<TARGET_FILE_DIR:jana>/jana.worker.js DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()
