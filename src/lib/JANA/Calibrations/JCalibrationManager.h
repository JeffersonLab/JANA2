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

#ifndef JANA2_JCALIBRATIONMANAGER_H
#define JANA2_JCALIBRATIONMANAGER_H

#include <JANA/Calibrations/JCalibration.h>
#include <JANA/Calibrations/JCalibrationCCDB.h>
#include <JANA/Calibrations/JCalibrationFile.h>
#include <JANA/Calibrations/JCalibrationGenerator.h>
#include <JANA/Services/JServiceLocator.h>

#include <algorithm>

class JCalibrationManager : public JService {

    vector<JCalibration*> calibrations;
    vector<JCalibrationGenerator*> calibrationGenerators;
    pthread_mutex_t calibration_mutex;

public:
    void AddCalibrationGenerator(JCalibrationGenerator* generator) {
        calibrationGenerators.push_back(generator);
    };

    void RemoveCalibrationGenerator(JCalibrationGenerator* generator) {

        vector<JCalibrationGenerator*>& f = calibrationGenerators;
        vector<JCalibrationGenerator*>::iterator iter = std::find(f.begin(), f.end(), generator);
        if(iter != f.end())f.erase(iter);
    }

    vector<JCalibrationGenerator*> GetCalibrationGenerators(){ return calibrationGenerators; }

    void GetJCalibrations(vector<JCalibration*> &calibs){ calibs=calibrations; }

    JCalibration* GetJCalibration(unsigned int run_number) {
        /// Return a pointer to the JCalibration object that is valid for the
        /// given run number.
        ///
        /// This first searches through the list of existing JCalibration
        /// objects (created by this JApplication object) to see if it
        /// already has the right one.If so, a pointer to it is returned.
        /// If not, a new JCalibration object is created and added to the
        /// internal list.
        /// Note that since we need to make sure the list is not modified
        /// by one thread while being searched by another, a mutex is
        /// locked while searching the list. It is <b>NOT</b> efficient
        /// to get or even use the JCalibration object every event. Factories
        /// should access it in their brun() callback and keep a local
        /// copy of the required constants for use in the evnt() callback.

        // url and context may be passed in either as environment variables
        // or configuration parameters. Default values are used if neither
        // is available.
        string url     = "file://./";
        string context = "default";
        if( getenv("JANA_CALIB_URL"    )!=NULL ) url     = getenv("JANA_CALIB_URL");
        if( getenv("JANA_CALIB_CONTEXT")!=NULL ) context = getenv("JANA_CALIB_CONTEXT");
        japp->SetDefaultParameter("JANA_CALIB_URL",     url,     "URL used to access calibration constants");
        japp->SetDefaultParameter("JANA_CALIB_CONTEXT", context, "Calibration context to pass on to concrete JCalibration derived class");

        // Lock mutex to keep list from being modified while we search it
        pthread_mutex_lock(&calibration_mutex);

        vector<JCalibration*>::iterator iter = calibrations.begin();
        for(; iter!=calibrations.end(); iter++){
            if((*iter)->GetRun()!=(int)run_number)continue;
            if((*iter)->GetURL()!=url)continue;					// These allow specialty programs to change
            if((*iter)->GetContext()!=context)continue;		// the source and still use us to instantiate
            // Found it! Unlock mutex and return pointer
            JCalibration *g = *iter;
            pthread_mutex_unlock(&calibration_mutex);
            return g;
        }

        // JCalibration object for this run_number doesn't exist in our list.
        // Create a new one and add it to the list.
        // We need to create an object of the appropriate subclass of
        // JCalibration. This determined by looking through the
        // existing JCalibrationGenerator objects and finding the
        // which claims the highest probability of being able to
        // open it based on the URL. If there are no generators
        // claiming a non-zero probability and the URL starts with
        // "file://", then a JCalibrationFile object is created
        // (i.e. we don't bother making a JCalibrationGeneratorFile
        // class and instead, handle it here.)

        JCalibrationGenerator* gen = NULL;
        double liklihood = 0.0;
        for(unsigned int i=0; i<calibrationGenerators.size(); i++){
            double my_liklihood = calibrationGenerators[i]->CheckOpenable(url, run_number, context);
            if(my_liklihood > liklihood){
                liklihood = my_liklihood;
                gen = calibrationGenerators[i];
            }
        }

        // Make the JCalibration object
        JCalibration *g=NULL;
        if(gen){
            g = gen->MakeJCalibration(url, run_number, context);
        }
        if(gen==NULL && (url.find("file://")==0)){
            g = new JCalibrationFile(url, run_number, context);
        }
        if(g){
            calibrations.push_back(g);
            jout<<"Created JCalibration object of type: "<<g->className()<<jendl;
            jout<<"Generated via: "<< (gen==NULL ? "fallback creation of JCalibrationFile":gen->Description())<<jendl;
            jout<<"Run:"<<g->GetRun()<<jendl;
            jout<<"URL: "<<g->GetURL()<<jendl;
            jout<<"context: "<<g->GetContext()<<jendl;
        }else{
            _DBG__;
            _DBG_<<"Unable to create JCalibration object!"<<endl;
            _DBG_<<"    URL: "<<url<<endl;
            _DBG_<<"context: "<<context<<endl;
            _DBG_<<"    run: "<<run_number<<endl;
            if(gen)
                _DBG_<<"attempted to use generator: "<<gen->Description()<<endl;
            else
                _DBG_<<"no appropriate generators found. attempted JCalibrationFile"<<endl;
        }

        // Unlock calibration mutex
        pthread_mutex_unlock(&calibration_mutex);

        return g;

    }

    template<class T>
    bool GetCalib(unsigned int run_number, unsigned int event_number, string namepath, map<string,T> &vals)
    {
        /// Get the JCalibration object from JApplication for the run number of
        /// the current event and call its Get() method to get the constants.

        // Note that we could do this by making "vals" a generic type T thus, combining
        // this with the vector version below. However, doing this explicitly will make
        // it easier for the user to understand how to call us.

        vals.clear();
        JCalibration *calib = GetJCalibration(run_number);
        if(!calib){
            _DBG_<<"Unable to get JCalibration object for run "<<run_number<<std::endl;
            return true;
        }
        return calib->Get(namepath, vals, event_number);
    }

    template<class T> bool GetCalib(unsigned int run_number, unsigned int event_number, string namepath, vector<T> &vals) {
        /// Get the JCalibration object from JApplication for the run number of
        /// the current event and call its Get() method to get the constants.

        vals.clear();
        JCalibration *calib = GetJCalibration(run_number);
        if(!calib){
            _DBG_<<"Unable to get JCalibration object for run "<<run_number<<std::endl;
            return true;
        }
        return calib->Get(namepath, vals, event_number);
    }
};

#endif //JANA2_JCALIBRATIONMANAGER_H


