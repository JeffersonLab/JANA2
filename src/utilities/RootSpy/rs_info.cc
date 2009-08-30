// $Id$
//
//    File: rs_info.cc
// Created: Sat Aug 29 07:42:56 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include <iostream>
#include <iomanip>
using namespace std;


#include "rs_info.h"

//---------------------------------
// rs_info    (Constructor)
//---------------------------------
rs_info::rs_info()
{
	pthread_mutex_init(&mutex, NULL);

	selected_server = "N/A";
	selected_hist = "-------------------------------------------------------";
	
	latest_hist = NULL;
	latest_hist_received_time = time(NULL);
}

//---------------------------------
// ~rs_info    (Destructor)
//---------------------------------
rs_info::~rs_info()
{

}
