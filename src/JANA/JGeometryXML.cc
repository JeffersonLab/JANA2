// $Id$
//
//    File: JGeometryXML.cc
// Created: Tues Jan 15, 2008
// Creator: davidl
//

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include "JGeometryXML.h"
#include "JParameterManager.h"
using namespace jana;

#if HAVE_XERCES

#if XERCES3
// XERCES 3

#else
// XERCES 2
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMXPathResult.hpp>

#endif

using namespace xercesc;
#endif


//---------------------------------
// JGeometryXML    (Constructor)
//---------------------------------
JGeometryXML::JGeometryXML(string url, int run, string context):JGeometry(url,run,context)
{
	/// File URL should be of form:
	/// file:///path-to-xmlfile
	/// If multiple XML files are required, then this should point to a
	/// top level file that includes all of the others.
	
	// Initialize our flag until we confirm the URL points to a valid XML file
	valid_xmlfile = false;
	md5_checksum = "";
	
	// First, check that the URL even starts with "xmlfile://"
	if(url.substr(0, 10)!=string("xmlfile://")){
		_DBG_<<"Poorly formed URL. Should start with \"xmlfile://\"."<<endl;
		_DBG_<<"URL:"<<url<<endl;
		_DBG_<<"(Try setting you JANA_GEOMETRY_URL environment variable.)"<<endl;
		return;
	}
	
	// Fill basedir with path to directory.
	xmlfile = url;
	xmlfile.replace(0,10,string("")); // wipe out "xmlfile://" part
	
	// Try and open top-level XML file to see if it exists and is readable
	ifstream f(xmlfile.c_str());
	if(!f.is_open()){
		_DBG_<<"Unable to open \""<<xmlfile<<"\"! Geometry not info not available!!"<<endl;
#if HAVE_XERCES
		parser = NULL;
		doc = NULL;
#endif
		run_min = run_max = -1;
		return;
	}else{
		f.close();
	}
	
	// Among other things, this should contain the range of runs for which
	// this calibration is valid. For now, just set them all to run_requested.
	run_min = run_max = run_found = GetRunRequested();

#if !HAVE_XERCES
	jerr<<endl;
	jerr<<"This JANA library was compiled without XERCESC support. To enable"<<endl;
	jerr<<"XERCESC, install it on your system and set your XERCESCROOT enviro."<<endl;
	jerr<<"variable to point to it. Then, recompile and install JANA."<<endl;
	jerr<<endl;
#else
	// Initialize XERCES system
	XMLPlatformUtils::Initialize();
	
	// Instantiate the DOM parser.
#if XERCES3
	parser = new XercesDOMParser();
	parser->setValidationScheme(XercesDOMParser::Val_Always);
	parser->setValidationSchemaFullChecking(true);
	parser->setDoSchema(true);
	parser->setDoNamespaces(true);    // optional
#else
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
	parser = ((DOMImplementationLS*)impl)->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
#endif

	// Create an error handler and install it
	JGeometryXML::ErrorHandler errorHandler;
	parser->setErrorHandler(&errorHandler);
	
	// EntityResolver is used to keep track of XML filenames so a full MD5 checksum can be made
	EntityResolver myEntityResolver(xmlfile);
	parser->setEntityResolver(&myEntityResolver);
	
	// Read in the XML and parse it
	parser->resetDocumentPool();
#if XERCES3
	parser->parse(xmlfile.c_str());
	md5_checksum = myEntityResolver.GetMD5_checksum();
	doc = parser->getDocument();
#else
	doc = parser->parseURI(xmlfile.c_str());
#endif
   
	valid_xmlfile = true;
	
	// Initialize found_xpaths_mutex
	pthread_mutex_init(&found_xpaths_mutex, NULL);
	
	// Make map of node names to speed up code in SearchTree later
	MapNodeNames(doc);
	
#endif  // !HAVE_XERCES
}

//---------------------------------
// ~JGeometryXML    (Destructor)
//---------------------------------
JGeometryXML::~JGeometryXML()
{
#if HAVE_XERCES
	// Release parser and delete any memory it allocated
	if(valid_xmlfile){
		//parser->release(); // This seems to be causing seg. faults so it is commented out.
	
		// Shutdown XERCES
		XMLPlatformUtils::Terminate();
	}
#endif
}

#if HAVE_XERCES
//---------------------------------
// MapNodeNames
//---------------------------------
void JGeometryXML::MapNodeNames(xercesc::DOMNode *current_node)
{
	// Record the current node's name
	char *tmp = XMLString::transcode(current_node->getNodeName());
	node_names[current_node] = string(tmp);
	XMLString::release(&tmp);

	// Loop over children of this node and recall ourselves to map their names
	for(DOMNode *child = current_node->getFirstChild(); child != 0; child=child->getNextSibling()){
		current_node = child;
		MapNodeNames(current_node); // attributes are automatically added as the tree is searched
	}
}
#endif // HAVE_XERCES

//---------------------------------
// Get
//---------------------------------
bool JGeometryXML::Get(string xpath, string &sval)
{
	/// Get the value of the attribute pointed to by the specified xpath
	/// and attribute by searching the XML DOM tree. Only the first matching
	/// occurance will be returned. The value of xpath may contain restrictions
	/// on the attributes anywhere along the node path via the XPATH 1.0
	/// specification.

	if(!valid_xmlfile){sval=""; return false;}
	

	// Look to see if we have already found the requested xpath.
	// doing this speeds up startup when using many threads
	pthread_mutex_lock(&found_xpaths_mutex);
	map<string, string>::iterator iter = found_xpaths.find(xpath);
	if(iter != found_xpaths.end()){
		sval = iter->second;
		pthread_mutex_unlock(&found_xpaths_mutex);
		return true;
	}
	
	// It is tempting here to unlock the found_xpaths_mutex
	// mutex, but doing so actually slows things down. This 
	// is because of 2 things:
	//
	//   1. Xerces will lock its own mutex anyway so the
	//      following code block will still run serially
	//
	//   2. Keeping the mutex locked blocks all other threads
	//      at the point above before checking if the
	//      result is cached. They therefore benefit from
	//      the cache entry once the mutex is finally released
	//      below.
	
#if HAVE_XERCES
	
	// XERCES locks its own mutex which causes horrible problems if the thread
	// is canceled while it has the lock. Disable cancelibility while here.
	int oldstate, oldtype;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	// NOTE: If this throws an exception, then we'll leave
	// this routine without unlocking found_xpaths_mutex !
	multimap<xercesc::DOMNode*, string> attributes;
	FindAttributeValues(xpath, attributes, 1);
	
	// If we found the attribute, copy it to users string
	if(attributes.size()>0){
		sval = (attributes.begin())->second;

		// Cache the result for use in subsequent calls
		found_xpaths[xpath] = sval;
	}

	// Unlock the found_xpaths_mutex mutex
	pthread_mutex_unlock(&found_xpaths_mutex);
	
	pthread_setcancelstate(oldstate, NULL);
	pthread_setcanceltype(oldtype, NULL);

	if(attributes.size()>0)return true; // return true to say we found it

#endif

	_DBG_<<"Node or attribute not found for xpath \""<<xpath<<"\"."<<endl;

	// Looks like we failed to find the requested item. Let the caller know.
	return false;
}

//---------------------------------
// Get
//---------------------------------
bool JGeometryXML::Get(string xpath, map<string, string> &svals)
{
	/// Get all of the attribute names and values for the specified xpath.
	/// Only the first matching
	/// occurance will be returned. The value of xpath may contain restrictions
	/// on the attributes anywhere along the node path via the XPATH 1.0
	/// specification.


	// Clear out anything that might already be in the svals container
	svals.clear();

	if(!valid_xmlfile)return false;

#if HAVE_XERCES
	
	// XERCES locks its own mutex which causes horrible problems if the thread
	// is canceled while it has the lock. Disable cancelibility while here.
	int oldstate, oldtype;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	multimap<xercesc::DOMNode*, string> attributes;
	FindAttributeValues(xpath, attributes, 1);
	
	// If we found the node, get the attribute list
	if(attributes.size()>0)GetAttributes((attributes.begin())->first, svals);

	pthread_setcancelstate(oldstate, NULL);
	pthread_setcanceltype(oldtype, NULL);

	if(attributes.size()>0)return true; // return true to say we found it

#endif

	_DBG_<<"Node or attribute not found for xpath \""<<xpath<<"\"."<<endl;

	// Looks like we failed to find the requested item. Let the caller know.
	return false;
}

//---------------------------------
// GetMultiple
//---------------------------------
bool JGeometryXML::GetMultiple(string xpath, vector<string> &vsval)
{
	/// Get the value of the attribute pointed to by the specified xpath
	/// and attribute by searching the XML DOM tree. All matching
	/// occurances will be returned. The value of xpath may contain restrictions
	/// on the attributes anywhere along the node path.

	vsval.clear();

	if(!valid_xmlfile){return false;}

#if HAVE_XERCES

	// XERCES locks its own mutex which causes horrible problems if the thread
	// is canceled while it has the lock. Disable cancelibility while here.
	int oldstate, oldtype;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	multimap<xercesc::DOMNode*, string> attributes;
	FindAttributeValues(xpath, attributes, 0);
	
	multimap<xercesc::DOMNode*, string>::iterator iter = attributes.begin();
	for(; iter!=attributes.end(); iter++){
		vsval.push_back(iter->second);
	}

	pthread_setcancelstate(oldstate, NULL);
	pthread_setcanceltype(oldtype, NULL);

#endif

	// Looks like we failed to find the requested item. Let the caller know.
	return vsval.size()>0;
}

//---------------------------------
// GetMultiple
//---------------------------------
bool JGeometryXML::GetMultiple(string xpath, vector<map<string, string> >&vsvals)
{
	/// Get the value of the attribute pointed to by the specified xpath
	/// and attribute by searching the XML DOM tree. All matching
	/// occurances will be returned. The value of xpath may contain restrictions
	/// on the attributes anywhere along the node path.

	vsvals.clear();

	if(!valid_xmlfile){return false;}

#if HAVE_XERCES

	// XERCES locks its own mutex which causes horrible problems if the thread
	// is canceled while it has the lock. Disable cancelibility while here.
	int oldstate, oldtype;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	multimap<xercesc::DOMNode*, string> attributes;
	FindAttributeValues(xpath, attributes, 0);
	
	multimap<xercesc::DOMNode*, string>::iterator iter = attributes.begin();
	for(; iter!=attributes.end(); iter++){

		DOMNode *node = iter->first;
		if(!node)continue;

		map<string, string> svals;
		GetAttributes(node, svals);
		vsvals.push_back(svals);
	}

	pthread_setcancelstate(oldstate, NULL);
	pthread_setcanceltype(oldtype, NULL);

#endif

	// Looks like we failed to find the requested item. Let the caller know.
	return vsvals.size()>0;
}

//---------------------------------
// GetXPaths
//---------------------------------
void JGeometryXML::GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level, const string &filter)
{
	/// Get all of the xpaths associated with the current geometry. Optionally
	/// append the attributes according to the value of level. (See JGeometry.h
	/// for valid values).
	/// If a non-empty string is passed in for "filter" then it is
	/// used to match xpaths that should be kept. If no filter is specified
	/// (i.e. an empty string) then all xpaths are returned.

	if(!valid_xmlfile){xpaths.clear(); return;}

#if HAVE_XERCES
	AddNodeToList((DOMNode*)doc->getDocumentElement(), "", xpaths, level);
#endif // XERCESC

	// If no filter is specified then return now.
	if(filter=="")return;
	
	// Parse the filter xpath
	vector<node_t> filter_nodes;
	string dummy_str;
	unsigned int dummy_int;
	ParseXPath(filter, filter_nodes, dummy_str, dummy_int);
	
	// We need at least one node on the filter to compare to
	if(filter_nodes.size()==0)return;

	// Loop over all xpaths
	vector<string> xpaths_to_keep;
	for(unsigned int i=0; i<xpaths.size(); i++){
		// Parse this xpath
		vector<node_t> nodes;
		ParseXPath(xpaths[i], nodes, dummy_str, dummy_int);
		
		// Loop over nodes of this xpath
		vector<node_t>::iterator iter;
		for(iter=nodes.begin(); iter!=nodes.end(); iter++){
			if(NodeCompare(filter_nodes.begin(), filter_nodes.end(), iter, nodes.end()))xpaths_to_keep.push_back(xpaths[i]);
		}
	}
	
	xpaths = xpaths_to_keep;
}

//---------------------------------
// NodeCompare
//---------------------------------
bool JGeometryXML::NodeCompare(node_iter_t iter1, node_iter_t end1, node_iter_t iter2, node_iter_t end2)
{
	/// Loop over nodes starting at iter1 and iter2 through end1 and end2
	/// to see if they match. The number of nodes and their names must
	/// match as well as the attributes. The attributes themselves are
	/// matched in the following way: All attributes appearing in the
	/// iter1 to end1 list must also appear in the iter2 to end2 list.
	/// The reverse however, need not be true. Furthermore, if the
	/// attribute value in the first list is an empty string, then the
	/// corresponding attribute value in the second list can be anything.
	/// Otherwise, they must be an exact match.

	// Loop over nodes in both lists simultaneously
	for(; iter1!=end1; iter1++, iter2++){
		// If we hit the end of the second iterator before the first
		// then they must not match.
		if(iter2==end2)return false;
		
		// Check node names
		if(iter1->first!="*")
			if(iter1->first != iter2->first)return false;

		// Loop over attributes for iter1
		map<string,string> &attr1 = iter1->second;
		map<string,string> &attr2 = iter2->second;
		map<string,string>::iterator attr_iter1 = attr1.begin();
		for(; attr_iter1!= attr1.end(); attr_iter1++){

			// Check if this attribute exists
			map<string,string>::iterator attr_iter2 = attr2.find(attr_iter1->first);
			if(attr_iter2==attr2.end())return false;
			// Attribute exists in both lists. If non-emtpy string in list 1,
			// then verify they match
			if(attr_iter1->second!=""){
				if(attr_iter1->second!=attr_iter2->second)return false;
			}
		}
	}

	return true;
}


//---------------------------------
// ParseXPath
//---------------------------------
void JGeometryXML::ParseXPath(string xpath, vector<pair<string, map<string,string> > > &nodes, string &attribute, unsigned int &attr_depth) const
{
	/// Parse a xpath string to obtain a list of node names and for each,
	/// a map of the attributes and their (optional) values. This is a
	/// very poor man's substitute for a real XPATH parser. It only works
	/// on strings that are of a form such as:
	///
	///  /HDDS/ForwardDC_s[@name='abc' and @id=143]/section[@name]/tubs
	///
	/// where the "and"s are completely ignored. The return vector has 
	/// pair objects for which the
	/// node names are the keys(first) and the values are a map containing the 
	/// attributes specified for that node(second). It is done this way so that
	/// the order of the node names may be maintained in the vector. (Otherwise,
	/// one might just use an STL map container rather than a vector of pairs).
	/// The attribute maps each have
	/// the attribute name as the key and the attribute value as the value.
	/// If no attribute value is specified (as for the section node in
	/// the above example) then the value is an empty string.
	///
	/// This does no checking that the format is valid, even for this
	/// very limited syntax. What can I say, it's a poor man's parser ;).

	// Clear attribute string
	attribute = "";
	attr_depth = 0xFFFFFFFF;

	// First, split path up into strings using "/" as a delimiter
	vector<string> sections;
	string::size_type lastPos = xpath.find_first_not_of("/", 0);
	do{
		string::size_type pos = xpath.find_first_of("/", lastPos);
		if(pos == string::npos)break;
		
		sections.push_back(xpath.substr(lastPos, pos-lastPos));
		
		lastPos = pos+1;
	}while(lastPos!=string::npos && lastPos<xpath.size());
	sections.push_back(xpath.substr(lastPos, xpath.length()-lastPos));

	// Now split each section into the node name and the attributes list
	for(unsigned int i=0; i<sections.size(); i++){
		string &str = sections[i];
		
		// Find the node name
		string::size_type pos_node_end = str.find_first_of("[", 0);
		if(pos_node_end==string::npos)pos_node_end = str.length();
		
		// If the node name is prefaced with a namespace, then discard it
		string::size_type pos_node_start = str.find_first_of(":", 0);
		if(pos_node_start==string::npos || pos_node_start>pos_node_end){
			pos_node_start=0;
		}else{
			pos_node_start++;
		}
		if(str[pos_node_start]=='@')pos_node_start=pos_node_end;
		string nodeName = str.substr(pos_node_start, pos_node_end-pos_node_start);

		// Pull out all of the attributes
		map<string,string> qualifiers;
		lastPos = str.find_first_of("@", 0);
		while(lastPos!=string::npos){
			lastPos++; // jump past "@"
			string attr="";
			string val="";
			string::size_type pos_equals = str.find_first_of("=", lastPos);
			string::size_type next_attr = str.find_first_of("@", lastPos);
			if(pos_equals!=string::npos && (next_attr>pos_equals || next_attr==string::npos)){
				// attribute has "=" in it
				attr = str.substr(lastPos, pos_equals-lastPos);
				string::size_type pos_end = str.find_first_of(" ", pos_equals);
				if(pos_end==string::npos)pos_end = str.size();
				
				// For values containing white space, we need to look for both
				// the opening and closing quotes.
				string::size_type pos_quote = str.find_first_of("'", lastPos);
				if(pos_quote!=string::npos){
					pos_quote = str.find_first_of("'", pos_quote+1);
					if(pos_quote!=string::npos && pos_quote>pos_end)pos_end = pos_quote;
				}
				
				// At this point, the substring in pos_equals+1 to pos_end
				// may contain quotes and/or a closing bracket "]". We need to 
				// identify these and clip them if needed.
				string::size_type pos_start = pos_equals+1;
				if(str[pos_end-1]==']')pos_end--;
				if(str[pos_end-1]=='\'')pos_end--;
				if(str[pos_end-1]=='\"')pos_end--;
				if(str[pos_start]=='\'')pos_start++;
				if(str[pos_start]=='\"')pos_start++;
				val = str.substr(pos_start, pos_end - pos_start);
			}else if(pos_equals==string::npos){
				// attribute exists, but does not have "=" in it
				string::size_type pos_end = str.find_first_of(" ", lastPos);
				if(pos_end==string::npos)pos_end = str.length();
				if(str[pos_end-1]==']')pos_end--;
				attr = str.substr(lastPos, pos_end-lastPos);
			}else{
				// no more attribute found
				break;
			}
			if(attr!=""){
				qualifiers[attr] = val;
				lastPos = str.find_first_of("@", lastPos);
			}else{
				break;
			}
		}
		
		// If this is the last section, it could be specifying only the desired
		// attribute and not actually a whole other node. Consider the example:
		// '//hdds:element[@name="Antimony"]/@a'
		// where the last "@a" means they want the "a" attribute of the 
		// "element" node. In these cases, we want to add the final attribute
		// to the qualifiers list of the previously found node and NOT
		// create a a whole other entry in the nodes map.
		if(nodeName=="" && i>0){
			if(qualifiers.size()==1){
				if(attribute!=""){
					// If we get here then it looks like we have already found a
					// "lone attribute" that is the target of the xpath query.
					// This can happen with an xpath that looks like this:
					//   //mynode/@id/hello/@name
					// This is an error in the xpath so we notify the user
					// but then go ahead and replace the attribute with the
					// current one.
					_DBG_<<"Multiple attribute targets specified in \""<<xpath<<"\""<<endl;
				}
				attribute = qualifiers.begin()->first;
				attr_depth = nodes.size()-1;
				map<string,string> &last_qualifiers = nodes[i-1].second;
				last_qualifiers[attribute] = "";
			}
		}else{
			// Add this node to the list
			pair<string, map<string,string> > node(nodeName, qualifiers);
			nodes.push_back(node); // This is needed to maintain the order
		}
	}

}

#if HAVE_XERCES
//---------------------------------
// AddNodeToList
//---------------------------------
void JGeometryXML::AddNodeToList(xercesc::DOMNode* start, string start_path, vector<string> &xpaths, ATTR_LEVEL_t level)
{
	/// This calls itself recursively to walk the DOM tree and find all xpaths 
	/// corresponding to all of the nodes. It optionally will append
	/// the attributes to the nodes in a form compatible with XPATH 1.0
	/// such that they can be used (though usually one would want to
	/// edit it slightly) as the xpath argument to one of the Get methods.

	// Get name of this node
	string nodeName = node_names.at(start);
	//char* tmp = XMLString::transcode(start->getNodeName());
	//string nodeName = tmp;
	//XMLString::release(&tmp);
	
	// Ignore nodes that start with a "#"
	if(nodeName[0] == '#')return;
	
	// Create map of all attributes of this node
	map<string,string> attributes;
	if(start->hasAttributes()) {
		DOMNamedNodeMap *pAttributes = start->getAttributes();
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i) {
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			char *attrName = XMLString::transcode(pAttributeNode->getName());
			char *attrValue = XMLString::transcode(pAttributeNode->getValue());
			attributes[attrName] = attrValue;
			XMLString::release(&attrName);
			XMLString::release(&attrValue);
		}
	}
	
	// Create attribute qualifier string
	string attr_qualifiers = "";
	if(level!=attr_level_none && attributes.size()>0){
		attr_qualifiers += "[";
		map<string,string>::iterator iter = attributes.begin();
		for(int i=0; iter!=attributes.end(); i++, iter++){
			if(i>0)attr_qualifiers += " and ";
			attr_qualifiers += "@"+iter->first+"='"+iter->second+"'";
		}
		attr_qualifiers += "]";
	}
	
	// Create xpath for this node
	string xpath = start_path + "/" + nodeName;
	xpaths.push_back(xpath + attr_qualifiers);

	// Add any child nodes of this node
	//string current_path = start_path + "/" + nodeName;
	for (DOMNode *child = start->getFirstChild(); child != 0; child=child->getNextSibling()){
		AddNodeToList(child, xpath + (level==attr_level_all ? attr_qualifiers:""), xpaths, level);
	}
}

//---------------------------------
// FindAttributeValues
//---------------------------------
void JGeometryXML::FindAttributeValues(string &xpath, multimap<DOMNode*, string> &attributes, unsigned int max_values)
{
	/// Search the DOM tree and find attributes matching the given xpath
	
	// First, parse the string to get node names and attribute qualifiers
	SearchParameters sp;
	ParseXPath(xpath, sp.nodes, sp.attribute_name, sp.attr_depth);
	
	// Fill in starting values for recursive search
	sp.max_values = max_values;
	sp.depth = 0;
	sp.attr_value = "";
	sp.current_node = doc;
	
	// Do the search. Results are returned in sp.
	sp.SearchTree(node_names);
	
	// Copy search results into user provided container
	attributes = sp.attributes;
}

//---------------------------------
// SearchTree
//---------------------------------
void JGeometryXML::SearchParameters::SearchTree(map<xercesc::DOMNode*, string> &node_names)
{
	/// This is a reentrant routine that recursively calls itself while walking the
	/// DOM tree, looking for a path that matches the xpath already parsed and
	/// passed to us via the "nodes" variable. This examines the node specified
	/// by "current_node" and if needed, all of it's children. The value of "depth"
	/// specifies which element of the nodes vector we are looking for. On the initial
	/// call to this routine, depth will be 0 signifying that we are trying to match
	/// the first node. Note that the first level specified in the xpath may not be
	/// at the root of the DOM tree so we may recall ourselves at several levels
	/// with depth=0 as we try to find the starting point in the DOM tree.
	///
	/// The "attr_depth" value specifies which element of the "nodes" vector has
	/// the atribute of interest. This is needed since the attribute of interest may
	/// reside at any level in the xpath (not necessarily at the end).
	///
	/// The "after_node" value is used when looking for multiple nodes that satisfy
	/// the same xpath. If this is NULL, then the first matching node encountered 
	/// is returned. If it is non-NULL, then matching nodes are skipped until
	/// the one specified by "after_node" is found. At that point, after_node is
	/// set to NULL and the search continued so that the next matching node will be
	/// returned. This means for each matching instance, the tree is re-searched 
	/// from the begining to look for the additional matches which is not very
	/// efficient, but it is what it is.


	// First, make sure "depth" isn't deeper than our nodes map!
	if(depth>=nodes.size())return;

	// Get node name in usable format
	const string &nodeName = node_names.at(current_node);
	//char *tmp = XMLString::transcode(current_node->getNodeName());
	//string nodeName = tmp;
	//XMLString::release(&tmp);

	// Check if the name of this node matches the one we're looking for
	// (specified by depth). 
	if(nodeName!=nodes[depth].first && nodes[depth].first!="" && nodes[depth].first!="*"){
		
		// OK, this node is not listed in the xpath explicitly, but it may still
		// be an ancestor of the desired xpath. We loop over all children in order
		// to check if the specified xpath exists in any of them.

		// If depth is not 0, then we know we're already part-way into
		// the tree. Return now since this is not the right branch.
		if(depth!=0)return;
		
		// At this point, we may have just not come across the first node
		// specified in our nodes map. Try each of our children
		// Loop over children and recall ourselves for each of them
		for(DOMNode *child = current_node->getFirstChild(); child != 0; child=child->getNextSibling()){
			current_node = child;
			SearchTree(node_names); // attributes are automatically added as the tree is searched
			if(max_values>0 && attributes.size()>=max_values)return; // bail if max num. of attributes found
		}
		
		// If we get here then we have searched all branches descending from this
		// node and any matches that were found have already been added to the 
		// attributes list. Return now since there is nothing else to do for the 
		// current node.
		return;
	}

	// Get list of attributes for this node
	map<string,string> my_attributes;
	JGeometryXML::GetAttributes(current_node, my_attributes);

	// If we get here then we are at the "depth"-th node in the list. Check all
	// attribute qualifiers for this node (if any).
	unsigned int Npassed = 0;
	map<string, string> &qualifiers = nodes[depth].second;
	string my_attr_value = "";
	map<string, string>::iterator iter;
	for(iter = qualifiers.begin(); iter!=qualifiers.end(); iter++){
		const string &attr = iter->first;
		const string &val = iter->second;

		// Loop over attributes of this node
		map<string, string>::iterator attr_iter;
		for(attr_iter = my_attributes.begin(); attr_iter!=my_attributes.end(); attr_iter++){
			const string &my_attr = attr_iter->first;
			const string &my_val = attr_iter->second;
			
			// If this is the attribute of interest, remember it so we can copy into attr_value below
			if(attr==attribute_name)my_attr_value = my_val;

			// If this matches a qualifier, increment Npassed counter
			if(attr == my_attr){
				if(val=="" || val==my_val)Npassed++;
				break;
			}
		}
	}

	// If we didn't pass all of the attribute tests, then return now since this does not match the xpath
	if(Npassed != qualifiers.size())return; 
	
	// If we get here AND we're at attr_depth then temporarily record the value of the
	// attribute of interest so when we get to the end of the xpath we can add it
	// to the list of attributes found.
	if(depth==attr_depth)attr_value = my_attr_value;

	// Check if we have found the final node at the end of the xpath
	if(depth==nodes.size()-1){
		// At this point, we have found a node that completely matches all node names
		// and qualifiers. Add this to the list of attributes
		attributes.insert(pair<DOMNode*, string>(current_node, attr_value));

		// No further searching of this node is needed.
		return;
	}

	// At this point, we have verified that the node names and all of
	// the qualifiers for each of them up to and including this node
	// are correct. Now we need to loop over this node's children and
	// have them check against the next level.
	DOMNode* save_current = current_node;
	depth++;
	for (DOMNode *child = current_node->getFirstChild(); child != 0; child=child->getNextSibling()){
		current_node = child;
		SearchTree(node_names);
	}
	depth--;
	current_node = save_current;
	
	return;
}

//---------------------------------
// GetAttributes
//---------------------------------
void JGeometryXML::GetAttributes(xercesc::DOMNode* node, map<string,string> &attributes)
{
	attributes.clear();

	if(!node->hasAttributes())return;
	
	// Loop over attributes of this node
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	int nSize = pAttributes->getLength();
	for(int i=0;i<nSize;++i) {
		DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
		char *tmp1 = XMLString::transcode(pAttributeNode->getName());
		char *tmp2 = XMLString::transcode(pAttributeNode->getValue());
		attributes[tmp1] = tmp2;
		XMLString::release(&tmp1);
		XMLString::release(&tmp2);
	}
}

//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

//----------------------------------
// EntityResolver (constructor)
//----------------------------------
JGeometryXML::EntityResolver::EntityResolver(const string &xmlFile)
{
	xml_filenames.push_back(xmlFile);
	
	string fname = xmlFile;
	size_t pos = fname.find_last_of('/');
	if(pos != string::npos){
		path = fname.substr(0,pos) + "/";
	}

	PRINT_CHECKSUM_INPUT_FILES = false;
	if(gPARMS){
		if(gPARMS->Exists("PRINT_CHECKSUM_INPUT_FILES")){
			gPARMS->GetParameter("PRINT_CHECKSUM_INPUT_FILES", PRINT_CHECKSUM_INPUT_FILES);
		}
	}
}

//----------------------------------
// EntityResolver (destructor)
//----------------------------------
JGeometryXML::EntityResolver::~EntityResolver()
{

}

//----------------------------------
// resolveEntity
//----------------------------------
#if XERCES3
xercesc::InputSource* JGeometryXML::EntityResolver::resolveEntity(const XMLCh* const publicId, const XMLCh* const systemId)
#else
xercesc::DOMInputSource* JGeometryXML::EntityResolver::resolveEntity(const XMLCh* const publicId, const XMLCh* const systemId, const XMLCh* const baseURI)
#endif
{
	/// This method gets called from the xerces parser each time it
	/// opens a file (except for the top-level file). For each of these,
	/// record the name of the file being opened, then just return NULL
	/// to have xerces handle opening the file in the normal way.

	// Do some backflips to get strings into std::string format
	std::string my_publicId = "";
	std::string my_systemId = "";
	if(publicId){
		char *my_publicId_ptr = xercesc::XMLString::transcode(publicId);
		my_publicId = my_publicId_ptr;
		xercesc::XMLString::release(&my_publicId_ptr);
	}
	if(systemId){
		char *my_systemId_ptr = xercesc::XMLString::transcode(systemId);
		my_systemId = my_systemId_ptr;
		xercesc::XMLString::release(&my_systemId_ptr);
	}
	//std::cerr<<"publicId="<<my_publicId<<"  systemId="<<my_systemId<<std::endl;

	// The systemId seems to be the one we want
	xml_filenames.push_back(path + my_systemId);

	return NULL; // have xerces handle this using its defaults
}

//----------------------------------
// GetXMLFilenames
//----------------------------------
std::vector<std::string> JGeometryXML::EntityResolver::GetXMLFilenames(void)
{
	return xml_filenames;
}

//----------------------------------
// GetMD5_checksum
//----------------------------------
std::string JGeometryXML::EntityResolver::GetMD5_checksum(void)
{
	/// This will calculate an MD5 checksum using all of the files currently
	/// in the list of XML files. To do this, it opens each file and reads it
	/// in, in its entirety, updating the checksum as it goes. The checksum is
	/// returned as a hexadecimal string.

	md5_state_t pms;
	md5_init(&pms);
	for(unsigned int i=0; i<xml_filenames.size(); i++){

		if(PRINT_CHECKSUM_INPUT_FILES){
			std::cerr<<".... Adding file to MD5 checksum : " << xml_filenames[i] << std::endl;
		}

		ifstream ifs(xml_filenames[i].c_str());
		if(!ifs.is_open())continue;

		// get length of file:
		ifs.seekg (0, ios::end);
		unsigned int length = ifs.tellg();
		ifs.seekg (0, ios::beg);

		// allocate memory:
		char *buff = new char [length];

		// read data as a block:
		ifs.read (buff,length);
		ifs.close();

		md5_append(&pms, (const md5_byte_t *)buff, length);

		delete[] buff;

		//std::cerr<<".... Adding file to MD5 checksum : " << xml_filenames[i] << "  (size=" << length << ")" << std::endl;
	}
	
	md5_byte_t digest[16];
	md5_finish(&pms, digest);
	
	char hex_output[16*2 + 1];
	for(int di = 0; di < 16; ++di) sprintf(hex_output + di * 2, "%02x", digest[di]);

	return hex_output;
}


#endif // XERCESC

