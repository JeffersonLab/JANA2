// Mainframe macro generated from application: /usr/local/root/PRO/bin/root.exe
// By ROOT version 5.22/00 on 2009-08-29 09:11:34

#ifndef ROOT_TGDockableFrame
#include "TGDockableFrame.h"
#endif
#ifndef ROOT_TGMenu
#include "TGMenu.h"
#endif
#ifndef ROOT_TGMdiDecorFrame
#include "TGMdiDecorFrame.h"
#endif
#ifndef ROOT_TG3DLine
#include "TG3DLine.h"
#endif
#ifndef ROOT_TGMdiFrame
#include "TGMdiFrame.h"
#endif
#ifndef ROOT_TGMdiMainFrame
#include "TGMdiMainFrame.h"
#endif
#ifndef ROOT_TGuiBldHintsButton
#include "TGuiBldHintsButton.h"
#endif
#ifndef ROOT_TGMdiMenu
#include "TGMdiMenu.h"
#endif
#ifndef ROOT_TGListBox
#include "TGListBox.h"
#endif
#ifndef ROOT_TGNumberEntry
#include "TGNumberEntry.h"
#endif
#ifndef ROOT_TGScrollBar
#include "TGScrollBar.h"
#endif
#ifndef ROOT_TGuiBldHintsEditor
#include "TGuiBldHintsEditor.h"
#endif
#ifndef ROOT_TGFrame
#include "TGFrame.h"
#endif
#ifndef ROOT_TGFileDialog
#include "TGFileDialog.h"
#endif
#ifndef ROOT_TGShutter
#include "TGShutter.h"
#endif
#ifndef ROOT_TGButtonGroup
#include "TGButtonGroup.h"
#endif
#ifndef ROOT_TGCanvas
#include "TGCanvas.h"
#endif
#ifndef ROOT_TGFSContainer
#include "TGFSContainer.h"
#endif
#ifndef ROOT_TGButton
#include "TGButton.h"
#endif
#ifndef ROOT_TGuiBldEditor
#include "TGuiBldEditor.h"
#endif
#ifndef ROOT_TGFSComboBox
#include "TGFSComboBox.h"
#endif
#ifndef ROOT_TGLabel
#include "TGLabel.h"
#endif
#ifndef ROOT_TRootGuiBuilder
#include "TRootGuiBuilder.h"
#endif
#ifndef ROOT_TGTab
#include "TGTab.h"
#endif
#ifndef ROOT_TGListView
#include "TGListView.h"
#endif
#ifndef ROOT_TGSplitter
#include "TGSplitter.h"
#endif
#ifndef ROOT_TGStatusBar
#include "TGStatusBar.h"
#endif
#ifndef ROOT_TGToolTip
#include "TGToolTip.h"
#endif
#ifndef ROOT_TGToolBar
#include "TGToolBar.h"
#endif
#ifndef ROOT_TGuiBldDragManager
#include "TGuiBldDragManager.h"
#endif

#include "Riostream.h"

void Dialog_SelectServerHist()
{

   // main frame
   TGMainFrame *fMainFrame1005 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
   fMainFrame1005->SetLayoutBroken(kTRUE);

   // "fGroupFrame515" group frame
   TGGroupFrame *fGroupFrame515 = new TGGroupFrame(fMainFrame1005,"Select Server/Histo");

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame540 = new TGHorizontalFrame(fGroupFrame515,255,26,kHorizontalFrame);

   ULong_t ucolor;        // will reflect user color changes
   gClient->GetColorByName("#ffffff",ucolor);

   // combo box
   TGComboBox *fComboBox518 = new TGComboBox(fHorizontalFrame540,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   fComboBox518->AddEntry("Entry 1 ",0);
   fComboBox518->AddEntry("Entry 2 ",1);
   fComboBox518->AddEntry("Entry 3 ",2);
   fComboBox518->AddEntry("Entry 4 ",3);
   fComboBox518->AddEntry("Entry 5 ",4);
   fComboBox518->AddEntry("Entry 6 ",5);
   fComboBox518->AddEntry("Entry 7 ",6);
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
   fComboBox560->AddEntry("Entry 1 ",0);
   fComboBox560->AddEntry("Entry 2 ",1);
   fComboBox560->AddEntry("Entry 3 ",2);
   fComboBox560->AddEntry("Entry 4 ",3);
   fComboBox560->AddEntry("Entry 5 ",4);
   fComboBox560->AddEntry("Entry 6 ",5);
   fComboBox560->AddEntry("Entry 7 ",6);
   fComboBox560->Resize(192,22);
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
}  
