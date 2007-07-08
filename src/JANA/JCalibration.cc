// $Id$
//
//    File: JCalibration.cc
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#include "JCalibration.h"

//---------------------------------
// JCalibration    (Constructor)
//---------------------------------
JCalibration::JCalibration(string url, int run, string context)
{
	this->url = url;
	this->run_requested = run;
	this->context = context;
}

//---------------------------------
// ~JCalibration    (Destructor)
//---------------------------------
JCalibration::~JCalibration()
{

}
