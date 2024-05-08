
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include "JCalibration.h"

#include <fstream>

class JCalibrationFile:public JCalibration{
    public:
        JCalibrationFile(string url, int32_t run, string context="default");
        virtual ~JCalibrationFile();
        virtual const char* className(void){return static_className();}
        static const char* static_className(void){return "JCalibrationFile";}

        bool GetCalib(string namepath, map<string, string> &svals, uint64_t event_number=0);
        bool GetCalib(string namepath, vector<string> &svals, uint64_t event_number=0);
        bool GetCalib(string namepath, vector< map<string, string> > &svals, uint64_t event_number=0);
        bool GetCalib(string namepath, vector< vector<string> > &svals, uint64_t event_number=0);
        bool PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, map<string, string> &svals, string comment="");
        bool PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< map<string, string> > &svals, string comment="");
        void GetListOfNamepaths(vector<string> &namepaths);

    protected:

        std::ofstream* CreateItemFile(string namepath, int32_t run_min, int32_t run_max, string &author, string &comment);
        void MakeDirectoryPath(string namepath);

    private:
        JCalibrationFile();

        string basedir;

        void AddToNamepathList(string dir, vector<string> &namepaths);
};


