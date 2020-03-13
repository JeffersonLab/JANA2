

find_package(ROOT)

if (ROOT_FOUND)

    add_library(janarate SHARED JEventProcessorJANARATE.cc)
    target_include_directories(janarate PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(janarate jana2 ${ROOT_LIBRARIES})
    set_target_properties(janarate PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS janarate DESTINATION plugins)

else()
    message(WARNING "ROOT not found. Skipping compilation of janarate.so")
#    add_library(janarate SHARED EXCLUDE_FROM_ALL JEventProcessorJANARATE.cc)

endif()

