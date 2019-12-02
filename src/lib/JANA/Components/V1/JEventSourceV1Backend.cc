//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

// $Id: JEventSource.cc 1347 2005-12-08 16:54:59Z davidl $
//
//    File: JEventSource.cc
// Created: Wed Jun  8 12:31:04 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <iostream>
#include <iomanip>
using namespace std;

#include <JANA/Components/JEventSourceFrontend.h>
#include <JANA/Components/V1/JEventV1.h>
#include <JANA/Components/V1/jerror.h>
#include <JANA/JLogger.h>

namespace jana {
namespace v1 {

//---------------------------------
// JEventSource    (Constructor)
//---------------------------------
JEventSource::JEventSource(const char *source_name)
{
    this->source_name = string(source_name);
    source_is_open = 0;
    pthread_mutex_init(&read_mutex, NULL);
    pthread_mutex_init(&in_progress_mutex, NULL);
    Nevents_read = 0;
    done_reading = false;
    Ncalls_to_GetEvent=0;
}

//---------------------------------
// ~JEventSource    (Destructor)
//---------------------------------
JEventSource::~JEventSource()
{

}

//----------------
// GetEvent
//----------------
jerror_t JEventSource::GetEvent(JEvent &event)
{
    /// This gets called from JApplication::ReadEvent so
    /// it can record the event in the base class before
    /// dispatching the call to the subclass' GetEvent method.
    pthread_mutex_lock(&in_progress_mutex);

    event.SetID(++Ncalls_to_GetEvent);
    in_progess_events.insert(event.GetID());

    pthread_mutex_unlock(&in_progress_mutex);

    return GetEvent(event);
}

//----------------
// GetObjects
//----------------
jerror_t JEventSource::GetObjects(JEvent &event, JFactory_base *factory)
{
    /// This only gets called if the subclass doesn't define it. In that
    /// case, the subclass must not support objects.
    return OBJECT_NOT_AVAILABLE;
}

//----------------
// FreeEvent
//----------------
void JEventSource::FreeEvent(JEvent &event)
{
    /// Free any memory associated with this event by calling the FreeEvent
    /// method of the subclass. This will also remove the event from the list
    /// of "in progress" events

    // As of JANA 0.6.6, this is now called from JEvent::FreeEvent explicitly
    // so that it can do some bookkeeping in addition to memory freeing
    // operations done in the subclass' FreeEvent()

    // Remove this event from the list of "in_progress" events
    pthread_mutex_lock(&in_progress_mutex);
    set<uint64_t>::iterator it = in_progess_events.find(event.GetID());
    if(it != in_progess_events.end()){
        in_progess_events.erase(it);
    }else{
        jerr<<" FreeEvent called for event not listed for this source!!!"<<endl;
        jerr<<" (event id = "<<event.GetID()<<")"<<endl;
    }
    pthread_mutex_unlock(&in_progress_mutex);

    // Call subclass' FreeEvent method
    // NOTE: if for some reason this is *not* a derived class or the derived
    // class did not supply a FreeEvent method then this will recursively call
    // itself.
    FreeEvent(event);
}

//----------------
// IsFinished
//----------------
bool JEventSource::IsFinished(void)
{
    /// Returns true if done_reading flags is set AND the in_progress container is empty
    /// indicating that we've both read the last event from the source and all events
    /// that have been read in have had FreeEvent called.

    if(!done_reading) return false;

    pthread_mutex_lock(&in_progress_mutex);
    bool in_progess_empty = in_progess_events.empty();
    pthread_mutex_unlock(&in_progress_mutex);

    return in_progess_empty;
}

}
}
