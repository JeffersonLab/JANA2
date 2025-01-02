
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Services/JLockService.h>

namespace jana::compatibility::jlockservice {
[[deprecated("Use JANA/Services/JLockService.h instead")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}


