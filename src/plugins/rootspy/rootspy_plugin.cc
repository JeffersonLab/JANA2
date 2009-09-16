// $Id$
//
//    File: rootspy_plugin.cc
// Created: Tue Sep 15 11:45:07 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//


#include <JANA/JApplication.h>
using namespace jana;

#include <DRootSpy.h>


// Entrance point for plugin
extern "C"{
void InitPlugin(JApplication *japp){
	InitJANAPlugin(japp);
	
	new DRootSpy();
}
}
