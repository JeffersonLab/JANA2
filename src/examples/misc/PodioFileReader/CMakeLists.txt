
if (USE_PODIO)

    add_jana_plugin(PodioFileReader)
    
    target_link_libraries(PodioFileReader 
        PUBLIC PodioDatamodel PodioDatamodelDict podio::podioRootIO)

else()
    message(STATUS "Skipping examples/PodioFileReader because USE_PODIO=Off")

endif()



