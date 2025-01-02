
#pragma once
#include <JANA/Geometry/JGeometryXML.h>

namespace jana::compatibility::jgeometryxml {
[[deprecated("Use JANA/Geometry/JGeometryXML.h instead")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}

