// $Id$
//
//    File: JGeometryXML.h
// Created: Tues Jan 15 2008
// Creator: davidl
//

#ifndef _JGeometryXML_
#define _JGeometryXML_

#include <iostream>

#include <JANA/jerror.h>
#include <JANA/JGeometry.h>
#include <JANA/jana_config.h>
#include <JANA/JStreamLog.h>


#if HAVE_XERCES
#ifndef __CINT__
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#else // __CINT__
namespace xercesc{
class xercesc::DOMBuilder;
class xercesc::DOMDocument;
class xercesc::DOMNode;
class xercesc::DOMErrorHandler;
class xercesc::DOMError;
}
#endif // __CINT__
#endif // HAVE_XERCES

// Place everything in JANA namespace
namespace jana{

class JGeometryXML:public JGeometry{
	public:

		typedef pair<string, map<string,string> > node_t;
		typedef vector<node_t>::iterator node_iter_t;
	
		JGeometryXML(string url, int run, string context="default");
		virtual ~JGeometryXML();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JGeometryXML";}
		
		bool Get(string xpath, string &sval);
		bool Get(string xpath, map<string, string> &svals);
		bool GetMultiple(string xpath, vector<string> &vsval);
		bool GetMultiple(string xpath, vector<map<string, string> >&vsvals);
		void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level, const string &filter="");

		void ParseXPath(string xpath, vector<node_t > &nodes, string &attribute, unsigned int &attr_depth) const;
		bool NodeCompare(node_iter_t iter1, node_iter_t end1, node_iter_t iter2, node_iter_t end2);

	protected:
	
	private:
		JGeometryXML();
		
		string xmlfile;
		bool valid_xmlfile;

#if HAVE_XERCES

		class SearchParameters{
			public:
				// Inputs
				vector<node_t> nodes;		// parsed xpath (see ParseXPath() )
				string attribute_name;		// Name of attribute of interest
				unsigned int attr_depth;	// index of nodes vector containing attribute of interest
				unsigned int max_values;	// return up to max_values attributes (0 means return all)
				
				// Outputs
				multimap<xercesc::DOMNode*, string> attributes; // DOMnode* is deepest level matched to xpath, string is value of specified attribute
				
				// Recursion variables (used to pass state through recursive calls)
				unsigned int depth;		// keeps track of depth in nodes tree
				string attr_value;		// value of attribute of interest in current branch
				xercesc::DOMNode *current_node;	// current DOMnode being searched

				void SearchTree(void);	// Returns true if a matching node was found
		};

		xercesc::DOMBuilder *parser;
		xercesc::DOMDocument *doc;
		
		void AddNodeToList(xercesc::DOMNode* start, string start_path, vector<string> &xpaths, JGeometry::ATTR_LEVEL_t level);
		//xercesc::DOMNode* FindNode(string xpath, string &attribute, xercesc::DOMNode *after_node=NULL);
		//xercesc::DOMNode* SearchTree(xercesc::DOMNode* current_node, unsigned int depth, vector<pair<string, map<string,string> > > &nodes, unsigned int attr_depth, bool find_all=false, vector<xercesc::DOMNode*> *dom_nodes=NULL);

		void FindAttributeValues(string &xpath, multimap<xercesc::DOMNode*, string> &attributes, unsigned int max_values=0);
		static void GetAttributes(xercesc::DOMNode* node, map<string,string> &attributes);

		// Error handler callback class
		class ErrorHandler : public xercesc::DOMErrorHandler
		{
			public:
				 //  Constructors and Destructor
				 ErrorHandler(){}
				 ~ErrorHandler(){}
				 bool handleError(const xercesc::DOMError& domError){jerr<<"Got Error!!"<<endl; return false;}
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

