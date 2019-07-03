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

#ifndef JANA2_RAWHIT_H
#define JANA2_RAWHIT_H

#include <JANA/JObject.h>
#include <JANA/JException.h>

struct DetectorAHit : public JObject {
    std::string sensor;
    size_t id;
    double V, t, x, y, z;
};


template <typename T>
struct Serializer {
    T deserialize(const std::string&) {
        throw JException("Deserializer not implemented!");
    };
    std::string serialize(const T& rh) {
        throw JException("Serializer not implemented!");
    };
};

template <>
struct Serializer<DetectorAHit> {
    DetectorAHit deserialize(const std::string& s) {

        DetectorAHit x;
        char sensor[64];
        int matches = sscanf(s.c_str(), "%64s %lu %lf %lf %lf %lf %lf",
                             sensor, &x.id, &x.V, &x.t, &x.x, &x.y, &x.z);
        if (matches != 7) {
            throw JException("Unable to parse string as DetectorAHit!");
        }
        x.sensor = std::string(sensor);
        return x;
    }

    std::string serialize(const DetectorAHit& x) {
        char buffer[200];
        sprintf(buffer, "%s %lu %.2lf %.2lf %.2lf %.2lf %.2lf",
                x.sensor.c_str(), x.id, x.V, x.t, x.x, x.y, x.z);
        auto result = std::string(buffer);
        return result;
    }

};

#endif //JANA2_RAWHIT_H
