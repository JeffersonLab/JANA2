// $Id: JEvent.cc 1039 2005-06-14 20:21:02Z davidl $
//
//    File: JEvent.cc
// Created: Wed Jun  8 12:30:53 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include "JEvent.h"
#include <JANA/JApplication.h>
using namespace std;
using namespace jana;


//---------------------------------
// JEvent    (Constructor)
//---------------------------------
JEvent::JEvent()
{
	source = NULL;
	event_number = 0 ;
	run_number = 0;
	ref = NULL;
	status = 0L;
	sequential = false;
}

//---------------------------------
// ~JEvent    (Destructor)
//---------------------------------
JEvent::~JEvent()
{

}

//---------------------------------
// Print
//---------------------------------
void JEvent::Print(void)
{
	jout<<"JEvent: this=0x"<<hex<<(unsigned long)this<<dec;
	jout<<" source=0x"<<hex<<(unsigned long)source<<dec;
	jout<<" event_number="<<event_number;
	jout<<" run_number="<<run_number;
	jout<<" ref=0x"<<hex<<(unsigned long)ref<<dec;
	jout<<" loop=0x"<<hex<<(unsigned long)loop<<dec;
	jout<<" status=0x"<<hex<<status<<dec;
	jout<<" sequential="<<sequential;
	jout<<endl;
}

//---------------------------------
// GetStatusBit
//---------------------------------
bool JEvent::GetStatusBit(uint32_t bit)
{
	/// Return the present value of the specified status bit.
	/// The value of "bit" should be from 0-63.
	
	return (status>>bit) & 0x01;
}

//---------------------------------
// SetStatusBit
//---------------------------------
bool JEvent::SetStatusBit(uint32_t bit, bool val)
{
	/// Set the value of the specified status bit. If the
	/// second argument is passed, the bit will be set to
	/// that value. Otherwise, the bit will be set to "true".
	/// The value of "bit" should be from 0-63.
	/// The value of the status bit prior to  this call is
	/// returned.

	bool old_val = (status>>bit) & 0x01;
	
	uint64_t mask = ((uint64_t)0x01)<<bit;
	
	if(val){
		// Set bit
		status |= mask;
	}else{
		// Clear bit
		status &= ~mask;
	}
	
	return old_val;
}

//---------------------------------
// ClearStatusBit
//---------------------------------
bool JEvent::ClearStatusBit(uint32_t bit)
{
	/// Clear the specified status bit.
	/// The value of "bit" should be from 0-63.
	/// This is equivalent to calling SetStatusBit(bit, false).
	/// The value of the status bit prior to  this call is
	/// returned.
	
	bool old_val = (status>>bit) & 0x01;
	
	uint64_t mask = ((uint64_t)0x01)<<bit;
	
	status &= ~mask;
	
	return old_val;
}

//---------------------------------
// ClearStatus
//---------------------------------
void JEvent::ClearStatus(void)
{
	/// Clear all bits in the status word. This
	/// is equivalent to calling SetStatus(0).
	
	status = 0L;
}

//---------------------------------
// SetStatusBitDescription
//---------------------------------
void JEvent::SetStatusBitDescription(uint32_t bit, string description)
{
	if(japp)japp->SetStatusBitDescription(bit, description);
}

//---------------------------------
// GetStatusBitDescription
//---------------------------------
string JEvent::GetStatusBitDescription(uint32_t bit)
{
	if(japp) return japp->GetStatusBitDescription(bit);
	return "no description available";
}

//---------------------------------
// GetStatusBitDescriptions
//---------------------------------
void JEvent::GetStatusBitDescriptions(map<uint32_t, string> &status_bit_descriptions)
{
	if(japp)japp->GetStatusBitDescriptions(status_bit_descriptions);	
}

