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

#include "JSubeventArrow.h"





JSubeventArrow::JSubeventArrow(std::string name, JSubeventProcessor* processor,
                               Queue<JSubevent>* inbox, Queue<JSubevent>* outbox)
    : JArrow(name, true), _processor(processor), _inbox(inbox), _outbox(outbox) {


}

void JSubeventArrow::execute(JArrowMetrics& results) {
    // auto in_result = _inbox.pop(in_buffer, get_chunksize());
    // for (JObject* item : in_buffer) {
    //    _processor.process(item);
    // }
    // auto out_result = _outbox.push(out_buffer);
    // measure everything
}

JSplitArrow::JSplitArrow(std::string name, JSubeventProcessor* processor,
                         Queue<Event>* inbox, Queue<JSubevent>* outbox)
    : JArrow(name, false), _processor(processor), _inbox(inbox), _outbox(outbox) {

}

void JSplitArrow::execute(JArrowMetrics& results) {
    // input_queue.pop(in_buffer, get_chunksize());
    // _processor.split(event, in_buffer);
    // size_t count = in_buffer.size();
    // for (size_t i=0; i<count; ++i) {
    //     out_buffer.push_back(JSubevent(event, buffer[i], i, count));
    // }
    // _outbox.push(out_buffer);
    // measure everything
}

JMergeArrow::JMergeArrow(std::string name, JSubeventProcessor* processor, Queue<JSubevent>* inbox, Queue<Event>* outbox)
    : JArrow(name, false), _processor(processor),  _inbox(inbox), _outbox(outbox) {

}

void JMergeArrow::execute(JArrowMetrics& results) {
    // This is the complicated one
    // For now, don't worry about allocations, or about queue overflow
    // Maintain a map of {parent, (count, [subevents])}
    // on each add, if subevents.size() = count, then
    //
    // Alternative: If item is not part of current event, then push back onto queue
}
