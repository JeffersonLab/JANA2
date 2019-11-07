

find_package(ROOT)

if (ROOT_FOUND)

    add_library(janarate SHARED JEventProcessorJANARATE.cc)

else()

    add_library(janarate SHARED EXCLUDE_FROM_ALL JEventProcessorJANARATE.cc)
    message(WARNING "Skipping compilation of janarate.so")

    if (NOT ROOT_FOUND)
        message(WARNING "Unable to find ROOT")
    endif()

endif()

target_include_directories(janarate PUBLIC ${ROOT_INCLUDE_DIRS})
target_link_libraries(janarate jana2 ${ROOT_LIBRARIES})
set_target_properties(janarate PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS janarate DESTINATION plugins)
