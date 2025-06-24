
if (USE_PODIO)

    add_jana_plugin(PodioFactories)
    target_link_libraries(PodioFactories PUBLIC PodioDatamodel PodioDatamodelDict)

else()

    message(STATUS "Skipping examples/PodioFactories because USE_PODIO=Off")

endif()



