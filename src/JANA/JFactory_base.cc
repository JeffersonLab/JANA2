// $Id: JFactory_base.cc 1150 2005-08-02 12:23:03Z davidl $

#include <stdlib.h>
#include <iostream>
using namespace std;

#include "JFactory_base.h"

//-------------
// printheader
//-------------
void JFactory_base::printheader(const char *header)
{
	/// Print the string given, along with a separator(----)
	/// and the factory name to signify the start of this factory's data
	
	// Print the header with separator
	cout<<dataClassName();
	if(strlen(Tag())>0)cout<<" : "<<Tag();
	cout<<endl;
	cout<<"---------------------------------------"<<endl;
	cout<<header<<endl;
	cout<<endl;

	// Find and record the column positions (just look for colons)
	char *c = (char*)header;
	_icol = 0;
	while((c = strstr(c,":"))){
		_columns[_icol++] = (int)((unsigned long)c - (unsigned long)header);
		if(_icol>=99)break;
		c++;
	}
	for(int i=_icol;i<100;i++)_columns[i] = 100;
	_table = "";
	
	header_width = strlen(header)+1;
	if(header_width<79)header_width=79;
}

//-------------
// printnewrow
//-------------
void JFactory_base::printnewrow(void)
{
	/// Initialize internal buffer in preparation of printing a new row.
	/// Call this before calling printcol().
	_row = string(header_width,' ');
	_icol = 0;
}

//-------------
// printnewrow
//-------------
void JFactory_base::printrow(void)
{
	/// Print the row to the screen. Make a call to printcol() for every
	/// column before calling this.
	_table += _row + "\n";
}

//-------------
// printcol
//-------------
void JFactory_base::printcol(const char *str)
{
	/// Print a formatted value to "str". Used by Print()
	_row.replace(_columns[_icol++]-strlen(str), strlen(str), str);
}
