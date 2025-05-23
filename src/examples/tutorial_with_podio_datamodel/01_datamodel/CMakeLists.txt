

PODIO_GENERATE_DATAMODEL(jana2_tutorial_podio_datamodel datamodel.yaml headers sources 
    IO_BACKEND_HANDLERS ROOT
    OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)

PODIO_ADD_DATAMODEL_CORE_LIB(jana2_tutorial_podio_datamodel "${headers}" "${sources}"
    OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})

PODIO_ADD_ROOT_IO_DICT(jana2_tutorial_podio_datamodel_dict jana2_tutorial_podio_datamodel "${headers}"
    ${CMAKE_CURRENT_BINARY_DIR}/src/selection.xml)

install(TARGETS jana2_tutorial_podio_datamodel
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/jana2_tutorial_podio/datamodel
)

install(TARGETS jana2_tutorial_podio_datamodel_dict
    DESTINATION lib
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jana2_tutorial_podio_datamodel_dictDict.rootmap DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjana2_tutorial_podio_datamodel_dict_rdict.pcm DESTINATION lib)

