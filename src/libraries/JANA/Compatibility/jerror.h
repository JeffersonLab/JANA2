//
/// jerror.h
/// 
/// This file contains error codes for errors specific to the
/// analysis code. Many functions return values
/// of type jerror_t. This header should be included in all
/// files which must deal with this type.

#pragma once

namespace jana::compatibility::jerror {
[[deprecated("This will be completely removed from JANA with no replacement. If you still need jerror.h, copy it into your own project.")]]
constexpr static int header_is_deprecated = 0;
constexpr static int warn_about_header_deprecation = header_is_deprecated;
}

enum jerror_t{
    NOERROR = 0,
    UNKNOWN_ERROR = -1000,

    MAX_EVENT_PROCESSORS_EXCEEDED,

    ERROR_OPENING_EVENT_SOURCE,
    ERROR_CLOSING_EVENT_SOURCE,
    NO_MORE_EVENTS_IN_SOURCE,
    NO_MORE_EVENT_SOURCES,
    EVENT_NOT_IN_MEMORY,
    EVENT_SOURCE_NOT_OPEN,
    OBJECT_NOT_AVAILABLE,
    DEVENT_OBJECT_DOES_NOT_EXIST,

    MEMORY_ALLOCATION_ERROR,

    RESOURCE_UNAVAILABLE,
    VALUE_OUT_OF_RANGE,

    INFINITE_RECURSION,
    UNRECOVERABLE_ERROR,

    FILTER_EVENT_OUT
};

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include <JANA/Compatibility/cint.h>


