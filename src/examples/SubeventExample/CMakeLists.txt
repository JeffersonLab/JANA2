

set (SubeventExample_SOURCES
		SubeventExample.cc
	)

add_executable(SubeventExample ${SubeventExample_SOURCES})

target_link_libraries(SubeventExample jana2)
set_target_properties(SubeventExample PROPERTIES PREFIX "" OUTPUT_NAME "SubeventExample")
install(TARGETS SubeventExample DESTINATION bin)


