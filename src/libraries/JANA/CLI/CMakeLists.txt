

# Generate JVersion.h

configure_file(JVersion.h.in JVersion.h)
install(FILES JVersion.h DESTINATION include/JANA/CLI)
