//
// Created by nbrei on 4/3/19.
//

#include <greenfield/ServiceLocator.h>

namespace greenfield {

// Global variable which serves as a replacement for japp
// Ideally we would make this be a singleton instead, but until we figure out
// how to merge our plugins' symbol tables correctly, singletons are broken.
ServiceLocator *serviceLocator;

} // namespace greenfield