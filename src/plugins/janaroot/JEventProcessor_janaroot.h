// $Id$
//
//    File: JEventProcessor_janaroot.h
// Created: Fri Jul 11 03:39:49 EDT 2008
// Creator: davidl (on Darwin Amelia.local 8.11.1 i386)
//

#ifndef _JEventProcessor_janaroot_
#define _JEventProcessor_janaroot_

#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JFactory.h>
#include <JANA/Services/JLockService.h>
#include <JANA/JApplication.h>

class JEventProcessor_janaroot:public JEventProcessor{
	public:
		JEventProcessor_janaroot();
		~JEventProcessor_janaroot(){};
		const char* className(void){return "JEventProcessor_janaroot";}
		
		enum data_type_t{
			type_unknown,
			type_short,
			type_ushort,
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
				TTree *tree;
				vector<TBranch*> branches;
				int Nmax;
				unsigned long buff_size;
				void *buff;
				unsigned long obj_size; // size of one object
				vector<unsigned long> item_sizes;
				vector<data_type_t> types;
				int *Nptr;
				unsigned long Bptr;
			map<int, vector<std::string> > StringMap;
				
				void Print(void){
					cout<<"    tree name:"<<tree->GetName()<<endl;
					cout<<"Num. branches:"<<branches.size()<<endl;
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
		void Init(void) override;						///< Called once at program start.
		void BeginRun(const std::shared_ptr<const JEvent>& event) override;	///< Called everytime a new run number is detected.
		void Process(const std::shared_ptr<const JEvent>& event) override;	///< Called every event.
		void EndRun(void) override;						///< Called everytime run number changes, provided brun has been called.
		void Finish(void) override;						///< Called after last event of last event source has been processed.

		std::shared_ptr<JLockService> lock_svc;

		unsigned int Nevents;
		
		unsigned int Nwarnings;
		unsigned int MaxWarnings;
		
		int JANAROOT_VERBOSE;
		vector<string> nametags_to_write_out;

		TFile *file;
		std::map<std::string, TreeInfo*> trees;
		int Nmax; ///< maximum number of objects of a given type in the event
		
		TreeInfo* GetTreeInfo(JFactory *fac);
		void FillTree(JFactory *fac, TreeInfo *binfo);
};

#endif // _JEventProcessor_janaroot_

