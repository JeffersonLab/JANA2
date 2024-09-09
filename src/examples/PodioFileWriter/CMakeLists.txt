
if (USE_PODIO)

    add_jana_plugin(PodioFileWriter)
    
    target_link_libraries(PodioFileWriter 
        PUBLIC PodioDatamodel PodioDatamodelDict podio::podioRootIO)

else()
    message(STATUS "Skipping examples/PodioFileWriter because USE_PODIO=Off")

endif()



