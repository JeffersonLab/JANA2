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
