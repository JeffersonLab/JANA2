
// Copyright 2008-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: David Lawrence

#pragma once
#include <iostream>

#include <JANA/JLogger.h>
#include <JANA/Geometry/JGeometry.h>
#include <JANA/Calibrations/JCalibration.h>
#include <JANA/JVersion.h>


#if JANA2_HAVE_XERCES
#if !defined(__CINT__) && !defined(__CLING__)
// XERCES3
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#else // __CINT__  __CLING__
namespace xercesc{
class xercesc::DOMBuilder;
class xercesc::DOMDocument;
class xercesc::DOMNode;
class xercesc::DOMErrorHandler;
class xercesc::DOMError;
}
#endif // __CINT__  __CLING__
#endif // JANA2_HAVE_XERCES


class JGeometryXML:public JGeometry{
    public:

        typedef pair<string, map<string,string> > node_t;
        typedef vector<node_t>::iterator node_iter_t;

                            JGeometryXML(string url, int run, string context="default");
                            void Init(string xmlfile, string xml);

                    virtual ~JGeometryXML();
        virtual const char* className(void){return static_className();}
         static const char* static_className(void){return "JGeometryXML";}

#if JANA2_HAVE_XERCES
                       void MapNodeNames(xercesc::DOMNode *current_node);
#endif  // JANA2_HAVE_XERCES
                       bool Get(string xpath, string &sval);
                       bool Get(string xpath, map<string, string> &svals);
                       bool GetMultiple(string xpath, vector<string> &vsval);
                       bool GetMultiple(string xpath, vector<map<string, string> >&vsvals);
                       void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level, const string &filter="");
                     string GetChecksum(void) const {return md5_checksum;}

                       void ParseXPath(string xpath, vector<node_t > &nodes, string &attribute, unsigned int &attr_depth) const;
                       bool NodeCompare(node_iter_t iter1, node_iter_t end1, node_iter_t iter2, node_iter_t end2);


    private:
        JGeometryXML();

    protected:
        string xmlfile;
        bool valid_xmlfile;
        JCalibration *jcalib;
        string md5_checksum;
        map<string, string> found_xpaths; // used to store xpaths already found to speed up subsequent requests
        pthread_mutex_t found_xpaths_mutex;
#if JANA2_HAVE_XERCES
        map<xercesc::DOMNode*, string> node_names;
#endif  // JANA2_HAVE_XERCES

#if JANA2_HAVE_XERCES

        class SearchParameters{
            public:
                // Inputs
                vector<node_t> nodes;		// parsed xpath (see ParseXPath() )
                string attribute_name;		// Name of attribute of interest
                unsigned int attr_depth;	// index of nodes vector containing attribute of interest
                unsigned int max_values;	// return up to max_values attributes (0 means return all)

                // Outputs
                std::multimap<xercesc::DOMNode*, string> attributes; // DOMnode* is deepest level matched to xpath, string is value of specified attribute

                // Recursion variables (used to pass state through recursive calls)
                unsigned int depth;		// keeps track of depth in nodes tree
                string attr_value;		// value of attribute of interest in current branch
                xercesc::DOMNode *current_node;	// current DOMnode being searched

                void SearchTree(map<xercesc::DOMNode*, string> &node_names);	// Returns true if a matching node was found
        };


      xercesc::XercesDOMParser *parser;
      xercesc::DOMDocument *doc;

        void AddNodeToList(xercesc::DOMNode* start, string start_path, vector<string> &xpaths, JGeometry::ATTR_LEVEL_t level);
        //xercesc::DOMNode* FindNode(string xpath, string &attribute, xercesc::DOMNode *after_node=NULL);
        //xercesc::DOMNode* SearchTree(xercesc::DOMNode* current_node, unsigned int depth, vector<pair<string, map<string,string> > > &nodes, unsigned int attr_depth, bool find_all=false, vector<xercesc::DOMNode*> *dom_nodes=NULL);

        void FindAttributeValues(string &xpath, std::multimap<xercesc::DOMNode*, string> &attributes, unsigned int max_values=0);
        static void GetAttributes(xercesc::DOMNode* node, map<string,string> &attributes);

        // Error handler callback class
   class ErrorHandler : public xercesc::ErrorHandler
   {
            public:
                 //  Constructors and Destructor
                 ErrorHandler(){}
                 ~ErrorHandler(){}
                 bool handleError(const xercesc::DOMError& /*domError*/){jerr<<"Got Error!!"<<std::endl; return false;}
             void resetErrors(){}
      
            // Purely virtual methods
      void warning(const xercesc::SAXParseException& /*exc*/){}
      void error(const xercesc::SAXParseException& /*exc*/){}
      void fatalError(const xercesc::SAXParseException& /*exc*/){}

            private :
                 //  Unimplemented constructors and operators
                 ErrorHandler(const ErrorHandler&);
                 void operator=(const ErrorHandler&);
        };

    // A simple entity resolver to keep track of files being
    // included from the top-level XML file so a full MD5 sum
    // can be made
    class EntityResolver : public xercesc::EntityResolver
    {
        public:
            EntityResolver(const std::string &xmlFile, JCalibration *jcalib);
            ~EntityResolver();
            xercesc::InputSource* resolveEntity(const XMLCh* const publicId, const XMLCh* const systemId);


            std::vector<std::string> GetXMLFilenames(void);
            std::string GetMD5_checksum(void);

        private:
            std::vector<std::string> xml_filenames;
            std::vector<std::string> xml_content;
            std::string path;
            JCalibration *jcalib;
            bool PRINT_CHECKSUM_INPUT_FILES;
    };
#endif  // JANA2_HAVE_XERCES

};

// The following is here just so we can use ROOT's THtml class to generate documentation.
#ifdef G__DICTIONARY
typedef JGeometryXML::node_t node_t;
#endif


