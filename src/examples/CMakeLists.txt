
add_subdirectory(tutorial_with_lightweight_datamodel)
add_subdirectory(misc)

if (${USE_PODIO})
    add_subdirectory(tutorial_with_podio_datamodel)
endif()
