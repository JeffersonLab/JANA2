//
// Created by nbrei on 4/3/19.
//

#include <JANA/JServiceLocator.h>


// Global variable which serves as a replacement for japp
// Ideally we would make this be a singleton instead, but until we figure out
// how to merge our plugins' symbol tables correctly, singletons are broken.
JServiceLocator *serviceLocator;

