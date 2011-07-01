
#include <stdlib.h>

#include <iostream>
using namespace std;

#include <THtml.h>

#include <JANA/JVersion.h>
#include <JANA/JApplication.h>
using namespace jana;

#include "mainpage.c++"

JVersion::JVersion(){}
JVersion::~JVersion(){}

int main(int argc, char *argv[])
{
	JApplication japp(argc, argv);
	JVersion v;

	// Create THtml object and generate documentation
	THtml html;
	html.SetProductName("JANA");
	
	// Get full paths to source directories via environment
	string root_src = string(getenv("ROOTSYS"))+"/include";
	string jana_src = string(getenv("JANA_HOME"))+"/src/JANA";
	string inputdir = jana_src+":"+root_src;
	cout<<"Setting input dir path to: "<<inputdir<<endl;
	html.SetInputDir(inputdir.c_str());
	
	cout<<"========================="<<endl;

#if 0	
	html.MakeClass("jana::JObject");
	html.MakeClass("jana::JEvent");
	html.MakeClass("jana::JFactory_base");
	html.MakeClass("jana::JApplication");
	html.MakeClass("jana::JEventLoop");
	html.MakeClass("jana::JEventProcessor");
	html.MakeClass("jana::JParameter");
	html.MakeClass("jana::JParameterManager");
#endif

	//html.MakeAll(kFALSE, "jana::*");
	//html.MakeIndex("jana::*");

	html.MakeAll(kFALSE, "jana::*|JStreamLog*"); // for some reason. JStreamLog* classes are not in jana namespace
	//html.MakeAll(kFALSE, "jana::*");
	//html.MakeIndex("J*");
	
	return 0;
}