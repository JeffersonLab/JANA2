// $Id$
//
//    File: JEventProcessor_janahbook.h
// Created: Mon Aug 11 09:00:05 EDT 2008
// Creator: davidl (on Darwin Amelia.local 8.11.1 i386)
//

#ifndef _JEventProcessor_janahbook_
#define _JEventProcessor_janahbook_

#include <map>
#include <string>
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;
using std::hex;
using std::dec;

#include <JANA/JEventProcessor.h>
#include <JANA/JFactory_base.h>

class JEventProcessor_janahbook:public jana::JEventProcessor{
	public:
		JEventProcessor_janahbook();
		~JEventProcessor_janahbook(){};
		const char* className(void){return "JEventProcessor_janahbook";}
		
		enum data_type_t{
			type_unknown,
			type_int,
			type_uint,
			type_long,
			type_ulong,
			type_float,
			type_double,
			type_string
		};
		
		class TreeInfo{
			public:
				int ntp_num;
				int Nmax;
				unsigned long buff_size;
				void *buff;
				unsigned long obj_size; // size of one object
				vector<unsigned long> item_sizes;
				vector<data_type_t> types;
				int *Nptr;
				unsigned long Bptr;
				
				void Print(void){
					cout<<"Ntuple number:"<<ntp_num<<endl;
					//cout<<"Num. branches:"<<branches.size()<<endl;
					cout<<"         Nmax:"<<Nmax<<endl;
					cout<<"    buff_size:"<<buff_size<<endl;
					cout<<"         buff:0x"<<hex<<(unsigned long)buff<<dec<<endl;
					cout<<"     obj_size:"<<obj_size<<endl;
					cout<<"        items:";
					for(unsigned int i=0; i<item_sizes.size(); i++){
						if(i!=0)cout<<"              ";
						cout<<"size="<<item_sizes[i]<<"  type="<<types[i]<<endl;
					}
					cout<<"        Nptr:0x"<<hex<<(unsigned long)Nptr<<dec<<endl;
					cout<<"        Bptr:0x"<<hex<<(unsigned long)Bptr<<dec<<endl;
				}
		};
		
	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.


		pthread_mutex_t rootmutex;
		unsigned int Nevents;

		std::map<std::string, TreeInfo*> trees;
		int Nmax; ///< maximum number of objects of a given type in the event
		
		TreeInfo* GetTreeInfo(jana::JFactory_base *fac);
		void FillTree(jana::JFactory_base *fac, TreeInfo *binfo);
};

#endif // _JEventProcessor_janahbook_

