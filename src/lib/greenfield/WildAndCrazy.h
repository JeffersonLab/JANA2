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

#ifndef JANA2_WILDANDCRAZY_H
#define JANA2_WILDANDCRAZY_H

#if 0

// If we didn't have tags to worry about

class Source<T> {
    T* process() {
        return t;
    }
};


template<typename A, typename B, typename C, typename X> class Func3 {
public:
    virtual X* process(A* a, B* b, C* c) = 0;

    void process(WorkingMemory* mem) {
        A* a = mem.get<A>();
        B* b = mem.get<B>();
        C* c = mem.get<C>();
        X* x = process(a, b, c);
        mem.put<X>(x);
    }
};

class XFunc : public Func3<A,B,C,X> {
public:
    X* process(A* a, B* b, C* c) {
        return f(*a, *b, *c);
    }
};




// =======================


template <typename T> class Source {
public:
    virtual void process(Event& evt) = 0;
};


template <typename T> class Factory {
    std::vector<std::pair<std::type_index, std::string>>> _inputs;
    std::string _tag;
    std__type_index _typeidx;

public:
    TrackFactory(std::string tag) : _tag(tag), _typeidx(type_index(typeid(T))) {};

    template <typename S>
    void require(std::string tag) {
        _inputs.push_back(make_pair(type_index(typeid(S)), tag));
    }
    virtual void process(Event& evt) = 0;
};

class TrackFactory : public Factory<Track> {
public:
    TrackFactory(std::string tag) : Factory(tag) {
        requires<Hit>("simple_hits");
        requires<Cluster>("simple_cluster");
    }
    void process(Event& evt) {
        auto a = Event.get<Hit>("simple_hits");
        auto b = Event.get<Cluster>("simple_clusters");
        auto c = calculate_tracks(a,b);
        Set(c);  // Honestly I'd be happier returning something
    }
};


class MyHistogramSink {
public:
    MyHistogramSink(std::string name) {
        requires<Tracks>("simple_tracks");
    }
    void process(Event& e) {
        auto a = Event.get<Tracks>("simple_tracks");
        plot(a);
    }
};




// ==========================
// Using David's Lambda idea


class Source<T,tag> {
    T* process() {
        return t;
    }
};

class Factory<T,tag> {
    (void->T*) process(Event* e) {
        auto a = Event.get<Hit>("simple_hits");
        return [=] {
            auto b = sum(a);
        }
    }
};

class Sink {
    (void->void) process(Event* e) {
        auto a = Event.get<Hit>("simple_hits");
        auto b = Event.get<Cluster>("simple_clusters");
        return [=] {
            plot(a+b);
        }
    }
};




class WildAndCrazy {



};


#endif // ifdef 0
#endif //JANA2_WILDANDCRAZY_H
