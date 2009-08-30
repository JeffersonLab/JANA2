
#ifndef _RS_MAINFRAME_H_
#define _RS_MAINFRAME_H_

// This class is made into a ROOT dictionary ala rootcint.
// Therefore, do NOT include anything Hall-D specific here.
// It is OK to do that in the .cc file, just not here in the 
// header.

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <time.h>

#include <TGClient.h>
#include <TGButton.h>
#include <TCanvas.h>
#include <TText.h>
#include <TRootEmbeddedCanvas.h>
#include <TTUBE.h>
#include <TNode.h>
#include <TGComboBox.h>
#include <TPolyLine.h>
#include <TEllipse.h>
#include <TMarker.h>
#include <TVector3.h>
#include <TGLabel.h>
#include <TTimer.h>
#include <TH1.h>


class rs_mainframe:public TGMainFrame {

	public:
		rs_mainframe(const TGWindow *p, UInt_t w, UInt_t h);
		~rs_mainframe(){};
		
		void ReadPreferences(void);
		void SavePreferences(void);
		
		// Slots for ROOT GUI
		void DoQuit(void);
		void DoTimer(void);
		void DoSelectServerHist(void);
		void DoSelectDelay(Int_t index);
		void DoUpdate(void);

		TGMainFrame *dialog_selectserverhist;
		TGLabel *selected_server;
		TGLabel *selected_hist;
		TGLabel *retrieved_lab;
		TGComboBox *delay;
		TCanvas *canvas;
		TGCheckButton *auto_refresh;
		TGCheckButton *loop_over_servers;
		TGCheckButton *loop_over_hists;
		
		bool delete_dialog_selectserverhist;

	private:
	
		TTimer *timer;
		long sleep_time; // in milliseconds
		time_t last_called;				// last time DoTimer() was called
		time_t last_ping_time;			// last time we broadcast for a server list
		time_t last_hist_requested;	// last time we requested a histogram (see rs_info for time we actually got it)
		
		time_t delay_time;
		
		string last_hnamepath_requested;	// last hnamepath requested
		TH1 *last_hist_plotted;
		
		void CreateGUI(void);
		
	ClassDef(rs_mainframe,1)
};



#endif //_RS_MAINFRAME_H_
