// $Id$
//
//    File: jv_mainframe.h
// Created: Fri Oct  3 08:12:01 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _jv_mainframe_
#define _jv_mainframe_

#include <string>
#include <iostream>
using namespace std;

#include <TApplication.h>
#include <TGFrame.h>
#include <TGClient.h>
#include <TGButton.h>
#include <TGLabel.h>
#include <TGMenu.h>
#include <TGListBox.h>
#include <TGDockableFrame.h>
#include <TGTab.h>
#include <TGResourcePool.h>
#include <TCanvas.h>
#include <TText.h>
#include <TRootEmbeddedCanvas.h>
#include <TColor.h>
#include <TTimer.h>

#include "JVFactoryInfo.h"

#if !defined(__CINT__) && !defined(__CLING__)
#include <JANA/JObject.h>
using namespace jana;
#endif // __CINT__  __CLING__



// information for menu bar
enum MenuCommandIdentifiers {
  M_FILE_OPEN,
  M_FILE_SAVE,
  M_FILE_NEW_CONFIG,
  M_FILE_OPEN_CONFIG,
  M_FILE_SAVE_CONFIG,
  M_FILE_SAVE_AS_CONFIG,
  M_FILE_EXIT,

  M_TOOLS_MACROS,
  M_TOOLS_TBROWSER,
  M_TOOLS_TREEINFO,
  M_TOOLS_SAVEHISTS,
  M_TOOLS_RESET,

  M_VIEW_NEW_TAB,
  M_VIEW_REMOVE_TAB,
  M_VIEW_LOGX,
  M_VIEW_LOGY,
  M_VIEW_SCALE_OPTS

};


class jv_mainframe:public TGMainFrame{
	public:
		jv_mainframe(const TGWindow *p, UInt_t w, UInt_t h, bool build_gui);
		virtual ~jv_mainframe();

		// Virtual methods and menu handler
		void CloseWindow(void);
		void HandleMenu(Int_t id);
		void DoQuit(void);
		void DoNext(void);
		void DoDelayedSelectObjectType(void);
		void DoSelectObjectType(Int_t id);
		void DoSelectObject(Int_t id);
		void DoSelectAssociatedObject(Int_t id);
		void DoSelectAssociatedToObject(Int_t id);
		void DoTimer(void);
		void DoDoubleClickAssociatedObject(Int_t id);
		void DoDoubleClickAssociatedToObject(Int_t id);
		Bool_t HandleConfigureNotify(Event_t *event);
		
		void UpdateInfo(string source, int run, int event);
		void UpdateObjectTypeList(vector<JVFactoryInfo> &facinfo);
		void SelectNewObject(void *vobj);
		void Redraw(TGFrame *lb);

#if !defined(__CINT__) && !defined(__CLING__)
		void UpdateObjectValues(JObject *obj);
#endif // __CINT__  __CLING__

		// Helper methods for building GUI
		TGLabel*          AddLabel(TGCompositeFrame* frame, string text, Int_t mode=kTextLeft, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGLabel*          AddNamedLabel(TGCompositeFrame* frame, string text, Int_t mode=kTextLeft, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGTextButton*     AddButton(TGCompositeFrame* frame, string text, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGCheckButton*    AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGPictureButton*  AddPictureButton(TGCompositeFrame* frame, string picture, string tooltip="", ULong_t hints=kLHintsLeft | kLHintsTop);
		TGFrame*          AddSpacer(TGCompositeFrame* frame, UInt_t w=10, UInt_t h=10, ULong_t hints=kLHintsCenterX | kLHintsCenterY);
		TGListBox*        AddListBox(TGCompositeFrame* frame, string lab="", UInt_t w=350, ULong_t hints=kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY);
		
		TGLabel   *lSource;
		TGLabel   *lRun;
		TGLabel   *lEvent;
		TGLabel   *lObjectType;
		TGLabel   *lObjectValue;
		TGListBox *lbObjectTypes;
		TGListBox *lbObjects;
		TGListBox *lbAssociatedObjects;
		TGListBox *lbAssociatedToObjects;
		TGListBox *lbObjectValues;
		
		TGTab *fTab;
		TGVerticalFrame *fCanvas;
		TRootEmbeddedCanvas *canvas;
		
		vector<string> objtypes; // nametags of values in object type listbox
		vector<void*> vobjs;
#if !defined(__CINT__) && !defined(__CLING__)
		vector<const JObject*> aobjs;
		vector<const JObject*> a2objs;
#endif // __CINT__  __CLING__

	protected:
		TTimer *timer;
		long sleep_time; // in milliseconds
		Int_t delayed_object_type_id;

		void CreateGUI(void);
	
	
	private:

	ClassDef(jv_mainframe,1)
};

#endif // _jv_mainframe_

