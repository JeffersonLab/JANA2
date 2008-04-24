// $Id: JFactory_base.cc 1150 2005-08-02 12:23:03Z davidl $

#include <stdlib.h>
#include <iostream>
using namespace std;

#include "JFactory_base.h"
using namespace jana;


//-------------
// toString
//-------------
string JFactory_base::toString(void) const
{
	/// Return a string containing the data for all of the objects
	/// produced by this factory for the current event. Note that
	/// this does not actually activate the factory to create the
	/// objects if they don't already exist. One should call the
	/// Get() method first if they wish to ensure the factory has
	/// been activated.
	///
	/// If no objects exist, then an empty string is returned. If objects
	/// do exist, then the string returned will contain column names
	/// and a separator line.
	///
	/// The string is built using values obtained via JObject::toStrings().


	// Get data in the form of strings from the sub-class which knows 
	// the data type we are.
	vector<vector<pair<string,string> > > allitems;
	toStrings(allitems);
	if(allitems.size()==0)return string("");

	// Make reference to first map which we'll use to get header info
	vector<pair<string,string> > &h = allitems[0];
	if(h.size()==0)return string("");

	// Make list of column names and simultaneously capture the string lengths
	vector<unsigned int> colwidths;
	vector<string> headers;
	vector<pair<string,string> >::iterator hiter = h.begin();
	for(; hiter!=h.end(); hiter++){
		headers.push_back(hiter->first);
		colwidths.push_back(hiter->first.length()+2);
	}
	
	assert(headers.size()==colwidths.size());
	
	// Look at all elements to find the maximum width of each column
	for(unsigned int i=0; i<allitems.size(); i++){
		vector<pair<string,string> > &a = allitems[i];
		
		assert(a.size()==colwidths.size());
		
		for(unsigned int j=0; j<a.size(); j++){
			pair<string,string> &b = a[j];
			
			unsigned int len = b.second.length()+2;
			if(len>colwidths[j])colwidths[j] = len;
		}
	}
	
	stringstream ss;
	
	// Print header
	unsigned int header_width=0;
	for(unsigned int i=0; i<colwidths.size(); i++)header_width += colwidths[i];
	string header = string(header_width,' ');
	unsigned int pos=0;
	for(unsigned int i=0; i<colwidths.size(); i++){
		header.replace(pos+colwidths[i]-headers[i].length()-1, headers[i].length()+1, headers[i]+":");
		pos += colwidths[i];
	}
	ss<<header<<endl;
	
	ss<<string(header_width,'-')<<endl;
	
	// Print data
	for(unsigned int i=0; i<allitems.size(); i++){
		vector<pair<string,string> > &a = allitems[i];
		assert(a.size()==colwidths.size());
		
		string row = string(header_width,' ');
		
		unsigned int pos=0;
		for(unsigned int j=0; j<a.size(); j++){
			pair<string,string> &b = a[j];
			
			row.replace(pos+colwidths[j]-b.second.length()-1, b.second.length(), b.second);			
			pos += colwidths[j];
		}
		
		ss<<row<<endl;
	}
	
	return ss.str();
}
