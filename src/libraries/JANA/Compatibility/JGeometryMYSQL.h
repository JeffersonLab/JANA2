// $Id$
//
//    File: JGeometryMYSQL.h
// Created: Wed May  7 16:21:37 EDT 2008
// Creator: davidl (on Darwin swire-d95.jlab.org 8.11.1 i386)
//

#pragma once
#include <JANA/Compatibility/jerror.h>
#include <JANA/Compatibility/JGeometry.h>
#include <JANA/CLI/JVersion.h>


class JGeometryMYSQL:public JGeometry{
    public:
        JGeometryMYSQL(string url, int run, string context="default");
        virtual ~JGeometryMYSQL();

        bool Get(string xpath, string &sval);
        bool Get(string xpath, map<string, string> &svals);
        bool GetMultiple(string xpath, vector<string> &vsval);
        bool GetMultiple(string xpath, vector<map<string, string> >&vsvals);
        void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level, const string &filter="");

    protected:


    private:
        JGeometryMYSQL();

};


