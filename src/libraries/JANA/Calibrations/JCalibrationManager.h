// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Calibrations/JCalibration.h>
#include <JANA/Calibrations/JCalibrationFile.h>
#include <JANA/Calibrations/JCalibrationGenerator.h>

#include <JANA/JService.h>

#include <algorithm>
#include "JANA/Services/JParameterManager.h"
#include "JLargeCalibration.h"

class JCalibrationManager : public JService {

    vector<JCalibration *> m_calibrations;
    vector<JLargeCalibration *> m_resource_managers;
    vector<JCalibrationGenerator *> m_calibration_generators;

    pthread_mutex_t m_calibration_mutex;
    pthread_mutex_t m_resource_manager_mutex;

    std::shared_ptr<JParameterManager> m_params;

    std::string m_url = "file://./";
    std::string m_context = "default";

public:

    JCalibrationManager() { 
        SetPrefix("jana"); 
    }

    void acquire_services(JServiceLocator* sl) {

        // Url and context may be passed in either as environment variables
        // or configuration parameters. Default values are used if neither is available.

        if (getenv("JANA_CALIB_URL") != nullptr) m_url = getenv("JANA_CALIB_URL");
        if (getenv("JANA_CALIB_CONTEXT") != nullptr) m_context = getenv("JANA_CALIB_CONTEXT");

        m_params = sl->get<JParameterManager>();
        m_params->SetDefaultParameter("JANA:CALIB_URL", m_url, "URL used to access calibration constants");
        m_params->SetDefaultParameter("JANA:CALIB_CONTEXT", m_context,
                                    "Calibration context to pass on to concrete JCalibration derived class");
    }

    void AddCalibrationGenerator(JCalibrationGenerator *generator) {
        m_calibration_generators.push_back(generator);
    };

    void RemoveCalibrationGenerator(JCalibrationGenerator *generator) {

        vector<JCalibrationGenerator *> &f = m_calibration_generators;
        vector<JCalibrationGenerator *>::iterator iter = std::find(f.begin(), f.end(), generator);
        if (iter != f.end())f.erase(iter);
    }

    vector<JCalibrationGenerator *> GetCalibrationGenerators() { return m_calibration_generators; }

    void GetJCalibrations(vector<JCalibration *> &calibs) { calibs = m_calibrations; }

    JCalibration *GetJCalibration(unsigned int run_number) {
        /// Return a pointer to the JCalibration object that is valid for the given run number.
        ///
        /// This first searches through the list of existing JCalibration objects (created by JCalibrationManager)
        /// to see if it already has the right one. If so, a pointer to it is returned. If not, a new JCalibration
        /// object is created and added to the internal list. Note that since we need to make sure the list is not
        /// modified by one thread while being searched by another, a mutex is locked while searching the list.
        /// It is <b>NOT</b> efficient to get or even use the JCalibration object every event. Factories should access
        /// it in their brun() callback and keep a local copy of the required constants for use in the evnt() callback.


        // Lock mutex to keep list from being modified while we search it
        pthread_mutex_lock(&m_calibration_mutex);

        vector<JCalibration *>::iterator iter = m_calibrations.begin();
        for (; iter != m_calibrations.end(); iter++) {
            if ((*iter)->GetRun() != (int) run_number)continue;
            if ((*iter)->GetURL() != m_url)continue;                    // These allow specialty programs to change
            if ((*iter)->GetContext() != m_context)continue;        // the source and still use us to instantiate
            // Found it! Unlock mutex and return pointer
            JCalibration *g = *iter;
            pthread_mutex_unlock(&m_calibration_mutex);
            return g;
        }

        // JCalibration object for this run_number doesn't exist in our list. Create a new one and add it to the list.
        // We need to create an object of the appropriate subclass of JCalibration. This determined by looking through the
        // existing JCalibrationGenerator objects and finding the which claims the highest probability of being able to
        // open it based on the URL. If there are no generators claiming a non-zero probability and the URL starts with
        // "file://", then a JCalibrationFile object is created (i.e. we don't bother making a JCalibrationGeneratorFile
        // class and instead, handle it here.)

        JCalibrationGenerator *gen = nullptr;
        double liklihood = 0.0;
        for (unsigned int i = 0; i < m_calibration_generators.size(); i++) {
            double my_liklihood = m_calibration_generators[i]->CheckOpenable(m_url, run_number, m_context);
            if (my_liklihood > liklihood) {
                liklihood = my_liklihood;
                gen = m_calibration_generators[i];
            }
        }

        // Make the JCalibration object
        JCalibration *g = nullptr;
        if (gen) {
            g = gen->MakeJCalibration(m_url, run_number, m_context);
        }
        if (gen == nullptr && (m_url.find("file://") == 0)) {
            g = new JCalibrationFile(m_url, run_number, m_context);
        }
        if (g) {
            m_calibrations.push_back(g);
            LOG_INFO(m_logger)
                << "Created JCalibration object of type: " << g->className() << "\n"
                << "  Generated via: "
                << (gen == nullptr ? "fallback creation of JCalibrationFile" : gen->Description())
                << "\n"
                << "  Run: " << g->GetRun() << "\n"
                << "  URL: " << g->GetURL() << "\n"
                << "  context: " << g->GetContext()
                << LOG_END;
        } else {
            JLogMessage m(m_logger, JLogger::Level::ERROR);
            m << "Unable to create JCalibration object!\n"
              << "  Run: " << run_number << "\n"
              << "  URL: " << m_url << "\n"
              << "  context: " << m_context << "\n";

            if (gen) {
                m << "  Attempted to use generator: " << gen->Description();
            } else {
                m << "  No appropriate generators found. Attempted JCalibrationFile";
            }
            std::move(m) << LOG_END;
        }

        // Unlock calibration mutex
        pthread_mutex_unlock(&m_calibration_mutex);
        return g;
    }

    template<class T>
    bool GetCalib(unsigned int run_number, unsigned int event_number, string namepath, map<string, T> &vals) {
        /// Get the JCalibration object from JApplication for the run number of
        /// the current event and call its Get() method to get the constants.

        // Note that we could do this by making "vals" a generic type T thus, combining
        // this with the vector version below. However, doing this explicitly will make
        // it easier for the user to understand how to call us.

        vals.clear();
        JCalibration *calib = GetJCalibration(run_number);
        if (!calib) {
            LOG_ERROR(m_logger) << "Unable to get JCalibration object for run " << run_number << LOG_END;
            return true;
        }
        return calib->Get(namepath, vals, event_number);
    }

    template<class T>
    bool GetCalib(unsigned int run_number, unsigned int event_number, string namepath, vector<T> &vals) {
        /// Get the JCalibration object from JApplication for the run number of
        /// the current event and call its Get() method to get the constants.

        vals.clear();
        JCalibration *calib = GetJCalibration(run_number);
        if (!calib) {
            LOG_ERROR(m_logger) << "Unable to get JCalibration object for run " << run_number << LOG_END;
            return true;
        }
        return calib->Get(namepath, vals, event_number);
    }


    JLargeCalibration *GetLargeCalibration(unsigned int run_number = 0) {

        /// Return a pointer to the JLargeCalibration object for the specified run_number. If no run_number is given or a
        /// value of 0 is given, then the first element from the list of resource managers is returned. If no managers
        /// currently exist, one will be created using one of the following in order of precedence:
        /// 1. JCalibration corresponding to given run number
        /// 2. First JCalibration object in list (used when run_number is zero)
        /// 3. No backing JCalibration object.
        ///
        /// The JCalibration is used to hold the URLs of resources so when a namepath is specified, the location of the
        /// resource on the web can be obtained and the file downloaded if necessary. See documentation in the JResourceManager
        /// class for more details.

        // Handle case for when no run number is specified
        if (run_number == 0) {
            pthread_mutex_lock(&m_resource_manager_mutex);
            if (m_resource_managers.empty()) {
                if (m_calibrations.empty()) {
                    m_resource_managers.push_back(new JLargeCalibration(m_params, nullptr));
                } else {
                    m_resource_managers.push_back(new JLargeCalibration(m_params, m_calibrations[0]));
                }
            }
            pthread_mutex_unlock(&m_resource_manager_mutex);

            return m_resource_managers[0];
        }

        // Run number is non-zero. Use it to get a JCalibration pointer
        JCalibration *jcalib = GetJCalibration(run_number);
        for (unsigned int i = 0; i < m_resource_managers.size(); i++) {
            if (m_resource_managers[i]->GetJCalibration() == jcalib)return m_resource_managers[i];
        }

        // No resource manager exists for the JCalibration that corresponds to the given run_number. Create one.
        JLargeCalibration *resource_manager = new JLargeCalibration(m_params, jcalib);
        pthread_mutex_lock(&m_resource_manager_mutex);
        m_resource_managers.push_back(resource_manager);
        pthread_mutex_unlock(&m_resource_manager_mutex);

        return resource_manager;

    }
};


