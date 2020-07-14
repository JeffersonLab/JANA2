// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JCALIBRATIONMANAGER_H
#define JANA2_JCALIBRATIONMANAGER_H

#include <JANA/Calibrations/JCalibration.h>
#include <JANA/Calibrations/JCalibrationCCDB.h>
#include <JANA/Calibrations/JCalibrationFile.h>
#include <JANA/Calibrations/JCalibrationGenerator.h>

#include <JANA/Services/JServiceLocator.h>
#include <JANA/Services/JLoggingService.h>

#include <algorithm>

class JCalibrationManager : public JService {

    vector<JCalibration*> calibrations;
    vector<JCalibrationGenerator*> calibrationGenerators;
    pthread_mutex_t calibration_mutex;
    JLogger logger;
    std::string url = "file://./";
    std::string context = "default";

public:
    void acquire_services(JServiceLocator* service_locator) {

        // Configure our logger
        logger = service_locator->get<JLoggingService>()->get_logger("JCalibrationManager");

        auto params = service_locator->get<JParameterManager>();

        // Url and context may be passed in either as environment variables
        // or configuration parameters. Default values are used if neither is available.

        if (getenv("JANA_CALIB_URL") != nullptr) url = getenv("JANA_CALIB_URL");
        if (getenv("JANA_CALIB_CONTEXT") != nullptr) context = getenv("JANA_CALIB_CONTEXT");

        params->SetDefaultParameter("JANA:CALIB_URL", url, "URL used to access calibration constants");
        params->SetDefaultParameter("JANA:CALIB_CONTEXT", context,
                                  "Calibration context to pass on to concrete JCalibration derived class");
    }

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
        if (g) {
            calibrations.push_back(g);
            LOG_INFO(logger)
                << "Created JCalibration object of type: " << g->className() << "\n"
                << "  Generated via: " << (gen == NULL ? "fallback creation of JCalibrationFile" : gen->Description())
                << "\n"
                << "  Run: " << g->GetRun() << "\n"
                << "  URL: " << g->GetURL() << "\n"
                << "  context: " << g->GetContext()
                << LOG_END;
        }
        else {
            JLogMessage m(logger, JLogger::Level::ERROR);
            m << "Unable to create JCalibration object!\n"
              << "  Run: " << run_number << "\n"
              << "  URL: " << url << "\n"
              << "  context: " << context << "\n";

            if (gen) {
                m << "  Attempted to use generator: " << gen->Description();
            }
            else {
                m << "  No appropriate generators found. Attempted JCalibrationFile";
            }
            std::move(m) << LOG_END;
        }

        // Unlock calibration mutex
        pthread_mutex_unlock(&calibration_mutex);
        return g;
    }

    template<class T>
    bool GetCalib(unsigned int run_number, unsigned int event_number, string namepath, map<string, T>& vals) {
        /// Get the JCalibration object from JApplication for the run number of
        /// the current event and call its Get() method to get the constants.

        // Note that we could do this by making "vals" a generic type T thus, combining
        // this with the vector version below. However, doing this explicitly will make
        // it easier for the user to understand how to call us.

        vals.clear();
        JCalibration* calib = GetJCalibration(run_number);
        if (!calib) {
            LOG_ERROR(logger) << "Unable to get JCalibration object for run " << run_number << LOG_END;
            return true;
        }
        return calib->Get(namepath, vals, event_number);
    }

    template<class T>
    bool GetCalib(unsigned int run_number, unsigned int event_number, string namepath, vector<T>& vals) {
        /// Get the JCalibration object from JApplication for the run number of
        /// the current event and call its Get() method to get the constants.

        vals.clear();
        JCalibration* calib = GetJCalibration(run_number);
        if (!calib) {
            LOG_ERROR(logger) << "Unable to get JCalibration object for run " << run_number << LOG_END;
            return true;
        }
        return calib->Get(namepath, vals, event_number);
    }
};

#endif //JANA2_JCALIBRATIONMANAGER_H


