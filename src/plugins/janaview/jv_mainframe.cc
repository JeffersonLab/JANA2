// $Id$
//
//    File: jv_mainframe.cc
// Created: Fri Oct  3 08:12:01 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#include "jv_mainframe.h"
#include "JEventProcessor_janaview.h"

//---------------------------------
// jv_mainframe    (Constructor)
//---------------------------------
jv_mainframe::jv_mainframe(const TGWindow *p, UInt_t w, UInt_t h,  bool build_gui):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	CreateGUI();
	
	SetWindowName("JANA Viewer");
	SetIconName("JANA Viewer");
	MapSubwindows();
	MapWindow();
	this->SetWidth(1000);
	Redraw(this);
	
	timer = new TTimer();
	timer->Connect("Timeout()", "jv_mainframe", this, "DoTimer()");
	sleep_time = 250;
	timer->Start(sleep_time, kFALSE);

}

//---------------------------------
// ~jv_mainframe    (Destructor)
//---------------------------------
jv_mainframe::~jv_mainframe()
{

}

//-------------------
// CloseWindow
//-------------------
void jv_mainframe::CloseWindow(void)
{
	DeleteWindow();
	gApplication->Terminate(0);
}

//-------------------
// HandleMenu
//-------------------
void jv_mainframe::HandleMenu(Int_t id)
{
   // Handle menu items.

  //cout << "in HandleMenu(" << id << ")" << endl;

   switch (id) {

   case M_FILE_OPEN:
     break;

   case M_FILE_SAVE:
     break;
	
   case M_FILE_NEW_CONFIG:
     break;

   case M_FILE_OPEN_CONFIG:
     break;

   case M_FILE_SAVE_CONFIG:
     break;

   case M_FILE_SAVE_AS_CONFIG:
     break;

   case M_FILE_EXIT: 
     DoQuit();       
     break;

   case M_TOOLS_MACROS:
     break;

   case M_TOOLS_TBROWSER:
     break;

   case M_VIEW_NEW_TAB:
	   break;

   case M_VIEW_REMOVE_TAB:
	   break;

   }
}

//-------------------
// DoTimer
//-------------------
void jv_mainframe::DoTimer(void)
{
	/// This method gets called every 250ms. It doesn't do anything
	/// itself, but it does cause the screen to get updated. Without
	/// this, the screen does not seem to update on it's own. It needs
	/// at least a mouse movement over the screen to cause an update.
	/// (This was observed on both Linux and Mac OS X).
}

//-------------------
// DoQuit
//-------------------
void jv_mainframe::DoQuit(void)
{
	cout<<"quitting ..."<<endl;
	gApplication->Terminate(0);	
}

//-------------------
// DoNext
//-------------------
void jv_mainframe::DoNext(void)
{
	JEP->NextEvent();
}

//-------------------
// DoDelayedSelectObjectType
//-------------------
void jv_mainframe::DoDelayedSelectObjectType(void)
{
	/// This is here to allow us to defer selection of the object
	/// type box after UpdateObjectTypeList is called. This happens
	/// when a new event has been read in. For some reason, calling
	/// DoSelectObjectType directly from UpdateObjectTypeList causes
	/// bad behavior in the GUI. Hence, calling this routine a few
	/// milliseconds after UpdateObjectTypeList has returned.
	DoSelectObjectType(delayed_object_type_id);
	lbObjectTypes->Select(delayed_object_type_id);
	lbObjectTypes->SetTopEntry(delayed_object_type_id);
	
	Redraw(lbObjectTypes);
}

//-------------------
// DoSelectObjectType
//-------------------
void jv_mainframe::DoSelectObjectType(Int_t id)
{
	// Clear listboxs of associated objects
	lbObjects->RemoveAll();
	lbAssociatedObjects->RemoveAll();
	lbAssociatedToObjects->RemoveAll();
	Redraw(lbObjects);
	Redraw(lbAssociatedObjects);
	Redraw(lbAssociatedToObjects);

	TGLBEntry *e = lbObjectTypes->GetEntry(id);
	if(!e){
		_DBG_<<"Unable to find factory for id:"<<id<< endl;
		return;
	}

	// Update label
	string nametag = e->GetTitle();
	lObjectType->SetTitle(strdup(nametag.c_str()));
	lObjectType->Resize();
	
	// Get factory name and tag
	string name = nametag;
	string tag = "";
	size_t pos=name.find(":");
	if(pos != name.npos){
		tag = name.substr(pos+1);
		name = name.substr(0, pos);
	}

	// Get factory and list of objects
	JEP->Lock();
	JFactory_base *fac = JEP->loop->GetFactory(name, tag.c_str());
	if(!fac){
		JEP->Unlock();
		_DBG_<<"Unable to find factory for name=\""<<name<<"\"  tag=\"" << tag << "\"" << endl;
		return;
	}

	// Here we have to go to some trouble to make sure the event source's GetObjects
	// method is called. The JFactory::Get method called below will only try to 
	// generate the objects using the factory algorithm so objects coming from the
	// source will not be necessarily be there unless we force the call here.	
	JEvent &jevent = JEP->loop->GetJEvent();
	JEventSource *source = jevent.GetJEventSource();
	source->GetObjects(jevent, fac);

	// Get pointers from factory
	vobjs = fac->Get();

	// Copy list of objects into listbox
	lbObjects->RemoveAll();
	for(uint32_t i=0; i<vobjs.size(); i++){
		char str[256];
		sprintf(str, "0x%016lx %s", (unsigned long)vobjs[i], ((JObject*)vobjs[i])->GetName().c_str());
		lbObjects->AddEntry(str, i+1);
	}
	
	JEP->MakeCallGraph();
	
	JEP->Unlock();
	Redraw(lbObjects);
}

//-------------------
// DoSelectObject
//-------------------
void jv_mainframe::DoSelectObject(Int_t id)
{
	// Clear listboxs of associated objects
	lbAssociatedObjects->RemoveAll();
	lbAssociatedToObjects->RemoveAll();
	Redraw(lbAssociatedObjects);
	Redraw(lbAssociatedToObjects);

	// Get pointer to selected object as a JObject
	Int_t idx = id-1;
	if(idx<0 || idx>=(Int_t)vobjs.size()) return;	
	JObject *obj = (JObject*)vobjs[idx];
	
	UpdateObjectValues(obj);

	// Get associated objects
	aobjs.clear();
	obj->GetT(aobjs);
	for(uint32_t i=0; i<aobjs.size(); i++){
		char str[256];
		sprintf(str, "0x%016lx %s", (unsigned long)aobjs[i], aobjs[i]->GetName().c_str());
		lbAssociatedObjects->AddEntry(str, i+1);
	}
	Redraw(lbAssociatedObjects);
	
	// Get associated to objects
	a2objs.clear();
	JEP->GetAssociatedTo(obj, a2objs);
	for(uint32_t i=0; i<a2objs.size(); i++){
		char str[256];
		sprintf(str, "0x%016lx %s", (unsigned long)a2objs[i], a2objs[i]->GetName().c_str());
		lbAssociatedToObjects->AddEntry(str, i+1);
	}
	Redraw(lbAssociatedToObjects);
}

//-------------------
// DoSelectAssociatedObject
//-------------------
void jv_mainframe::DoSelectAssociatedObject(Int_t id)
{
	// Get pointer to selected object as a JObject
	Int_t idx = id-1;
	if(idx<0 || idx>=(Int_t)aobjs.size()) return;	
	JObject *obj = (JObject*)aobjs[idx];
	
	UpdateObjectValues(obj);
}

//-------------------
// DoSelectAssociatedToObject
//-------------------
void jv_mainframe::DoSelectAssociatedToObject(Int_t id)
{
	// Get pointer to selected object as a JObject
	Int_t idx = id-1;
	if(idx<0 || idx>=(Int_t)a2objs.size()) return;	
	JObject *obj = (JObject*)a2objs[idx];
	
	UpdateObjectValues(obj);
}

//-------------------
// DoDoubleClickAssociatedObject
//-------------------
void jv_mainframe::DoDoubleClickAssociatedObject(Int_t id)
{
	// Get pointer to selected object as a JObject
	Int_t idx = id-1;
	if(idx<0 || idx>=(Int_t)aobjs.size()) return;	
	JObject *obj = (JObject*)aobjs[idx];
	
	SelectNewObject(obj);
}

//-------------------
// DoDoubleClickAssociatedToObject
//-------------------
void jv_mainframe::DoDoubleClickAssociatedToObject(Int_t id)
{
	// Get pointer to selected object as a JObject
	Int_t idx = id-1;
	if(idx<0 || idx>=(Int_t)a2objs.size()) return;	
	JObject *obj = (JObject*)a2objs[idx];
	
	SelectNewObject(obj);
}

//-------------------
// HandleConfigureNotify
//-------------------
Bool_t jv_mainframe::HandleConfigureNotify(Event_t *event)
{
	/// This gets called when the window is resized (but also for other things)
	/// It is here so we can update the CallGraph tab when the window is resized.

	if(JEP) JEP->MakeCallGraph();
	return this->TGMainFrame::HandleConfigureNotify(event);
}


//==============================================================================

//-------------------
// UpdateInfo
//-------------------
void jv_mainframe::UpdateInfo(string source, int run, int event)
{
	lSource->SetText(source.c_str());

	char str[256];
	sprintf(str, "%d", run);
	lRun->SetText(str);
	sprintf(str, "%d", event);
	lEvent->SetText(str);
	
	Redraw(lSource);
}

//-------------------
// UpdateObjectTypeList
//-------------------
void jv_mainframe::UpdateObjectTypeList(vector<JVFactoryInfo> &facinfo)
{
	// Check if an entry is already selected and remember it so we can re-select it
	string selected_nametag = "";
	Int_t id = lbObjectTypes->GetSelected();
	if(id>0 && id<=lbObjectTypes->GetNumberOfEntries()){
		TGLBEntry *e = lbObjectTypes->GetEntry(id);
		if(e) selected_nametag = e->GetTitle();
	}

	// Clean out old list and create new one of nametags
	lbObjectTypes->RemoveAll();
	objtypes.clear();

	// First, copy all nametags into a set so they will be ordered alphabetically
	set<string> nametags;
	for(uint32_t i=0; i<facinfo.size(); i++){
		JVFactoryInfo &finfo = facinfo[i];
		nametags.insert(finfo.nametag);
	}

	// Fill in listbox and keep corresponding vector of objects
	set<string>::iterator iter = nametags.begin();
	id=1;
	uint32_t selected_id = 0;
	for(; iter!=nametags.end(); iter++, id++){
		string nametag = *iter;
		lbObjectTypes->AddEntry(nametag.c_str(), id);
		objtypes.push_back(nametag);
		if(nametag == selected_nametag) selected_id = id;
	}
	
	Redraw(lbObjectTypes);

	if(selected_id>0){
		delayed_object_type_id = selected_id;
		TTimer::SingleShot(1000, "jv_mainframe", this, "DoDelayedSelectObjectType()");
	}	
}

//-------------------
// UpdateObjectValues
//-------------------
void jv_mainframe::UpdateObjectValues(JObject *obj)
{
	char title[256];
	sprintf(title, "0x%016lx : %s", (unsigned long)obj, obj->GetName().c_str());
	lObjectValue->SetTitle(title);
	lObjectValue->Resize();

	vector<pair<string,string> > items;
	obj->toStrings(items);
	
	// Find maximum label width
	uint32_t max_width = 0;
	for(uint32_t i=0; i<items.size(); i++){
		uint32_t width = items[i].first.length();
		if(width>max_width) max_width = width;
	}

	// We must go to some trouble to use a fixed-width font in the
	// listbox, but this is needed to get things to line up. This
	// was copied from an example here:
	// http://root.cern.ch/phpBB3/viewtopic.php?p=9281
	const TGFont *ufont;         // will reflect user font changes
	ufont = gClient->GetFont("-adobe-courier-medium-r-*-*-12-*-*-*-*-*-iso8859-1");
	if (!ufont) ufont = fClient->GetResourcePool()->GetDefaultFont();

	// graphics context changes
	GCValues_t val;
	val.fMask = kGCFont;
	val.fFont = ufont->GetFontHandle();
	TGGC *uGC = gClient->GetGC(&val, kTRUE);

	// Replace contents of listbox
	lbObjectValues->RemoveAll();
	for(uint32_t i=0; i<items.size(); i++){
		string &lab = items[i].first;
		string &val = items[i].second;
		string line = lab + string(max_width+1 - lab.length(), ' ') + " : " + val;
		
		TGTextLBEntry *entry = new TGTextLBEntry(lbObjectValues->GetContainer(), new TGString(line.c_str()), i+1, uGC->GetGC(), ufont->GetFontStruct());
		lbObjectValues->AddEntry((TGLBEntry *)entry, new TGLayoutHints(kLHintsTop | kLHintsLeft));
	}
	Redraw(lbObjectValues);
}

//-------------------
// SelectNewObject
//-------------------
void jv_mainframe::SelectNewObject(void *vobj)
{
	JObject *obj = (JObject*)vobj;
	string nametag = obj->GetNameTag();
	for(uint32_t i=0; i<objtypes.size(); i++){
		if(objtypes[i] != nametag) continue;
		
		DoSelectObjectType(i+1);
		lbObjectTypes->Select(i+1);
		lbObjectTypes->SetTopEntry(i+1);
		
		break;
	}
	
	for(uint32_t i=0; i<objtypes.size(); i++){
		if(vobjs[i] != vobj) continue;
		
		DoSelectObject(i+1);
		lbObjects->Select(i+1);
		lbObjects->SetTopEntry(i+1);
		
		break;
	}
}

//-------------------
// Redraw
//-------------------
void jv_mainframe::Redraw(TGFrame *f)
{
	// This unfortunate business seems to be needed to get the listbox to redraw itself
	TGDimension dim = f->GetSize();
	dim.fWidth+=1;
	f->Resize(dim);
	dim.fWidth-=1;
	f->Resize(dim);
}

//==============================================================================

//-------------------
// AddLabel
//-------------------
TGLabel* jv_mainframe::AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints)
{
	TGLabel *lab = new TGLabel(frame, text.c_str());
	lab->SetTextJustify(mode);
	lab->SetMargins(0,0,0,0);
	lab->SetWrapLength(-1);
	frame->AddFrame(lab, new TGLayoutHints(hints,2,2,2,2));

	return lab;
}

//-------------------
// AddNamedLabel
//-------------------
TGLabel* jv_mainframe::AddNamedLabel(TGCompositeFrame* frame, string title, Int_t mode, ULong_t hints)
{
	TGHorizontalFrame *f = new TGHorizontalFrame(frame);
	AddLabel(f, title, kTextRight);
	TGLabel *lab = AddLabel(f, "---------", mode);
	frame->AddFrame(f, new TGLayoutHints(kLHintsExpandX,2,2,2,2));

	return lab;
}

//-------------------
// AddButton
//-------------------
TGTextButton* jv_mainframe::AddButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGTextButton *b = new TGTextButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	b->Resize(100,22);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddCheckButton
//-------------------
TGCheckButton* jv_mainframe::AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGCheckButton *b = new TGCheckButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddPictureButton
//-------------------
TGPictureButton* jv_mainframe::AddPictureButton(TGCompositeFrame* frame, string picture, string tooltip, ULong_t hints)
{
	TGPictureButton *b = new TGPictureButton(frame, gClient->GetPicture(picture.c_str()));
	if(tooltip.length()>0) b->SetToolTipText(tooltip.c_str());
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddSpacer
//-------------------
TGFrame* jv_mainframe::AddSpacer(TGCompositeFrame* frame, UInt_t w, UInt_t h, ULong_t hints)
{
	/// Add some empty space. Usually, you'll only want to set w or h to
	/// reserve width or height pixels and set the other to "1".

	TGFrame *f =  new TGFrame(frame, w, h);
	frame->AddFrame(f, new TGLayoutHints(hints ,2,2,2,2));
	
	return f;
}

//-------------------
// AddListBox
//-------------------
TGListBox* jv_mainframe::AddListBox(TGCompositeFrame* frame, string lab, UInt_t w, ULong_t hints)
{
	if(lab != "") AddLabel(frame, lab);
	TGListBox *lb = new TGListBox(frame);
	frame->AddFrame(lb, new TGLayoutHints(hints ,2,2,2,2));
	lb->SetWidth(w);
	
	return lb;
}


//-------------------
// CreateGUI
//-------------------
void jv_mainframe::CreateGUI(void)
{
	// Use the "color wheel" rather than the classic palette.
	TColor::CreateColorWheel();
	

   //==============================================================================================
   // make a menubar
   // Create menubar and popup menus. The hint objects are used to place
   // and group the different menu widgets with respect to eachother.

//	TGDockableFrame    *fMenuDock;
	TGMenuBar  *fMenuBar;
	TGPopupMenu  *fMenuFile, *fMenuTools, *fMenuView;
	TGLayoutHints      *fMenuBarLayout, *fMenuBarItemLayout;

	fMenuBarLayout = new TGLayoutHints(kLHintsTop | kLHintsExpandX);
	fMenuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);

	fMenuFile = new TGPopupMenu(gClient->GetRoot());
	fMenuFile->AddEntry("&Open List...", M_FILE_OPEN);
	fMenuFile->AddEntry("&Save List...", M_FILE_SAVE);
	fMenuFile->AddSeparator();
	fMenuFile->AddEntry("New Configuration...", M_FILE_NEW_CONFIG);
	fMenuFile->AddEntry("Open Configuration...", M_FILE_OPEN_CONFIG);
	fMenuFile->AddEntry("Save Configuration", M_FILE_SAVE_CONFIG);
	fMenuFile->AddEntry("Save Configuration As ...", M_FILE_SAVE_AS_CONFIG);
	fMenuFile->AddEntry("E&xit", M_FILE_EXIT);

	fMenuTools = new TGPopupMenu(gClient->GetRoot());
	fMenuTools->AddEntry("Config Macros...", M_TOOLS_MACROS);
	fMenuTools->AddEntry("Start TBrowser", M_TOOLS_TBROWSER);
	fMenuTools->AddEntry("View Tree Info", M_TOOLS_TREEINFO);
	fMenuTools->AddEntry("Save Hists...",  M_TOOLS_SAVEHISTS);
	fMenuTools->AddSeparator();
	fMenuTools->AddEntry("Reset Histograms/Macros...",  M_TOOLS_RESET);

	fMenuView = new TGPopupMenu(gClient->GetRoot());
	fMenuView->AddEntry("New Tab...", M_VIEW_NEW_TAB);
	fMenuView->AddEntry("Remove Tab...", M_VIEW_REMOVE_TAB);
	fMenuView->AddSeparator();
	fMenuView->AddEntry("Log X axis", M_VIEW_LOGX);
	fMenuView->AddEntry("Log Y axis", M_VIEW_LOGY);
	fMenuView->AddEntry("Scale Options...", M_VIEW_SCALE_OPTS);

	fMenuBar = new TGMenuBar(this, 1, 1, kHorizontalFrame | kRaisedFrame );
	this->AddFrame(fMenuBar, fMenuBarLayout);
	fMenuBar->AddPopup("&File",  fMenuFile, fMenuBarItemLayout);
	fMenuBar->AddPopup("&Tools", fMenuTools, fMenuBarItemLayout);
	fMenuBar->AddPopup("&View",  fMenuView, fMenuBarItemLayout);

	// connect the menus to methods
	// Menu button messages are handled by the main frame (i.e. "this")
	// HandleMenu() method.
	fMenuFile->Connect("Activated(Int_t)", "jv_mainframe", this, "HandleMenu(Int_t)");
	fMenuTools->Connect("Activated(Int_t)", "jv_mainframe", this, "HandleMenu(Int_t)");
	fMenuView->Connect("Activated(Int_t)", "jv_mainframe", this, "HandleMenu(Int_t)");
   
   
	//==============================================================================================
	// Fill in main content of window. Note that most of the area is taken up by the
	// contents of the TGTab object which is filled in by the RSTab class constructor.

	// Main vertical frame
	TGVerticalFrame *fMain = new TGVerticalFrame(this);
	this->AddFrame(fMain, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));

	// Top, middle(tabs), and bottom frames
	TGHorizontalFrame *fMainTop = new TGHorizontalFrame(fMain);
	TGHorizontalFrame *fMainMid = new TGHorizontalFrame(fMain);
	TGHorizontalFrame *fMainBot = new TGHorizontalFrame(fMain);

	fMain->AddFrame(fMainTop, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
	fMain->AddFrame(fMainMid, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));
	fMain->AddFrame(fMainBot, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));

	//....... Top Frame .......
	
	TGGroupFrame *fInfo = new TGGroupFrame(fMainTop, "Current Event Info", kVerticalFrame);
	fMainTop->AddFrame(fInfo, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));

	lSource = AddNamedLabel(fInfo, "Source:");
	lRun    = AddNamedLabel(fInfo, "   Run:");
	lEvent  = AddNamedLabel(fInfo, " Event:");

	TGButton *bNext = AddButton(fInfo, "Next");

	//....... Middle Frame .......
	
	TGVerticalFrame *fObjectTypes = new TGVerticalFrame(fMainMid);
	fMainMid->AddFrame(fObjectTypes, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandY ,2,2,2,2));
	lbObjectTypes = AddListBox(fObjectTypes, "Object Types", 300, kLHintsExpandY);

	fTab = new TGTab(fMainMid);
	fMainMid->AddFrame(fTab, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));
	fTab->SetWidth(600);

	// ---- Object Details Tab ----
	TGCompositeFrame *tObjectDetails = fTab->AddTab("Object Details");
	TGHorizontalFrame *fObjectDetails = new TGHorizontalFrame(tObjectDetails);
	tObjectDetails->AddFrame(fObjectDetails, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY ,2,2,2,2));

	TGVerticalFrame *fObjects = new TGVerticalFrame(fObjectDetails);
	TGVerticalFrame *fAssociated = new TGVerticalFrame(fObjectDetails);
	TGVerticalFrame *fObjectValues = new TGVerticalFrame(fObjectDetails);
	fObjectDetails->AddFrame(fObjects, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY ,2,2,2,2));
	fObjectDetails->AddFrame(fAssociated, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY ,2,2,2,2));
	fObjectDetails->AddFrame(fObjectValues, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY ,2,2,2,2));

	lObjectType = AddLabel(fObjects, "---------------");
	lbObjects = AddListBox(fObjects, "Objects this event");

	lbAssociatedObjects = AddListBox(fAssociated, "Associated Objects");
	lbAssociatedToObjects = AddListBox(fAssociated, "Objects to which this is associated\n(may be incomplete)");

	lObjectValue = AddLabel(fObjectValues, "---------------");
	lbObjectValues = AddListBox(fObjectValues, "", 250, kLHintsExpandX | kLHintsExpandY);

	// ---- Call Graph Tab ----
	TGCompositeFrame *tCallGraph = fTab->AddTab("Call Graph");
	
	// Use TGCanvas to get scrollbars for the canvas
	TGCanvas *gcanvas = new TGCanvas(tCallGraph, 100, 100, kFixedSize);
	tCallGraph->AddFrame(gcanvas, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX));
	fCanvas = new TGVerticalFrame(gcanvas->GetViewPort(), 10, 10);
	gcanvas->SetContainer(fCanvas);

	canvas = new TRootEmbeddedCanvas("rec1", fCanvas, 400, 400);
	canvas->GetCanvas()->SetFillColor(TColor::GetColor( (Float_t)0.96, 0.96, 0.99));
	fCanvas->AddFrame(canvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));
	
	//....... Bottom Frame .......
	//AddSpacer(fMainBot, 50, 1, kLHintsRight);
	TGTextButton *bQuit = AddButton(fMainBot, "Quit", kLHintsRight | kLHintsBottom);


	//==================== Connect GUI elements to methods ====================
	bQuit->Connect("Clicked()","jv_mainframe", this, "DoQuit()");
	bNext->Connect("Clicked()","jv_mainframe", this, "DoNext()");
	lbObjectTypes->Connect("Selected(Int_t)","jv_mainframe", this, "DoSelectObjectType(Int_t)");
	lbObjects->Connect("Selected(Int_t)","jv_mainframe", this, "DoSelectObject(Int_t)");
	lbAssociatedObjects->Connect("Selected(Int_t)","jv_mainframe", this, "DoSelectAssociatedObject(Int_t)");
	lbAssociatedObjects->Connect("DoubleClicked(Int_t)","jv_mainframe", this, "DoDoubleClickAssociatedObject(Int_t)");
	lbAssociatedToObjects->Connect("Selected(Int_t)","jv_mainframe", this, "DoSelectAssociatedToObject(Int_t)");
	lbAssociatedToObjects->Connect("DoubleClicked(Int_t)","jv_mainframe", this, "DoDoubleClickAssociatedToObject(Int_t)");
}

