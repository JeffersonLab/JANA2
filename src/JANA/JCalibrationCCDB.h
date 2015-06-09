#ifndef _JCalibrationCCDB_
#define _JCalibrationCCDB_

// This entire file was copied from the CCDB 0.06 source
// (janaccdb directory). The only modification was this
// comment.

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
        JCalibrationCCDB(ccdb::Calibration* calib, string url, int run, string context="default"):
	        JCalibration(calib->GetConnectionString(), run, context)
	    {

		    mCalibration = calib;

			#ifdef CCDB_DEBUG_OUTPUT
			jout<<"CCDB::janaccdb created JCalibrationCCDB with connection string:" << calib->GetConnectionString()<< " run:"<<run<< " context:"<<context<<endl;
			#endif
        }


        /** @brief   destructor
         */
        virtual ~JCalibrationCCDB()
        {
            if(mCalibration!=NULL) delete mCalibration;
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


        /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] svals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, map<string, string> &svals, int event_number=0)
        {
            //
            try
            {   
                bool result = mCalibration->GetCalib(svals, namepath);

                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                string result_str((result)?string("loaded"):string("failure"));
                cout<<"CCDB::janaccdb"<<endl;
                cout<<"CCDB::janaccdb REQUEST map<string, string> request = '"<<namepath<<"' result = "<<result_str<<endl;
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

                return !result; //JANA has false - if success and true if error
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif //end of CCDB debug output

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
        bool GetCalib(string namepath, vector<string> &svals, int event_number=0)
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
			
            try
            {   
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
                cout<<"CCDB::janaccdb"<<endl;
                cout<<"CCDB::janaccdb REQUEST vector<string> request = '"<<namepath<<"' result = "<<result_str<<endl;
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

                return !result; //JANA has false - if success and true if error
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif //end of CCDB debug output

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
        bool GetCalib(string namepath, vector< map<string, string> > &vsvals, int event_number=0)
        {
            //
            try
            {
                 bool result = mCalibration->GetCalib(vsvals, namepath);

                 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 string result_str((result)?string("loaded"):string("failure"));
                 cout<<"CCDB::janaccdb"<<endl;
                 cout<<"CCDB::janaccdb REQUEST vector<map<string, string>> request = '"<<namepath<<"' result = "<<result_str<<endl;
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


                return !result; //JANA has false - if success and true if error, CCDB otherwise
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif

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
        bool GetCalib(string namepath, vector< vector<string> > &vsvals, int event_number=0)
        {
            //
            try
            {
                 bool result = mCalibration->GetCalib(vsvals, namepath);

                 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 string result_str((result)?string("loaded"):string("failure"));
                 cout<<"CCDB::janaccdb"<<endl;
                 cout<<"CCDB::janaccdb REQUEST vector<vector<string>> request = '"<<namepath<<"' result = "<<result_str<<endl;
                 if(result)
                 {
                     string first_value(" --NAN-- ");
                     if(vsvals.size()>0 && vsvals[0].size()>0)
                     {
                         map<string, string>::const_iterator iter = vsvals[0].begin();
                         first_value.assign(*iter);
                     }

                     cout<<"CCDB::janaccdb selected rows = '"<<vsvals.size() <<"' selected columns = '"<<(int)((vsvals.size()>0)? vsvals[0].size() :0)
                         <<"' first value = '"<<first_value<<"'"<<endl;
                 }
                 #endif  //end of CCDB debug output


                return !result; //JANA has false - if success and true if error, CCDB otherwise
            }
            catch (std::exception ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif

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
            try
            {  
                mCalibration->GetListOfNamepaths(namepaths);
            }
            catch (std::exception ex)
            {

                //some ccdb debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb Exception cought at GetListOfNamepaths(vector<string> &namepaths). What = "<< ex.what()<<endl;
                #endif
            }
        }
        
    private:
        JCalibrationCCDB();					// prevent use of default constructor
        ccdb::Calibration * mCalibration;	///Underlaying CCDB user api class 
        
    };

} // Close JANA namespace

#endif // _JCalibrationCCDB_
