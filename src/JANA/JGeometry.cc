// $Id: JGeometry.cc 1763 2006-05-10 14:29:25Z davidl $
//
//    File: JGeometry.cc
// Created: Thu Aug 11 22:17:29 EDT 2005
// Creator: davidl (on Darwin Harriet.local 7.8.0 powerpc)
//

#include "JGeometry.h"

//---------------------------------
// JGeometry    (Constructor)
//---------------------------------
JGeometry::JGeometry(unsigned int run_number)
{
	/// The value of run_number is the run for which the geometry is
	/// requested. The values of min_run_number and max_run_number
	/// should be set here to reflect the range of runs for which
	/// this geometry is valid. These are checked using the inline
	/// method IsInRange() from JApplication when a geometry object
	/// is requested.

	/// (Placeholder for now. This should come from the database ...)
	min_run_number = max_run_number = run_number;
	
}

//---------------------------------
// JGeometry    (Destructor)
//---------------------------------
JGeometry::~JGeometry()
{

}
