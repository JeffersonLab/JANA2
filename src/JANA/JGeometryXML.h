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

#include <JANA/jerror.h>
#include <JANA/JGeometry.h>
#include <JANA/jana_config.h>

#if HAVE_XERCES
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#endif

// Place everything in JANA namespace
namespace jana
{

class JGeometryXML:public JGeometry{
	public:
		JGeometryXML(string url, int run, string context="default");
		virtual ~JGeometryXML();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JGeometryXML";}
		
		bool Get(string xpath, string &sval);
		bool Get(string xpath, map<string, string> &svals);
		void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level, const string &filter="");

		typedef pair<string, map<string,string> > node_t;
		typedef vector<node_t>::iterator node_iter_t;

		void ParseXPath(string xpath, vector<node_t > &nodes, string &attribute, unsigned int &attr_depth) const;
		bool NodeCompare(node_iter_t iter1, node_iter_t end1, node_iter_t iter2, node_iter_t end2);

	protected:
	
	private:
		JGeometryXML();
		
		string xmlfile;
		bool valid_xmlfile;

#if HAVE_XERCES
		xercesc::DOMBuilder *parser;
		xercesc::DOMDocument *doc;
		
		void AddNodeToList(xercesc::DOMNode* start, string start_path, vector<string> &xpaths, JGeometry::ATTR_LEVEL_t level);
		xercesc::DOMNode* FindNode(string xpath, string &attribute);
		xercesc::DOMNode* SearchTree(xercesc::DOMNode* current_node, unsigned int depth, vector<pair<string, map<string,string> > > &nodes, unsigned int attr_depth);

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

} // Close JANA namespace


#endif // _JGeometryXML_

