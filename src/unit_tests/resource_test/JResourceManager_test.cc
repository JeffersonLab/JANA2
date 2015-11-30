// $Id$
//
//    File: JResourceManager_test.cc
// Created: Thu Oct 25 09:13:59 EDT 2012
// Creator: davidl (on Linux ifarm1101 2.6.18-274.3.1.el5 x86_64)
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
using namespace std;
		 
#include <JANA/JApplication.h>
#include <JANA/JResourceManager.h>
using namespace jana;

#define CATCH_CONFIG_RUNNER
#include "../catch.hpp"

int NARG;
char **ARGV;

unsigned int TestResourceManager_Option1(void);
unsigned int TestResourceManager_Option2(void);
unsigned int TestResourceManager_Option3(void);
string       TestResourceManager_DIR(const char* confvar, const char *envar, const char *defpath);

//------------------
// main
//------------------
int main(int narg, char *argv[])
{
	cout<<endl;
	jout<<"----- starting JResourceManager unit test ------"<<endl;

	// Record command line args for later use
	NARG = narg;
	ARGV = argv;

	int result = Catch::Main( narg, argv );

	return result;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("resource/get option 1", "Gets a remote resource using JResourceManager through JApplication")
{
	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);

	// First time should download
	jout << "------------------ Option 1 Test 1 (download)-------------------" << endl;
	REQUIRE( TestResourceManager_Option1() == 175951 );

	// Second time uses already downloaded file
	jout << "------------------ Option 1 Test 2 (no download)-------------------" << endl;
	REQUIRE( TestResourceManager_Option1() == 175951 );

	// Check that option 2 works with file downloaded using option 1
	jout << "------------------ Option 1 Test 3 (no download)-------------------" << endl;
	REQUIRE( TestResourceManager_Option2() == 175951 );

	// Clean up temporary directories
	jout<<"Removing temporary directories/files ..."<<endl;
	system("rm -rf tmp_calib tmp_resources");

	delete app;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("resource/get option 2", "Gets a remote resource using JResourceManager through JApplication")
{
	// Create JApplication for use with last 2 tests
	JApplication *app = new JApplication(NARG, ARGV);

	// First time should download
	jout << "------------------ Option 2 Test 1 (download)-------------------" << endl;
	REQUIRE( TestResourceManager_Option2() == 175951 );

	// Second time uses already downloaded file
	jout << "------------------ Option 2 Test 2 (no download)-------------------" << endl;
	REQUIRE( TestResourceManager_Option2() == 175951 );

	// Clean up temporary directories
	jout<<"Removing temporary directories/files ..."<<endl;
	system("rm -rf tmp_calib tmp_resources");

	delete app;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("resource/get option 3", "Gets a remote resource using JResourceManager through JApplication")
{
	// Create JApplication for use with last 2 tests
	JApplication *app = new JApplication(NARG, ARGV);

	// First time should download
	jout << "------------------ Option 3 Test 1 (download)-------------------" << endl;
	REQUIRE( TestResourceManager_Option3() == 175951 );

	// Clean up temporary directories
	jout<<"Removing temporary directories/files ..."<<endl;
	system("rm -rf tmp_calib tmp_resources");
	
	delete app;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("resource/dir", "Checks that resource manager chooses directory correctly")
{
	string user = (getenv("USER")==NULL ? "jana":getenv("USER"));
	string tmp_dir = "/tmp/" + user + "/resources";

	char cwd_buff[1024];
	getcwd(cwd_buff, 1024);
	string local_dir = string(cwd_buff);
	
	string mypath1 = string("/bad/path1:/bad/path2:")+local_dir;
	string mypath2 = string("/bad/path1:")+local_dir+":/bad/path2";
	string mypath3 = local_dir+":/bad/path1:/bad/path2";
	string mypath4 = string("/bad/path1:/bad/path2:")+local_dir+"/extra";

	jout << "------------------ No DIR specified -------------------" << endl;
	REQUIRE( TestResourceManager_DIR(NULL, NULL , NULL) == tmp_dir+"/res/path/test" );

	jout << "------------------ JANA:RESOURCE_DIR -------------------" << endl;
	REQUIRE( TestResourceManager_DIR(local_dir.c_str(), NULL , NULL) == local_dir+"/res/path/test" );
	REQUIRE( TestResourceManager_DIR(local_dir.c_str(), "/bad/path" , NULL) == local_dir+"/res/path/test" );
	REQUIRE( TestResourceManager_DIR(local_dir.c_str(), "/bad/path" , mypath1.c_str()) == local_dir+"/res/path/test" );

	jout << "------------------ JANA_RESOURCE_DIR -------------------" << endl;
	REQUIRE( TestResourceManager_DIR(NULL, local_dir.c_str(), NULL) == local_dir+"/res/path/test" );
	REQUIRE( TestResourceManager_DIR(NULL, local_dir.c_str(), mypath1.c_str()) == local_dir+"/res/path/test" );

	jout << "------------------ JANA:RESOURCE_DEFAULT_PATH -------------------" << endl;
	REQUIRE( TestResourceManager_DIR(NULL, NULL, mypath1.c_str()) == local_dir+"/res/path/test" );
	REQUIRE( TestResourceManager_DIR(NULL, NULL, mypath2.c_str()) == local_dir+"/res/path/test" );
	REQUIRE( TestResourceManager_DIR(NULL, NULL, mypath3.c_str()) == local_dir+"/res/path/test" );
	REQUIRE( TestResourceManager_DIR(NULL, NULL, mypath4.c_str()) == tmp_dir+"/res/path/test" );
}

//------------------
// TestResourceManager_Option1
//------------------
unsigned int TestResourceManager_Option1(void)
{
	// Option 1 uses the "URL_base" and "path" keys in the calibDB

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
	_DBG_<<"Creating file: "<<info_xml<<endl;
	ofstream tmp_ofs(info_xml.c_str());
	tmp_ofs.close();
	
	// Make subdirectory "test"
	mkdir((calibdir+"/test").c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	
	// Create a resource description file in the new directory
	string fname = "test/fieldmap";
	string fname_fullpath = calibdir + "/" + fname;
	string URL_base = "https://halldsvn.jlab.org/repos/tags/calib-2011-07-31";
	string path = "Magnets/Solenoid/solenoid_1500_poisson_20090814_01";
	jout<<"Writing "<<fname<<" to calibration directory ..." <<endl;
	ofstream ofs(fname_fullpath.c_str());
	ofs<<"# Auto-generated, temporary test file" << endl;
	ofs<<"URL_base " << URL_base << endl;
	ofs<<"path " << path << endl;
	ofs<<endl;
	ofs.close();

	// Set the JANA_CALIB_URL environment variable to point to
	// the temporary directory.
	char putenv_str[256];
	sprintf(putenv_str, "JANA_CALIB_URL=file://%s", calibdir.c_str());
	putenv(putenv_str);
	_DBG_<<putenv_str<<endl;
	_DBG_<<"URL_base="<<URL_base<<"  path="<<path<<endl;
	// Set the resource directory name and set the
	// JANA:RESOURCE_DIR configuration parameter to
	// point to it
	string resource_dir = cwd + "/tmp_resources";
	jout<<"Setting resources directory to: "<<resource_dir<<endl;
	gPARMS->SetParameter("JANA:RESOURCE_DIR", resource_dir);

	// Create an empty info.xml file in resources directory
	// to avoid warning from JCalibrationFile
	info_xml = resource_dir + "/info.xml";
	_DBG_<<"Creating file: "<<info_xml<<endl;
	tmp_ofs.open(info_xml.c_str());
	tmp_ofs.close();

	// Have JApplication create a JCalibration object
	japp->GetJCalibration(1);
	
	// Have JApplication create a JResourceManager object
	JResourceManager *jrm = japp->GetJResourceManager(1);

	// Get the resource
	vector< vector<float> > Bmap;
	jout << "Reading field map ..." << endl;
	jrm->Get("test/fieldmap", Bmap);
	jout<<Bmap.size()<<" entries found" << endl;
	
	return Bmap.size();
}

//------------------
// TestResourceManager_Option2
//------------------
unsigned int TestResourceManager_Option2(void)
{
	// Option 2 uses only the "URL" key in the calibDB

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
	_DBG_<<"Creating file: "<<info_xml<<endl;
	
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

	// Create an empty info.xml file in resources directory
	// to avoid warning from JCalibrationFile
	info_xml = resource_dir + "/info.xml";
	_DBG_<<"Creating file: "<<info_xml<<endl;
	tmp_ofs.open(info_xml.c_str());
	tmp_ofs.close();

	// Have JApplication create a JCalibration object
	japp->GetJCalibration(1);
	
	// Have JApplication create a JResourceManager object
	JResourceManager *jrm = japp->GetJResourceManager(1);

	// Get the resource
	vector< vector<float> > Bmap;
	jout << "Reading field map ..." << endl;
	jrm->Get("test/fieldmap", Bmap);
	jout<<Bmap.size()<<" entries found" << endl;
	
	return Bmap.size();
}


//------------------
// TestResourceManager_Option3
//------------------
unsigned int TestResourceManager_Option3(void)
{
	// Option 3 uses the "path" keys in the calibDB and
	// the JANA:RESOURCE_URL for the URL_base value

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
	string URL_base = "https://halldsvn.jlab.org/repos/tags/calib-2011-07-31";
	string path = "Magnets/Solenoid/solenoid_1500_poisson_20090814_01";
	jout<<"Writing "<<fname<<" to calibration directory ..." <<endl;
	ofstream ofs(fname_fullpath.c_str());
	ofs<<"# Auto-generated, temporary test file" << endl;
	//ofs<<"URL_base " << URL_base << endl;
	ofs<<"path " << path << endl;
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

	jout<<"Setting JANA:RESOURCE_URL to: "<<URL_base<<endl;
	gPARMS->SetParameter("JANA:RESOURCE_URL", URL_base);

	// Have JApplication create a JCalibration object
	japp->GetJCalibration(1);
	
	// Have JApplication create a JResourceManager object
	JResourceManager *jrm = japp->GetJResourceManager(1);

	// Get the resource
	vector< vector<float> > Bmap;
	jout << "Reading field map ..." << endl;
	jrm->Get("test/fieldmap", Bmap);
	jout<<Bmap.size()<<" entries found" << endl;
	
	return Bmap.size();
}

//------------------
// TestResourceManager_DIR
//------------------
string TestResourceManager_DIR(const char* confvar, const char *envar, const char *defpath)
{
	// Set the resource directory using various means and
	// return the one the resource manager chooses.
	
	// Create new JApplication for use in this test only
	JApplication *japp = new JApplication(NARG, ARGV);
	
	// Set JANA:RESOURCE_DIR configuration parameter if given
	if(confvar) gPARMS->SetParameter("JANA:RESOURCE_DIR", string(confvar));
	
	// Set environment variable
	if(envar){
		setenv("JANA_RESOURCE_DIR", strdup(envar), 1);
	}else{
		unsetenv("JANA_RESOURCE_DIR");
	}

	// Set JANA:RESOURCE_DEFAULT_PATH configuration parameter if given
	if(defpath) gPARMS->SetParameter("JANA:RESOURCE_DEFAULT_PATH", string(defpath));

	// Get the resource manager (this should actually instantiate it)
	JResourceManager *jrm = japp->GetJResourceManager(1);
	
	string path = jrm->GetLocalPathToResource("res/path/test");

	delete japp;

	return path;
}

