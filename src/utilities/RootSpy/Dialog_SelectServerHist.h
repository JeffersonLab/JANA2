// $Id$
//
//    File: Dialog_SelectServerHist.h
// Created: Sat Aug 29 09:02:35 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#ifndef _Dialog_SelectServerHist_
#define _Dialog_SelectServerHist_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>

class Dialog_SelectServerHist:public TGMainFrame{
	public:
		Dialog_SelectServerHist(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_SelectServerHist();
		
		void DoOK(void);
		void DoCancel(void);
		void DoTimer(void);
		void DoSelectServer(Int_t index);
		
	protected:
	
	
	private:

		TTimer *timer;
		long sleep_time; // in milliseconds
		time_t last_called;
		time_t last_ping_time;
		time_t last_hist_time;
		vector<string> last_servers;
		vector<string> last_hnamepaths;

		void CreateGUI(void);

		TGComboBox *server;
		TGComboBox *hist;

	ClassDef(Dialog_SelectServerHist,1)
};

#endif // _Dialog_SelectServerHist_

