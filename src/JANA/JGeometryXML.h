// $Id$
//
//    File: JGeometryXML.h
// Created: Tues Jan 15 2008
// Creator: davidl
//

#ifndef _JGeometryXML_
#define _JGeometryXML_

#include <iostream>
using std::cout;
using std::endl;

#include "jerror.h"
#include "JGeometry.h"

#ifdef XERCESC
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#endif

class JGeometryXML:public JGeometry{
	public:
		JGeometryXML(string url, int run, string context="default");
		virtual ~JGeometryXML();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JGeometryXML";}
		
		bool Get(string xpath, string &sval);
		bool Get(string xpath, map<string, string> &svals);
		void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level);
		
	protected:
	
	private:
		JGeometryXML();
		
		string xmlfile;

#ifdef XERCESC
		xercesc::DOMBuilder *parser;
		xercesc::DOMDocument *doc;
		
		void AddNodeToList(xercesc::DOMNode* start, string start_path, vector<string> &xpaths, JGeometry::ATTR_LEVEL_t level);
		xercesc::DOMNode* FindNode(string xpath, string &attribute);
		void ParseXPath(string xpath, vector<pair<string, map<string,string> > > &nodes, string &attribute);
		xercesc::DOMNode* SearchTree(xercesc::DOMNode* current_node, unsigned int depth, vector<pair<string, map<string,string> > > &nodes);

		// Error handler callback class
		class ErrorHandler : public xercesc::DOMErrorHandler
		{
			public:
				 //  Constructors and Destructor
				 ErrorHandler(){}
				 ~ErrorHandler(){}
				 bool handleError(const xercesc::DOMError& domError){cout<<"Got Error!!"<<endl; return false;}
				 void resetErrors();

			private :
				 //  Unimplemented constructors and operators
				 ErrorHandler(const ErrorHandler&);
				 void operator=(const ErrorHandler&);
		};
#endif

};

#endif // _JGeometryXML_

