// $Id$
//
//    File: JResourceManager_test.cc
// Created: Thu Oct 25 09:13:59 EDT 2012
// Creator: davidl (on Linux ifarm1101 2.6.18-274.3.1.el5 x86_64)
//

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
using namespace std;
		 
#include <JANA/JApplication.h>
#include <JANA/JResourceManager.h>
using namespace jana;

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

unsigned int TestResourceManager(void);

//------------------
// main
//------------------
int main(int narg, char *argv[])
{
	cout<<endl;
	jout<<"----- starting JResourceManager unit test ------"<<endl;

	// Create a JApplication object
	JApplication *app = new JApplication(narg, argv);

	int result = Catch::Main( narg, argv );

	// Cleanup
	delete app;

	jout<<"----- finished JResourceManager unit test ------"<<endl;

	return result;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("resource/get", "Gets a remote resource using JResourceManager through JApplication")
{
	REQUIRE( TestResourceManager() == 175951 );
}

//------------------
// TestResourceManager
//------------------
unsigned int TestResourceManager(void)
{
	// Create a temporary calibration directory
	char cwd_buff[1024];
	getcwd(cwd_buff, 1024);
	string cwd(cwd_buff);
	string calibdir = cwd + "/tmp_calib";
	jout<<"Making temporary calibration directory: "<< calibdir <<endl;
	mkdir(calibdir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	
	// Create an empty info.xml file in resources directory
	// to avoid warning from JCalibrationFile
	string info_xml = calibdir + "/info.xml";
	ofstream tmp_ofs(info_xml.c_str());
	tmp_ofs.close();
	
	// Make subdirectory "test"
	mkdir((calibdir+"/test").c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	
	// Create a resource description file in the new directory
	string fname = "test/fieldmap";
	string fname_fullpath = calibdir + "/" + fname;
	string URL = "https://halldsvn.jlab.org/repos/tags/calib-2011-07-31/Magnets/Solenoid/solenoid_1500_poisson_20090814_01";
	jout<<"Writing "<<fname<<" to calibration directory ..." <<endl;
	ofstream ofs(fname_fullpath.c_str());
	ofs<<"# Auto-generated, temporary test file" << endl;
	ofs<<"URL " << URL << endl;
	ofs<<endl;
	ofs.close();
	
	// Set the JANA_CALIB_URL environment variable to point to
	// the temporary directory.
	char putenv_str[256];
	sprintf(putenv_str, "JANA_CALIB_URL=file://%s", calibdir.c_str());
	putenv(putenv_str);
	
	// Set the resource directory name and set the
	// JANA:RESOURCE_DIR configuration parameter to
	// point to it
	string resource_dir = cwd + "/tmp_resources";
	jout<<"Setting resources directory to: "<<resource_dir<<endl;
	gPARMS->SetParameter("JANA:RESOURCE_DIR", resource_dir);

	// Have JApplication create a JCalibration object
	japp->GetJCalibration(1);
	
	// Have JApplication create a JResourceManager object
	JResourceManager *jrm = japp->GetJResourceManager(1);

	// Get the resource
	vector< vector<float> > Bmap;
	jout << "Reading field map ..." << endl;
	jrm->Get("test/fieldmap", Bmap);
	jout<<Bmap.size()<<" entries found" << endl;
	
	// Clean up temporary directories
	jout<<"Removing temporary directories/files ..."<<endl;
	system("rm -rf tmp_calib tmp_resources");

	return Bmap.size();
}
