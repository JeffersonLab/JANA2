#ifndef _JCalibrationCCDB_
#define _JCalibrationCCDB_

// This entire file was copied from the CCDB 0.06 source
// (janaccdb directory). It has been modified based on
// changes that were later made to DCalibrationCCDB.h in
// the sim-recon code.

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <JANA/jerror.h>
#include <JANA/JCalibration.h>
#include <JANA/JStreamLog.h>
#include <CCDB/Calibration.h>

using namespace std;
using namespace jana;

// Place everything in JANA namespace
namespace jana
{

	/** 
	 *  Descendant of JCalibration class which allow to use CCDB as JANA calibration source
	 */
    class JCalibrationCCDB : public JCalibration
    {
    public:

        /** @brief    Constructor
         *
         * @parameter [in] url - connection string. like mysql://...
         * @parameter [in] run - run number
         * @parameter [in] context - variation
         */
        JCalibrationCCDB(ccdb::Calibration* calib, string url, int32_t run, string context="default"):
	        JCalibration(calib->GetConnectionString(), run, context)
	    {

		    mCalibration = calib;
		    pthread_mutex_init(&mutex, NULL);

			#ifdef CCDB_DEBUG_OUTPUT
			jout<<"CCDB::janaccdb created JCalibrationCCDB with connection string:" << calib->GetConnectionString()<< " run:"<<run<< " context:"<<context<<endl;
			#endif
        }


        /** @brief   destructor
         */
        virtual ~JCalibrationCCDB()
        {
				if(mCalibration!=NULL){
					pthread_mutex_lock(&mutex);
					delete mCalibration;
					pthread_mutex_unlock(&mutex);
				}
        }
        

        /** @brief gets a className
         */
        virtual const char* className(void)
        {
            return static_className();
        }


        /** @brief gets a className static version of function
         */
        static const char* static_className(void)
        {
            return "JCalibrationCCDB";
        }

        /** @brief gets pointer to underlying ccdb::Calibration object
         */
		ccdb::Calibration* GetCCDBCalibObj(void){
			return mCalibration;
		}

        /** @brief lock mutex used when accessing CCDB
         */
		void Lock(void){
			pthread_mutex_lock(&mutex);
		}

        /** @brief unlock mutex used when accessing CCDB
         */
		void Unlock(void){
			pthread_mutex_unlock(&mutex);
		}


        /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] svals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, map<string, string> &svals, uint64_t event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            //
            try
            {   
				//>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT                
                cout<<"CCDB::janaccdb"<<endl;
                cout<<"CCDB::janaccdb REQUEST map<string, string> request = '"<<namepath<<"'"<<endl;
                #endif  //>end of  CCDB debug output

                bool result = mCalibration->GetCalib(svals, namepath);

                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                string result_str((result)?string("loaded"):string("failure"));
                if(result)
                {
                    string first_value(" --NAN-- ");
                    if(svals.size()>0)
                    {
                        map<string, string>::const_iterator iter = svals.begin();
                        first_value.assign(iter->second);
                    }
                    cout<<"CCDB::janaccdb selected name-values count = '"<<svals.size()<<"' first_value '"<<first_value<<"'"<<endl;
                }
                #endif  //>end of  CCDB debug output

                pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif //end of CCDB debug output

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error
            }
        }


         /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] svals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, vector<string> &svals, uint64_t event_number=0)
        {
         /// The CCDB method corresponding to this one treats constants stored as
			/// a single column but many rows as an error and throws an exception if
			/// it is detected. The message in the exception suggests using a 
			/// vector<vector<T> > instead. Thus, we do that here and convert if
			/// necessary into a 1-D vector.
			///
			/// It is worth noting that back in May 2014 a similar issue was 
			/// addressed in the CCDB code itself when the method filling a
			/// map<string, string> was used. Fixing this one in CCDB also will
			/// require coordinated releases of CCDB and JANA. We choose to
			/// handle it here in order to avoid that.
			
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            try
            {   
				//>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT                
                cout<<"CCDB::janaccdb"<<endl;
                cout<<"CCDB::janaccdb REQUEST vector<string> request = '"<<namepath<<"'"<<endl;
                #endif  //>end of  CCDB debug output

				    vector<vector<string> > ssvals;
                bool result = mCalibration->GetCalib(ssvals, namepath);
				
				if(ssvals.size() == 1){
					// ---- COLUMN-WISE ----
					
					svals = ssvals[0];

				}else{
					// ---- ROW-WISE ----

					// add first element of each row (should only be one!)
					for(uint32_t i=0; i<ssvals.size(); i++){
						if(ssvals[i].size() > 0) svals.push_back(ssvals[i][0]);
					}
				}

                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                string result_str((result)?string("loaded"):string("failure"));
                if(result)
                {
                    string first_value(" --NAN-- ");
                    if(svals.size()>0)
                    {
                        vector<string>::const_iterator iter = svals.begin();
                        first_value.assign(*iter);
                    }
                    cout<<"CCDB::janaccdb selected name-values count = '"<<svals.size()<<"' first_value '"<<first_value<<"'"<<endl;
                }
                #endif  //>end of  CCDB debug output

                pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif //end of CCDB debug output

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error
            }
        }


        /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] vsvals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, vector< map<string, string> > &vsvals, uint64_t event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            //
            try
            {
				 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb"<<endl;
                 cout<<"CCDB::janaccdb REQUEST vector<map<string, string>> request = '"<<namepath<<"'"<<endl;
                 #endif  //end of CCDB debug output

                 bool result = mCalibration->GetCalib(vsvals, namepath);

                 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb result = "<<string ((result)?string("loaded"):string("failure"))<<endl;
                 if(result)
                 {
                     string first_value(" --NAN-- ");
                     if(vsvals.size()>0 && vsvals[0].size()>0)
                     {
                         map<string, string>::const_iterator iter = vsvals[0].begin();
                         first_value.assign(iter->second);
                     }

                     cout<<"CCDB::janaccdb selected rows = '"<<vsvals.size() <<"' selected columns = '"<<(int)((vsvals.size()>0)? vsvals[0].size() :0)
                         <<"' first value = '"<<first_value<<"'"<<endl;
                 }
                 #endif  //end of CCDB debug output


                pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error, CCDB otherwise
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error, CCDB otherwise
            }
        }


         /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] vsvals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, vector< vector<string> > &vsvals, uint64_t event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            //
            try
            {
				 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb"<<endl;
                 cout<<"CCDB::janaccdb REQUEST vector<vector<string> > request = '"<<namepath<<"'"<<endl;
                 #endif  //end of CCDB debug output

                 bool result = mCalibration->GetCalib(vsvals, namepath);

                 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb result = "<<string ((result)?string("loaded"):string("failure"))<<endl;
                 if(result)
                 {
                     string first_value(" --NAN-- ");
                     if(vsvals.size()>0 && vsvals[0].size()>0)
                     {
                         vector<string>::const_iterator iter = vsvals[0].begin();
                         first_value.assign(*iter);
                     }

                     cout<<"CCDB::janaccdb selected rows = '"<<vsvals.size() <<"' selected columns = '"<<(int)((vsvals.size()>0)? vsvals[0].size() :0)
                         <<"' first value = '"<<first_value<<"'"<<endl;
                 }
                 #endif  //end of CCDB debug output

                pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error, CCDB otherwise
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error, CCDB otherwise
            }
        }


       /** @brief    GetListOfNamepaths
         *
         * @parameter [in] vector<string> & namepaths
         * @return   void
         */
        void GetListOfNamepaths(vector<string> &namepaths)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            try
            {  
				//some ccdb debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb Getting list of namepaths. "<<endl;
                #endif

                mCalibration->GetListOfNamepaths(namepaths);
            }
            catch (std::exception ex)
            {

                //some ccdb debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb Exception cought at GetListOfNamepaths(vector<string> &namepaths). What = "<< ex.what()<<endl;
                #endif
            }
            pthread_mutex_unlock(&mutex);
        }
        
    private:
        JCalibrationCCDB();					// prevent use of default constructor
        ccdb::Calibration * mCalibration;	///Underlaying CCDB user api class 
        pthread_mutex_t mutex;
        
    };

} // Close JANA namespace

#endif // _JCalibrationCCDB_
