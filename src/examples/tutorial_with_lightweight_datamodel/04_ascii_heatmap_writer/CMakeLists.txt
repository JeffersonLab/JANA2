

add_jana_library(lw_ascii_heatmap_writer_common
    SOURCES
        AsciiHeatmap_writer.cc
        AsciiHeatmap_writer_legacy.cc
    PUBLIC_HEADER
        AsciiHeatmap_writer.h
        AsciiHeatmap_writer_legacy.h
    TESTS
        AsciiHeatmap_writer_tests.cc
)

target_link_libraries(lw_ascii_heatmap_writer_common PUBLIC lw_datamodel)

add_jana_plugin(lw_ascii_heatmap_writer SOURCES ascii_heatmap_writer_plugin.cc)

target_link_libraries(lw_ascii_heatmap_writer PUBLIC lw_ascii_heatmap_writer_common)





