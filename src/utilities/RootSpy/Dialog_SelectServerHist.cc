// $Id$
//
//    File: Dialog_SelectServerHist.cc
// Created: Sat Aug 29 09:02:35 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_SelectServerHist.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"

#include <TApplication.h>

#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TTimer.h>

//---------------------------------
// Dialog_SelectServerHist    (Constructor)
//---------------------------------
Dialog_SelectServerHist::Dialog_SelectServerHist(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	// Define all of the of the graphics objects. 
	CreateGUI();

	// Set up timer to call the DoTimer() method repeatedly
	// so events can be automatically advanced.
	timer = new TTimer();
	timer->Connect("Timeout()", "Dialog_SelectServerHist", this, "DoTimer()");
	sleep_time = 500;
	timer->Start(sleep_time, kFALSE);

	time_t now = time(NULL);
	last_called = now;
	last_ping_time = now;
	
	DoTimer();

	// Finish up and map the window
	SetWindowName("RootSpy Select Server/Histogram");
	SetIconName("SelectHist");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

}

//---------------------------------
// ~Dialog_SelectServerHist    (Destructor)
//---------------------------------
Dialog_SelectServerHist::~Dialog_SelectServerHist()
{

}

//-------------------
// DoTimer
//-------------------
void Dialog_SelectServerHist::DoTimer(void)
{
	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.
	
	time_t now = time(NULL);
	
	vector<string> servers;
	
	// Check for missing servers
	RS_INFO->Lock();
	map<string,time_t>::iterator iter=RS_INFO->servers.begin();
	for(; iter!=RS_INFO->servers.end(); iter++){
		if((now-iter->second<last_ping_time) && ((last_ping_time-now)>5)){
			cout<<"server: "<<iter->first<<" has gone away"<<endl;
			RS_INFO->servers.erase(iter);
		}else{
			servers.push_back(iter->first);
		}
	}
	RS_INFO->Unlock();
	
	// Ping servers occasionally to make sure our list is up-to-date
	if(now-last_ping_time >= 3){
		RS_CMSG->PingServers();
		last_ping_time = now;
	}
	
	// Check to see if the list of servers differs from last time. If so,
	// we'll need to update the combo box
	sort(servers.begin(), servers.end());
	bool servers_changed = last_servers.size() != servers.size();
	for(unsigned int i=0; i<servers.size(); i++){
		if(servers_changed)break;
		if(servers[i] != last_servers[i])servers_changed=true;
	}
	
	// Refill combobox if needed
	if(servers_changed){

		// Get current setting first so we may maintain it
		string selected = "";
		TGTextEntry *entry = server->GetTextEntry();
		if(server->GetNumberOfEntries()){
			if(entry)selected = entry->GetText();
		}else{
			if(entry)entry->SetText("");
		}
	
		server->RemoveAll();
		for(unsigned int i=0; i<servers.size(); i++){
			const char *s = servers[i].c_str();
			server->AddEntry(s, i);
			
			if(servers[i]==selected || (selected=="" && i==0)){ 
				if(entry)entry->SetText(s);
				server->Select(i, kTRUE);
			}
		}
		last_servers = servers;
	}


	// Ping server occasionally to make sure our histogram list is up-to-date
	if(now-last_hist_time >= 3){
		string servername = server->GetListBox()->GetEntry(server->GetSelected())->GetTitle();
		if(servername!=""){
			RS_CMSG->RequestHists(servername);
			last_hist_time = now;
		}
	}
	
	// Check to see if the list of histograms differs from last time. If so,
	// we'll need to update the combo box. First, get the latest list of hinfo_t
	// objects for this server.
	RS_INFO->Lock();
	string servername = server->GetSelectedEntry()->GetTitle();
	vector<rs_info::hinfo_t> hists;
	map<string,vector<rs_info::hinfo_t> >::iterator hi = RS_INFO->hists.find(servername);
	if(hi!=RS_INFO->hists.end())hists = hi->second;
	RS_INFO->Unlock();
	
	// Make list of path+name strings for all histos and sort them
	vector<string> hnamepaths;
	for(unsigned int i=0; i<hists.size(); i++){
		string hnamepath = hists[i].path;
		if(hnamepath[hnamepath.length()-1]!='/')hnamepath += "/";
		hnamepath += hists[i].name;
		hnamepaths.push_back(hnamepath);
	}
	sort(hnamepaths.begin(), hnamepaths.end());

	// Check if list of histos has changed
	bool hists_changed = last_hnamepaths.size() != hnamepaths.size();
	for(unsigned int i=0; i<hnamepaths.size(); i++){
		if(hists_changed)break;
		if(hnamepaths[i] != last_hnamepaths[i])hists_changed=true;
	}
	
	// Refill combobox if needed
	if(hists_changed){

		// Get current setting first so we may maintain it
		string selected = "";
		TGTextEntry *entry = hist->GetTextEntry();
		if(hist->GetNumberOfEntries()){
			if(entry)selected = entry->GetText();
		}else{
			if(entry)entry->SetText("");
		}
	
		hist->RemoveAll();
		for(unsigned int i=0; i<hnamepaths.size(); i++){
			hist->AddEntry(hnamepaths[i].c_str(), i);
			
			if(hnamepaths[i]==selected || (selected=="" && i==0)){ 
				if(entry)entry->SetText(hnamepaths[i].c_str());
				hist->Select(i, kTRUE);
			}
		}

		last_hnamepaths = hnamepaths;
	}

	last_called = now;
}

//---------------------------------
// DoSelectServer
//---------------------------------
void Dialog_SelectServerHist::DoSelectServer(Int_t index)
{
	string servername = server->GetListBox()->GetEntry(index)->GetTitle();
	RS_CMSG->RequestHists(servername);
	last_hist_time = time(NULL);
}

//---------------------------------
// DoOK
//---------------------------------
void Dialog_SelectServerHist::DoOK(void)
{
	RS_INFO->selected_server = server->GetSelectedEntry()->GetTitle();
	RS_INFO->selected_hist = hist->GetSelectedEntry()->GetTitle();

	DoCancel();
}

//---------------------------------
// DoCancel
//---------------------------------
void Dialog_SelectServerHist::DoCancel(void)
{
	timer->Stop();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_selectserverhist = true;
}

//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_SelectServerHist::CreateGUI(void)
{
	TGMainFrame *fMainFrame1005 = this;

	//======================= The following was copied for a macro made with TGuiBuilder ===============
   // main frame
   //TGMainFrame *fMainFrame1005 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
   //fMainFrame1005->SetLayoutBroken(kTRUE);

   // "fGroupFrame515" group frame
   TGGroupFrame *fGroupFrame515 = new TGGroupFrame(fMainFrame1005,"Select Server/Histo");

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame540 = new TGHorizontalFrame(fGroupFrame515,255,26,kHorizontalFrame);

   ULong_t ucolor;        // will reflect user color changes
   gClient->GetColorByName("#ffffff",ucolor);

   // combo box
   TGComboBox *fComboBox518 = new TGComboBox(fHorizontalFrame540,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   fComboBox518->Resize(192,22);
   fComboBox518->Select(-1);
   fHorizontalFrame540->AddFrame(fComboBox518, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel582 = new TGLabel(fHorizontalFrame540,"Server:");
   fLabel582->SetTextJustify(36);
   fLabel582->SetMargins(0,0,0,0);
   fLabel582->SetWrapLength(-1);
   fHorizontalFrame540->AddFrame(fLabel582, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame515->AddFrame(fHorizontalFrame540, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame553 = new TGHorizontalFrame(fGroupFrame515,255,26,kHorizontalFrame);

   gClient->GetColorByName("#ffffff",ucolor);

   // combo box
   TGComboBox *fComboBox560 = new TGComboBox(fHorizontalFrame553,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   fComboBox560->Resize(400,22);
   fComboBox560->Select(-1);
   fHorizontalFrame553->AddFrame(fComboBox560, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel587 = new TGLabel(fHorizontalFrame553,"Histo:");
   fLabel587->SetTextJustify(36);
   fLabel587->SetMargins(0,0,0,0);
   fLabel587->SetWrapLength(-1);
   fHorizontalFrame553->AddFrame(fLabel587, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame515->AddFrame(fHorizontalFrame553, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame596 = new TGHorizontalFrame(fGroupFrame515,264,26,kHorizontalFrame);
   TGTextButton *fTextButton601 = new TGTextButton(fHorizontalFrame596,"Cancel");
   fTextButton601->SetTextJustify(36);
   fTextButton601->SetMargins(0,0,0,0);
   fTextButton601->SetWrapLength(-1);
   fTextButton601->Resize(90,22);
   fHorizontalFrame596->AddFrame(fTextButton601, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGTextButton *fTextButton606 = new TGTextButton(fHorizontalFrame596,"OK");
   fTextButton606->SetTextJustify(36);
   fTextButton606->SetMargins(0,0,0,0);
   fTextButton606->SetWrapLength(-1);
   fTextButton606->Resize(90,22);
   fHorizontalFrame596->AddFrame(fTextButton606, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame515->AddFrame(fHorizontalFrame596, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame515->SetLayoutManager(new TGVerticalLayout(fGroupFrame515));
   fGroupFrame515->Resize(291,122);
   fMainFrame1005->AddFrame(fGroupFrame515, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fGroupFrame515->MoveResize(32,32,291,122);

   fMainFrame1005->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame1005->MapSubwindows();

   fMainFrame1005->Resize(fMainFrame1005->GetDefaultSize());
   fMainFrame1005->MapWindow();
   fMainFrame1005->Resize(490,372);
	//==============================================================================================

	// Connect GUI elements to methods
	TGTextButton* &ok = fTextButton606;
	TGTextButton* &cancel = fTextButton601;
	this->server = fComboBox518;
	this->hist = fComboBox560;
	
	ok->Connect("Clicked()","Dialog_SelectServerHist", this, "DoOK()");
	cancel->Connect("Clicked()","Dialog_SelectServerHist", this, "DoCancel()");
	this->server->Connect("Selected(Int_t)","Dialog_SelectServerHist", this, "DoSelectServer(Int_t)");
}
