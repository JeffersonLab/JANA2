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

#ifndef JANA2_JCSVEVENTPROCESSOR_H
#define JANA2_JCSVEVENTPROCESSOR_H


#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include <fstream>

template <typename T>
class JCsvWriter : public JEventProcessor {
private:
    std::string m_tag;
    std::string m_dest_dir = ".";
    std::fstream m_dest_file;
    std::mutex m_mutex;
    bool m_header_written = false;

public:

    JCsvWriter(std::string tag = "") : m_tag(std::move(tag)) {
        SetTypeName(NAME_OF_THIS);
        japp->GetJParameterManager()->SetDefaultParameter("csv:dest_dir", m_dest_dir, "Location where CSV files get written");
    };

    void Init(void) override {

        std::string filename;
        if (m_tag == "") {
            filename = m_dest_dir + "/" + JTypeInfo::demangle<T>() + ".csv";
        }
        else {
            filename = m_dest_dir + "/" + JTypeInfo::demangle<T>() + "_" + m_tag + ".csv";
        }
        m_dest_file.open(filename, std::fstream::out);
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {

        auto event_nr = event->GetEventNumber();
        auto jobjs = event->Get<T>(m_tag);

        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_header_written) {
            if (jobjs.size() > 0) {
                JObjectSummary summary;
                jobjs[0]->Summarize(summary);
                m_dest_file << "EventNr";
                for (auto& field : summary.get_fields()) {
                    m_dest_file << ", " << field.name;
                }
                m_dest_file << std::endl;
                m_header_written = true;
            }
        }

        for (auto obj : jobjs) {

            JObjectSummary summary;
            obj->Summarize(summary);
            m_dest_file << event_nr;
            for (auto& field : summary.get_fields()) {
                m_dest_file << ", " << field.value;
            }
            m_dest_file << std::endl;
        }

    }

    void Finish(void) override {
        m_dest_file.close();
    }

    string GetType() const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }


};


#endif //JANA2_JCSVEVENTPROCESSOR_H
