
#pragma once
#include <JANA/Geometry/JGeometry.h>

namespace jana::compatibility::jgeometry {
[[deprecated("Use JANA/Geometry/JGeometry.h instead")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}
