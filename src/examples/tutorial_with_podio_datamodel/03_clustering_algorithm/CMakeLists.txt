
add_jana_plugin(PodioFactories)
target_link_libraries(PodioFactories PUBLIC PodioDatamodel PodioDatamodelDict)

add_executable(PodioExample ${PodioExample_SOURCES})
target_link_libraries(PodioExample 
    PUBLIC jana2_static_lib PodioDatamodel PodioDatamodelDict podio::podioRootIO)

set_target_properties(PodioExample PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

add_test(NAME jana-example-podio-tests COMMAND PodioExample)


