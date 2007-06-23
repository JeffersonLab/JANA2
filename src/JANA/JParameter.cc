// $Id: JParameter.cc 1388 2005-12-13 14:13:24Z davidl $
//
//    File: JParameter.cc
// Created: Fri Aug 12 14:58:18 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <iostream>

#include "JParameter.h"

//---------------------------------
// JParameter    (Constructor)
//---------------------------------
JParameter::JParameter(string my_key, string my_value)
{
	key = my_key;
	value = my_value;
	isdefault = false;
	hasdefault = false;
}

//---------------------------------
// Dump
//---------------------------------
void JParameter::Dump(void)
{
	cout<<" -----------------------------"<<endl;
	cout<<"  className: "<<className()<<endl;
	cout<<"        key: "<<key<<endl;
	cout<<"      value: "<<value<<endl;
	cout<<"  isdefault: "<<isdefault<<endl;
	cout<<" hasdefault: "<<hasdefault<<endl;
	cout<<"    printme: "<<printme<<endl;
	cout<<"       type: "<<DataName(type)<<endl;
}

//---------------------------------
// DataName
//---------------------------------
const char* JParameter::DataName(dataType_t type)
{
	switch(type){
		case UNKNOWN:			return "unknown";
		case BOOL:				return "bool";
		case CHAR:				return "char";
		case CHAR_PTR:			return "char*";
		case CONST_CHAR_PTR:	return "const char*";
		case STRING:			return "string";
		case SHORT:				return "short";
		case INT:				return "int";
		case LONG:				return "long";
		case LONGLONG:			return "long long";
		case UCHAR:				return "unsigned char";
		case USHORT:			return "unsigned short";
		case UINT:				return "unsigned int";
		case ULONG:				return "unsigned long";
		case ULONGLONG:		return "unsigned long long";
		case FLOAT:				return "float";
		case DOUBLE:			return "double";
	}
	return "unknown";
}


