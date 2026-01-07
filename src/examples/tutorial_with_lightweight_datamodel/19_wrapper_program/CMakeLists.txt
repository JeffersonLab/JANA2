
add_executable(lw-tutorial-recon tutorial_recon_main.cc)

target_link_libraries(lw-tutorial-recon PRIVATE
  jana2_shared_lib
  lw_protocluster_common
  lw_random_hit_source_common
  lw_ascii_heatmap_writer_common
)
# Note that if you are outside the JANA2 source tree, you need to target_link_libraries against JANA::jana2_shared_lib instead

install(TARGETS lw-tutorial-recon DESTINATION bin)

