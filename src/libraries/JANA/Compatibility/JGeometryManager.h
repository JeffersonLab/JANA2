
#pragma once
#include <JANA/Geometry/JGeometryManager.h>

namespace jana::compatibility::jgeometrymanager {
[[deprecated("Use JANA/Geometry/JGeometryManager.h instead")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}


