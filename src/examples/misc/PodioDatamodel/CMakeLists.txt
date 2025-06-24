
if (USE_PODIO)

    PODIO_GENERATE_DATAMODEL(PodioDatamodel datamodel.yaml headers sources 
        IO_BACKEND_HANDLERS ROOT
        OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
    )

    PODIO_ADD_DATAMODEL_CORE_LIB(PodioDatamodel "${headers}" "${sources}"
        OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})

    PODIO_ADD_ROOT_IO_DICT(PodioDatamodelDict PodioDatamodel "${headers}" 
        ${CMAKE_CURRENT_BINARY_DIR}/src/selection.xml)

    install(TARGETS PodioDatamodel
        EXPORT jana2_targets
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/JANA/examples/PodioDatamodel
    )

    install(TARGETS PodioDatamodelDict
        EXPORT jana2_targets
        DESTINATION lib
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PodioDatamodelDictDict.rootmap DESTINATION lib)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libPodioDatamodelDict_rdict.pcm DESTINATION lib)

else()
    message(STATUS "Skipping examples/PodioDatamodel because USE_PODIO=Off")

endif()



