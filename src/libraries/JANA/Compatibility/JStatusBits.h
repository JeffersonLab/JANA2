
#pragma once
#include <JANA/Utils/JStatusBits.h>

namespace jana::compatibility::jstatusbits {
[[deprecated("Use JANA/Utils/JStatusBits.h instead")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}

