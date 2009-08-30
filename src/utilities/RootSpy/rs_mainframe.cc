
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <fstream>
using namespace std;


#include "RootSpy.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"
#include "Dialog_SelectServerHist.h"

#include <TApplication.h>
#include <TPolyMarker.h>
#include <TLine.h>
#include <TMarker.h>
#include <TBox.h>
#include <TVector3.h>
#include <TGeoVolume.h>
#include <TGeoManager.h>
#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TArrow.h>
#include <TLatex.h>
#include <TColor.h>


//-------------------
// Constructor
//-------------------
rs_mainframe::rs_mainframe(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{

	// Define all of the of the graphics objects. 
	CreateGUI();

	// Set up timer to call the DoTimer() method repeatedly
	// so events can be automatically advanced.
	timer = new TTimer();
	timer->Connect("Timeout()", "rs_mainframe", this, "DoTimer()");
	sleep_time = 250;
	timer->Start(sleep_time, kFALSE);

	time_t now = time(NULL);
	last_called=now-1;
	last_ping_time = now;
	last_hist_requested = now -3;
	
	delay_time = 4; // default is 4 seconds (needs to be tied to default used to set GUI)
	
	last_hnamepath_requested = "N/A";
	last_hist_plotted = NULL;
	
	dialog_selectserverhist = NULL;
	delete_dialog_selectserverhist = false;

	// Finish up and map the window
	SetWindowName("RootSpy");
	SetIconName("RootSpy");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

}

//-------------------
// DoQuit
//-------------------
void rs_mainframe::DoQuit(void)
{
	cout<<"quitting ..."<<endl;
	SavePreferences();

	// This is supposed to return from the Run() method in "main()"
	// since we call SetReturnFromRun(true), but it doesn't seem to work.
	gApplication->Terminate(0);	
}

//-------------------
// DoTimer
//-------------------
void rs_mainframe::DoTimer(void)
{
	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.
	
	time_t now = time(NULL);
	
	// Check if there are any dialogs we need to delete
	if(delete_dialog_selectserverhist){
		delete dialog_selectserverhist;
		dialog_selectserverhist = NULL;
	}
	delete_dialog_selectserverhist = false;
	
	// Update server label if necessary
	if(selected_server){
		string s = selected_server->GetTitle();
		if(s!=RS_INFO->selected_server){
			selected_server->SetText(RS_INFO->selected_server.c_str());
		}
	}
	
	// Update histo label if necessary
	if(selected_hist){
		string s = selected_hist->GetTitle();
		if(s!=RS_INFO->selected_hist){
			selected_hist->SetText(RS_INFO->selected_hist.c_str());
		}
	}
	
	// Request update of histogram if necessary
	if(auto_refresh->GetState()==kButtonDown){
		if(((now-last_hist_requested) >= delay_time) || (last_hnamepath_requested!=RS_INFO->selected_hist)){
			DoUpdate();
		}
	}
	
	// Update histo if necessary
	RS_INFO->Lock();
	if(RS_INFO->latest_hist != last_hist_plotted){
		if(RS_INFO->latest_hist){
			canvas->cd();
			RS_INFO->latest_hist->Draw();
			canvas->Update();
			last_hist_plotted = RS_INFO->latest_hist;
			string timestr = ctime(&RS_INFO->latest_hist_received_time);
			timestr[timestr.length()-1] = ' '; // replace carriage return
			retrieved_lab->SetText(timestr.c_str());
		}
	}
	RS_INFO->Unlock();

	last_called = now;
}

//-------------------
// DoSelectServerHist
//-------------------
void rs_mainframe::DoSelectServerHist(void)
{
	if(!dialog_selectserverhist){
		dialog_selectserverhist = new Dialog_SelectServerHist(gClient->GetRoot(), 10, 10);
	}else{
		dialog_selectserverhist->RaiseWindow();
		dialog_selectserverhist->RequestFocus();
	}
}

//-------------------
// DoSelectDelay
//-------------------
void rs_mainframe::DoSelectDelay(Int_t index)
{
	_DBG_<<"Setting auto-refresh delay to "<<index<<"seconds"<<endl;
	delay_time = (time_t)index;
}

//-------------------
// DoUpdate
//-------------------
void rs_mainframe::DoUpdate(void)
{
	// Send a request to the server for an updated histogram

	RS_INFO->Lock();
	if(RS_INFO->selected_hist.find(':')!=string::npos){ // make sure a histogram name is set
		RS_CMSG->RequestHistogram(RS_INFO->selected_server, RS_INFO->selected_hist);
		last_hist_requested = time(NULL);
		last_hnamepath_requested = RS_INFO->selected_hist;
	}
	RS_INFO->Unlock();
}

//-------------------
// ReadPreferences
//-------------------
void rs_mainframe::ReadPreferences(void)
{
	// Preferences file is "${HOME}/.RootSys"
	const char *home = getenv("HOME");
	if(!home)return;
	
	// Try and open file
	string fname = string(home) + "/.RootSys";
	ifstream ifs(fname.c_str());
	if(!ifs.is_open())return;
	cout<<"Reading preferences from \""<<fname<<"\" ..."<<endl;
	
	// Loop over lines
	char line[1024];
	while(!ifs.eof()){
		ifs.getline(line, 1024);
		if(strlen(line)==0)continue;
		if(line[0] == '#')continue;
		string str(line);
		
		// Break line into tokens
		vector<string> tokens;
		string buf; // Have a buffer string
		stringstream ss(str); // Insert the string into a stream
		while (ss >> buf)tokens.push_back(buf);
		if(tokens.size()<1)continue;

#if 0		
		// Check first token to decide what to do
		if(tokens[0] == "checkbutton"){
			if(tokens.size()!=4)continue; // should be of form "checkbutton name = value" with white space on either side of the "="
			map<string, TGCheckButton*>::iterator it = checkbuttons.find(tokens[1]);
			if(it != checkbuttons.end()){
				if(tokens[3] == "on")(it->second)->SetState(kButtonDown);
			}
		}
		
		if(tokens[0] == "DTrackCandidate"){
			if(tokens.size()!=3)continue; // should be of form "DTrackCandidate = tag" with white space on either side of the "="
			default_candidate = tokens[2];
		}

		if(tokens[0] == "DTrack"){
			if(tokens.size()!=3)continue; // should be of form "DTrack = tag" with white space on either side of the "="
			default_track = tokens[2];
		}

		if(tokens[0] == "DParticle"){
			if(tokens.size()!=3)continue; // should be of form "DParticle = tag" with white space on either side of the "="
			default_track = tokens[2];
		}

		if(tokens[0] == "Reconstructed"){
			if(tokens.size()!=3)continue; // should be of form "Reconstructed = Factory:tag" with white space on either side of the "="
			default_reconstructed = tokens[2];
		}
#endif		
	}
	
	// close file
	ifs.close();
}

//-------------------
// SavePreferences
//-------------------
void rs_mainframe::SavePreferences(void)
{
	// Preferences file is "${HOME}/.RootSys"
	const char *home = getenv("HOME");
	if(!home)return;
	
	// Try deleting old file and creating new file
	string fname = string(home) + "/.RootSys";
	unlink(fname.c_str());
	ofstream ofs(fname.c_str());
	if(!ofs.is_open()){
		cout<<"Unable to create preferences file \""<<fname<<"\"!"<<endl;
		return;
	}
	
	// Write header
	time_t t = time(NULL);
	ofs<<"##### RootSys preferences file ###"<<endl;
	ofs<<"##### Auto-generated on "<<ctime(&t)<<endl;
	ofs<<endl;

#if 0
	// Write all checkbuttons that are "on"
	map<string, TGCheckButton*>::iterator iter;
	for(iter=checkbuttons.begin(); iter!=checkbuttons.end(); iter++){
		TGCheckButton *but = iter->second;
		if(but->GetState() == kButtonDown){
			ofs<<"checkbutton "<<(iter->first)<<" = on"<<endl;
		}
	}
	ofs<<endl;

	ofs<<"DTrackCandidate = "<<(candidatesfactory->GetTextEntry()->GetText())<<endl;
	ofs<<"DTrack = "<<(tracksfactory->GetTextEntry()->GetText())<<endl;
	ofs<<"DParticle = "<<(particlesfactory->GetTextEntry()->GetText())<<endl;
	ofs<<"Reconstructed = "<<(reconfactory->GetTextEntry()->GetText())<<endl;
#endif

	ofs<<endl;
	ofs.close();
	cout<<"Preferences written to \""<<fname<<"\""<<endl;
}

//-------------------
// CreateGUI
//-------------------
void rs_mainframe::CreateGUI(void)
{
	// Use the "color wheel" rather than the classic palette.
	TColor::CreateColorWheel();
	
	TGMainFrame *fMainFrame1435 = this;

	//======================= The following was copied for a macro made with TGuiBuilder ===============
	   // composite frame
   TGCompositeFrame *fMainFrame656 = new TGCompositeFrame(fMainFrame1435,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame657 = new TGCompositeFrame(fMainFrame656,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame658 = new TGCompositeFrame(fCompositeFrame657,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame659 = new TGCompositeFrame(fCompositeFrame658,833,700,kVerticalFrame);
//   fCompositeFrame659->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fCompositeFrame660 = new TGCompositeFrame(fCompositeFrame659,833,700,kVerticalFrame);
//   fCompositeFrame660->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fCompositeFrame661 = new TGCompositeFrame(fCompositeFrame660,833,700,kVerticalFrame);
//   fCompositeFrame661->SetLayoutBroken(kTRUE);

   // vertical frame
   TGVerticalFrame *fVerticalFrame662 = new TGVerticalFrame(fCompositeFrame661,708,640,kVerticalFrame);
//   fVerticalFrame662->SetLayoutBroken(kTRUE);

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame663 = new TGHorizontalFrame(fVerticalFrame662,564,118,kHorizontalFrame);

   // "Current Histogram Info." group frame
   TGGroupFrame *fGroupFrame664 = new TGGroupFrame(fHorizontalFrame663,"Current Histogram Info.",kHorizontalFrame | kRaisedFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame665 = new TGVerticalFrame(fGroupFrame664,62,60,kVerticalFrame);
   TGLabel *fLabel666 = new TGLabel(fVerticalFrame665,"Server:");
   fLabel666->SetTextJustify(kTextRight);
   fLabel666->SetMargins(0,0,0,0);
   fLabel666->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel666, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
   TGLabel *fLabel667 = new TGLabel(fVerticalFrame665,"Histogram:");
   fLabel667->SetTextJustify(kTextRight);
   fLabel667->SetMargins(0,0,0,0);
   fLabel667->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel667, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
   TGLabel *fLabel668 = new TGLabel(fVerticalFrame665,"Retrieved:");
   fLabel668->SetTextJustify(kTextRight);
   fLabel668->SetMargins(0,0,0,0);
   fLabel668->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel668, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame665, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame669 = new TGVerticalFrame(fGroupFrame664,29,60,kVerticalFrame);
   TGLabel *fLabel670 = new TGLabel(fVerticalFrame669,"------------------------------");
   fLabel670->SetTextJustify(kTextLeft);
   fLabel670->SetMargins(0,0,0,0);
   fLabel670->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel670, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel671 = new TGLabel(fVerticalFrame669,"-------------------------------------------------------");
   fLabel671->SetTextJustify(kTextLeft);
   fLabel671->SetMargins(0,0,0,0);
   fLabel671->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel671, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel672 = new TGLabel(fVerticalFrame669,"------------------------------");
   fLabel672->SetTextJustify(kTextLeft);
   fLabel672->SetMargins(0,0,0,0);
   fLabel672->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel672, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame669, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame673 = new TGVerticalFrame(fGroupFrame664,97,78,kVerticalFrame);
   TGTextButton *fTextButton674 = new TGTextButton(fVerticalFrame673,"Change Server/Histo");
   fTextButton674->SetTextJustify(36);
   fTextButton674->SetMargins(0,0,0,0);
   fTextButton674->SetWrapLength(-1);
   fTextButton674->Resize(93,22);
   fVerticalFrame673->AddFrame(fTextButton674, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
//   TGTextButton *fTextButton675 = new TGTextButton(fVerticalFrame673,"Change Histo.");
//   fTextButton675->SetTextJustify(36);
//   fTextButton675->SetMargins(0,0,0,0);
//   fTextButton675->SetWrapLength(-1);
//   fTextButton675->Resize(87,22);
//   fVerticalFrame673->AddFrame(fTextButton675, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGTextButton *fTextButton676 = new TGTextButton(fVerticalFrame673,"Update");
   fTextButton676->SetTextJustify(36);
   fTextButton676->SetMargins(0,0,0,0);
   fTextButton676->SetWrapLength(-1);
   fTextButton676->Resize(87,22);
   fVerticalFrame673->AddFrame(fTextButton676, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame673, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->SetLayoutManager(new TGHorizontalLayout(fGroupFrame664));
   fGroupFrame664->Resize(232,114);
   fHorizontalFrame663->AddFrame(fGroupFrame664, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // "fGroupFrame746" group frame
   TGGroupFrame *fGroupFrame746 = new TGGroupFrame(fHorizontalFrame663,"Continuous Update options",kHorizontalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame678 = new TGVerticalFrame(fGroupFrame746,144,63,kVerticalFrame);
   TGCheckButton *fCheckButton679 = new TGCheckButton(fVerticalFrame678,"Auto-refresh");
   fCheckButton679->SetTextJustify(36);
   fCheckButton679->SetMargins(0,0,0,0);
   fCheckButton679->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton679, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGCheckButton *fCheckButton680 = new TGCheckButton(fVerticalFrame678,"loop over all servers");
   fCheckButton680->SetTextJustify(36);
   fCheckButton680->SetMargins(0,0,0,0);
   fCheckButton680->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton680, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGCheckButton *fCheckButton681 = new TGCheckButton(fVerticalFrame678,"loop over all hists");
   fCheckButton681->SetTextJustify(36);
   fCheckButton681->SetMargins(0,0,0,0);
   fCheckButton681->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton681, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame746->AddFrame(fVerticalFrame678, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame682 = new TGHorizontalFrame(fGroupFrame746,144,26,kHorizontalFrame);
   TGLabel *fLabel683 = new TGLabel(fHorizontalFrame682,"delay:");
   fLabel683->SetTextJustify(36);
   fLabel683->SetMargins(0,0,0,0);
   fLabel683->SetWrapLength(-1);
   fHorizontalFrame682->AddFrame(fLabel683, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   ULong_t ucolor;        // will reflect user color changes
   gClient->GetColorByName("#ffffff",ucolor);

   // combo box
   TGComboBox *fComboBox684 = new TGComboBox(fHorizontalFrame682,"-",-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   fComboBox684->AddEntry("0s",0);
   fComboBox684->AddEntry("1s",1);
   fComboBox684->AddEntry("2s",2);
   fComboBox684->AddEntry("4s",4);
   fComboBox684->AddEntry("10s ",10);
   fComboBox684->Resize(50,22);
   fComboBox684->Select(-1);
   fHorizontalFrame682->AddFrame(fComboBox684, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame746->AddFrame(fHorizontalFrame682, new TGLayoutHints(kLHintsNormal));

   fGroupFrame746->SetLayoutManager(new TGHorizontalLayout(fGroupFrame746));
   fGroupFrame746->Resize(324,99);
   fHorizontalFrame663->AddFrame(fGroupFrame746, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fVerticalFrame662->AddFrame(fHorizontalFrame663, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fHorizontalFrame663->MoveResize(2,2,564,118);

   // embedded canvas
   TRootEmbeddedCanvas *fRootEmbeddedCanvas698 = new TRootEmbeddedCanvas(0,fVerticalFrame662,704,432);
   Int_t wfRootEmbeddedCanvas698 = fRootEmbeddedCanvas698->GetCanvasWindowId();
   TCanvas *c125 = new TCanvas("c125", 10, 10, wfRootEmbeddedCanvas698);
   fRootEmbeddedCanvas698->AdoptCanvas(c125);
   fVerticalFrame662->AddFrame(fRootEmbeddedCanvas698, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));
   fRootEmbeddedCanvas698->MoveResize(2,124,704,432);

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame1060 = new TGHorizontalFrame(fVerticalFrame662,704,82,kHorizontalFrame);

   // "fGroupFrame750" group frame
   TGGroupFrame *fGroupFrame711 = new TGGroupFrame(fHorizontalFrame1060,"cMsg Info.");
   TGLabel *fLabel1030 = new TGLabel(fGroupFrame711,"UDL = cMsg://localhost/cMsg/rootspy");
   fLabel1030->SetTextJustify(36);
   fLabel1030->SetMargins(0,0,0,0);
   fLabel1030->SetWrapLength(-1);
   fGroupFrame711->AddFrame(fLabel1030, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGTextButton *fTextButton1041 = new TGTextButton(fGroupFrame711,"modify");
   fTextButton1041->SetTextJustify(36);
   fTextButton1041->SetMargins(0,0,0,0);
   fTextButton1041->SetWrapLength(-1);
   fTextButton1041->Resize(97,22);
   fGroupFrame711->AddFrame(fTextButton1041, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame711->SetLayoutManager(new TGVerticalLayout(fGroupFrame711));
   fGroupFrame711->Resize(133,78);
   fHorizontalFrame1060->AddFrame(fGroupFrame711, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGTextButton *fTextButton1075 = new TGTextButton(fHorizontalFrame1060,"Quit");
   fTextButton1075->SetTextJustify(36);
   fTextButton1075->SetMargins(0,0,0,0);
   fTextButton1075->SetWrapLength(-1);
   fTextButton1075->Resize(97,22);
   fHorizontalFrame1060->AddFrame(fTextButton1075, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));

   fVerticalFrame662->AddFrame(fHorizontalFrame1060, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
   fHorizontalFrame1060->MoveResize(2,560,704,82);

   fCompositeFrame661->AddFrame(fVerticalFrame662, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fVerticalFrame662->MoveResize(2,2,708,640);

   fCompositeFrame660->AddFrame(fCompositeFrame661, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fCompositeFrame661->MoveResize(0,0,833,700);

   fCompositeFrame659->AddFrame(fCompositeFrame660, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fCompositeFrame660->MoveResize(0,0,833,700);

   fCompositeFrame658->AddFrame(fCompositeFrame659, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fCompositeFrame657->AddFrame(fCompositeFrame658, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fMainFrame656->AddFrame(fCompositeFrame657, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fMainFrame1435->AddFrame(fMainFrame656, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fMainFrame1435->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame1435->MapSubwindows();

   fMainFrame1435->Resize(fMainFrame1435->GetDefaultSize());
   fMainFrame1435->MapWindow();
   fMainFrame1435->Resize(833,700);
	//==============================================================================================

	// Connect GUI elements to methods
	TGTextButton* &quit = fTextButton1075;
	TGTextButton* &selectserver = fTextButton674;
	selected_server = fLabel670;
	selected_hist = fLabel671;
	retrieved_lab = fLabel672;
	delay = fComboBox684;
	canvas = fRootEmbeddedCanvas698->GetCanvas();
	auto_refresh = fCheckButton679;
	loop_over_servers = fCheckButton680;
	loop_over_hists = fCheckButton681;
	TGTextButton* &update = fTextButton676;
	TGTextButton* &modify = fTextButton1041;
	
	quit->Connect("Clicked()","rs_mainframe", this, "DoQuit()");
	selectserver->Connect("Clicked()","rs_mainframe", this, "DoSelectServerHist()");
	update->Connect("Clicked()","rs_mainframe", this, "DoUpdate()");

	delay->Select(4, kTRUE);
	delay->GetTextEntry()->SetText("4s");
	delay->Connect("Selected(Int_t)","rs_mainframe", this, "DoSelectDelay(Int_t)");
	
	loop_over_servers->SetEnabled(kFALSE);
	loop_over_hists->SetEnabled(kFALSE);
	modify->SetEnabled(kFALSE);
	
	canvas->SetFillColor(kWhite);
}


