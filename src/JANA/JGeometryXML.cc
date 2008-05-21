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
using namespace jana;

#if HAVE_XERCES
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

#ifndef HAVE_XERCES
	cout<<endl;
	cout<<"This JANA library was compiled without XERCESC support. To enable"<<endl;
	cout<<"XERCESC, install it on your system and set your XERCESCROOT enviro."<<endl;
	cout<<"variable to point to it. Then, recompile and install JANA."<<endl;
	cout<<endl;
#else
	// Initialize XERCES system
	XMLPlatformUtils::Initialize();
	
	// Instantiate the DOM parser.
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
	parser = ((DOMImplementationLS*)impl)->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);

	// Create an error handler and install it
	JGeometryXML::ErrorHandler errorHandler;
	parser->setErrorHandler(&errorHandler);
	
	// Read in the XML and parse it
	//cout<<"Parsing geometry starting from: \""<<xmlfile<<"\" ... "<<endl;
	parser->resetDocumentPool();
	doc = parser->parseURI(xmlfile.c_str());

	valid_xmlfile = true;
#endif
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

//---------------------------------
// Get
//---------------------------------
bool JGeometryXML::Get(string xpath, string &sval)
{
	/// Get the value of the attribute pointed to by the specified xpath
	/// and attribute by searching the XML DOM tree. Only the first matching
	/// occurance will be returned. The value of xpath may contain restrictions
	/// on the attributes anywhere along the node path via the XPATH 1.0
	/// specification as implemented by Xerces.

	if(!valid_xmlfile){sval=""; return false;}

#if HAVE_XERCES

	// Ideally we would use the XPath parser built into xerces to parse
	// our xpath string. However, the piss-poor documentation on this feature
	// has already cost me more time than it would take to write my own
	// (limited) parser with still no results or real clues as to what
	// I'm doing wrong. So, with great bitterness, I'm commenting this
	// out and just writing my own so I can move on to more important things.

	//XMLCh* xpath_xmlch = XMLString::transcode(xpath.c_str());
	//DOMNode *node=NULL;
	//try{
	//		node = (DOMNode*)doc->evaluate(xpath_xmlch, doc, NULL, DOMXPathResult::ANY_TYPE, NULL);
	//}catch(DOMException ex){
	//	char *mess = XMLString::transcode(ex.getMessage());
	//	cout<<"Exception: "<<mess<<endl;
	//	cout<<"xpath=\""<<XMLString::transcode(xpath_xmlch)<<"\""<<endl;
	//	XMLString::release(&mess);
	//}
	//XMLString::release(&xpath_xmlch);

	string attribute;
	DOMNode *node = FindNode(xpath, attribute);
	
	if(attribute==""){
		_DBG_<<"Get(string, string&) method called but no attribute specified in xpath."<<endl;
		_DBG_<<"The xpath string should end in \"/@attributeName\"."<<endl;
		return false;
	}

	if(node!=NULL && node->hasAttributes()){
		// We found the node! Loop over attributes looking for the one we want
		// get all the attributes of the node

		DOMNamedNodeMap *pAttributes = node->getAttributes();
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i) {
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			
			// Copy name/value into local variables and realease memory allocated by xerces
			char *attrName  = XMLString::transcode(pAttributeNode->getName());
			char *attrValue = XMLString::transcode(pAttributeNode->getValue());
			string name(attrName);
			string value(attrValue);
			XMLString::release(&attrName);
			XMLString::release(&attrValue);
			
			// Check if this is the attribute we're looking for
			if(name == attribute){
				sval = value;
				return true;
			}
		}
	}
#endif

	_DBG_<<"Node or attribute not found."<<endl;

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
	/// specification as implemented by Xerces.


	// Clear out anything that might already be in the svals container
	svals.clear();

	if(!valid_xmlfile){return false;}

#if HAVE_XERCES
	// Get the pointer to the node of interest.
	string attribute;
	DOMNode *node = FindNode(xpath, attribute);
	
	if(node!=NULL){
		// We found the node! Loop over attributes.
		if(node->hasAttributes()) {
			// get all the attributes of the node
			DOMNamedNodeMap *pAttributes = node->getAttributes();
			int nSize = pAttributes->getLength();
			for(int i=0;i<nSize;++i) {
				DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
				// Copy name/value into map, realeasing memory allocated by xerces
				char *attrName  = XMLString::transcode(pAttributeNode->getName());
				char *attrValue = XMLString::transcode(pAttributeNode->getValue());
				string name(attrName);
				string value(attrValue);
				XMLString::release(&attrName);
				XMLString::release(&attrValue);
				
				if(name==attribute || attribute=="")svals[name] = value;
			}
		}

		return true;
	}
#endif

	// Looks like we failed to find the requested item. Let the caller know.
	return false;
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
// FilterCompare
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
	char* tmp = XMLString::transcode(start->getNodeName());
	string nodeName = tmp;
	XMLString::release(&tmp);
	
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
// FindNode
//---------------------------------
DOMNode* JGeometryXML::FindNode(string xpath, string &attribute)
{
	/// Parse the xpath string into node names and attribute qualifiers.
	/// This is a *very* restricted form of the xpath syntax and done only
	/// to serve the purposes of this specific application at this time.
	/// The string "attribute" is used to return the name of the attribute
	/// if it is specified at the end of the xpath string. For example,
	///
	/// '//hdds:element[@name="Antimony"]/@a'
	///
	/// specifies that the attribute "a" of the element node is what is
	/// desired. By contrast,
	///
	/// '//hdds:element[@name="Antimony" and @a]'
	///
	/// specifies the node, but not a particular attribute. For these cases,
	/// an empty string is ("") is copied into the attribute variable.
	
	// First, parse the string to get node names and attribute qualifiers
	vector<pair<string, map<string,string> > > nodes;
	unsigned int attr_depth;
	ParseXPath(xpath, nodes, attribute, attr_depth);

#if 0	
	vector<pair<string, map<string,string> > >::iterator iter = nodes.begin();
	for(; iter!=nodes.end(); iter++){
		_DBG_<<"node \""<<iter->first<<"\": ";
		
		map<string,string> &qualifiers = iter->second;
		map<string,string>::iterator qiter = qualifiers.begin();
		for(; qiter!=qualifiers.end(); qiter++){
			_DBG_<<"\""<<qiter->first<<"\"=\""<<qiter->second<<"\" ";
		}
		
		cout<<endl;
	}
#endif

	// The only practical way to find the nodes now is to use a recursive
	// routine to walk the tree until it finds the correct node with all
	// of the correct attributes of the parent nodes along the way.
	DOMNode *node = SearchTree(doc, 0, nodes, attr_depth);

	return node;
}

//---------------------------------
// SearchTree
//---------------------------------
DOMNode* JGeometryXML::SearchTree(DOMNode* current_node, unsigned int depth, vector<pair<string, map<string,string> > > &nodes, unsigned int attr_depth)
{
	// First, make sure "depth" isn't deeper than our nodes map.
	if(depth>=nodes.size())return NULL;

	// Get node name in usable format
	char *tmp = XMLString::transcode(current_node->getNodeName());
	string nodeName = tmp;
	XMLString::release(&tmp);

	// Check if the name of this node matches the one we're looking for
	// (specified by depth). If not, then we may need to loop over this node's
	// children and recall ourselves. This is for the case when we are
	// first called and the depth=0 node may not be at the root of the 
	// tree.
	if(nodeName!=nodes[depth].first && nodes[depth].first!="" && nodes[depth].first!="*"){

		// If depth is not 0, then we know we're already part-way into
		// the tree. Return NULL now to signify this is not the right branch
		if(depth!=0)return NULL;
		
		// At this point, we may have just not come across the first node
		// specified in our nodes map. Try each of our children
		// Loop over children and recall ourselves for each of them
		for (DOMNode *child = current_node->getFirstChild(); child != 0; child=child->getNextSibling()){
			DOMNode *node = SearchTree(child, 0, nodes, attr_depth);
			if(node!=NULL){
				// Wow! it looks like we found it. Go ahead and return the
				// node pointer.
				return node;
			}
		}
		
		// Looks like we didn't find the node on this branch. Return NULL.
		return NULL;
	}

	// OK, We are at the "depth"-th node in our list. Check any attribute
	// qualifiers for this node.
	unsigned int Npassed = 0;
	map<string, string> &qualifiers = nodes[depth].second;
	map<string, string>::iterator iter = qualifiers.begin();
	for(; iter!=qualifiers.end(); iter++){
		const string &attr = iter->first;
		const string &val = iter->second;
		
		// Loop over attributes of this node
		if(current_node->hasAttributes()) {
			// get all the attributes of the node
			DOMNamedNodeMap *pAttributes = current_node->getAttributes();
			int nSize = pAttributes->getLength();
			for(int i=0;i<nSize;++i) {
				DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
				char *tmp1 = XMLString::transcode(pAttributeNode->getName());
				char *tmp2 = XMLString::transcode(pAttributeNode->getValue());
				string my_attr = tmp1;
				string my_val = tmp2;
				XMLString::release(&tmp1);
				XMLString::release(&tmp2);
				
				if(attr == my_attr){
					if(val=="" || val==my_val)Npassed++;
					break;
				}
			}
		}
	}
	
	// If we didn't pass all of the attribute tests, then return NULL now
	if(Npassed != qualifiers.size())return NULL;
	
	// Check if we have found the final node
	if(depth==nodes.size()-1)return current_node;
	
	// At this point, we have verified that the node names and all of
	// the qualifies for each of them up to and including this node
	// are correct. Now we need to loop over this node's children and
	// have them check against the next level.
	for (DOMNode *child = current_node->getFirstChild(); child != 0; child=child->getNextSibling()){
		DOMNode *node = SearchTree(child, depth+1, nodes, attr_depth);
		if(node!=NULL){
			return depth==attr_depth ? current_node:node;
		}
	}
	
	return NULL;
}


#endif // XERCESC

