// $Id: JGeometry.h 1763 2006-05-10 14:29:25Z davidl $
//
//    File: JGeometry.h
// Created: Thu Aug 11 22:17:29 EDT 2005
// Creator: davidl (on Darwin Harriet.local 7.8.0 powerpc)
//

#ifndef _JGeometry_
#define _JGeometry_

#include "jerror.h"


class JGeometry{
	public:
		JGeometry(){}
		JGeometry(unsigned int run_number);
		virtual ~JGeometry();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JGeometry";}

		inline bool IsInRange(unsigned int run){return run>=min_run_number && run<=max_run_number;}
				
	protected:
		unsigned int min_run_number;	
		unsigned int max_run_number;
		
	
	private:

};

#endif // _JGeometry_

