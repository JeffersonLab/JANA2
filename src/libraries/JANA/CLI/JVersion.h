#pragma once

// We are moving JVersion.h out of CLI. 
// In the future, use #include <JANA/JVersion.h> instead.

#include <JANA/JVersion.h>

namespace jana::cli::jversion {
[[deprecated("Use JANA/JVersion.h instead")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}
