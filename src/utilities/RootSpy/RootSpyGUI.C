// Mainframe macro generated from application: /usr/local/root/PRO/bin/root.exe
// By ROOT version 5.22/00 on 2009-08-28 20:28:30

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
#ifndef ROOT_TGComboBox
#include "TGComboBox.h"
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
#ifndef ROOT_TGMsgBox
#include "TGMsgBox.h"
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
#ifndef ROOT_TRootEmbeddedCanvas
#include "TRootEmbeddedCanvas.h"
#endif
#ifndef ROOT_TCanvas
#include "TCanvas.h"
#endif
#ifndef ROOT_TGuiBldDragManager
#include "TGuiBldDragManager.h"
#endif

#include "Riostream.h"

void RootSpyGUI()
{

   // main frame
   TGMainFrame *fMainFrame1435 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);

   // composite frame
   TGCompositeFrame *fMainFrame656 = new TGCompositeFrame(fMainFrame1435,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame657 = new TGCompositeFrame(fMainFrame656,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame658 = new TGCompositeFrame(fCompositeFrame657,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame659 = new TGCompositeFrame(fCompositeFrame658,833,700,kVerticalFrame);
   fCompositeFrame659->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fCompositeFrame660 = new TGCompositeFrame(fCompositeFrame659,833,700,kVerticalFrame);
   fCompositeFrame660->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fCompositeFrame661 = new TGCompositeFrame(fCompositeFrame660,833,700,kVerticalFrame);
   fCompositeFrame661->SetLayoutBroken(kTRUE);

   // vertical frame
   TGVerticalFrame *fVerticalFrame662 = new TGVerticalFrame(fCompositeFrame661,708,640,kVerticalFrame);
   fVerticalFrame662->SetLayoutBroken(kTRUE);

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame663 = new TGHorizontalFrame(fVerticalFrame662,564,118,kHorizontalFrame);

   // "Current Histogram Info." group frame
   TGGroupFrame *fGroupFrame664 = new TGGroupFrame(fHorizontalFrame663,"Current Histogram Info.",kHorizontalFrame | kRaisedFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame665 = new TGVerticalFrame(fGroupFrame664,62,60,kVerticalFrame);
   TGLabel *fLabel666 = new TGLabel(fVerticalFrame665,"Server:");
   fLabel666->SetTextJustify(36);
   fLabel666->SetMargins(0,0,0,0);
   fLabel666->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel666, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel667 = new TGLabel(fVerticalFrame665,"Histogram:");
   fLabel667->SetTextJustify(36);
   fLabel667->SetMargins(0,0,0,0);
   fLabel667->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel667, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel668 = new TGLabel(fVerticalFrame665,"Retrieved:");
   fLabel668->SetTextJustify(36);
   fLabel668->SetMargins(0,0,0,0);
   fLabel668->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel668, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame665, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame669 = new TGVerticalFrame(fGroupFrame664,29,60,kVerticalFrame);
   TGLabel *fLabel670 = new TGLabel(fVerticalFrame669,"-----");
   fLabel670->SetTextJustify(36);
   fLabel670->SetMargins(0,0,0,0);
   fLabel670->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel670, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel671 = new TGLabel(fVerticalFrame669,"-----");
   fLabel671->SetTextJustify(36);
   fLabel671->SetMargins(0,0,0,0);
   fLabel671->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel671, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel672 = new TGLabel(fVerticalFrame669,"-----");
   fLabel672->SetTextJustify(36);
   fLabel672->SetMargins(0,0,0,0);
   fLabel672->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel672, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame669, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame673 = new TGVerticalFrame(fGroupFrame664,97,78,kVerticalFrame);
   TGTextButton *fTextButton674 = new TGTextButton(fVerticalFrame673,"Change Server");
   fTextButton674->SetTextJustify(36);
   fTextButton674->SetMargins(0,0,0,0);
   fTextButton674->SetWrapLength(-1);
   fTextButton674->Resize(93,22);
   fVerticalFrame673->AddFrame(fTextButton674, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGTextButton *fTextButton675 = new TGTextButton(fVerticalFrame673,"Change Histo.");
   fTextButton675->SetTextJustify(36);
   fTextButton675->SetMargins(0,0,0,0);
   fTextButton675->SetWrapLength(-1);
   fTextButton675->Resize(87,22);
   fVerticalFrame673->AddFrame(fTextButton675, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
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
   TGComboBox *fComboBox684 = new TGComboBox(fHorizontalFrame682,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   fComboBox684->AddEntry("2s",2);
   fComboBox684->AddEntry("4s",4);
   fComboBox684->AddEntry("10s ",10);
   fComboBox684->Resize(102,22);
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
   fVerticalFrame662->AddFrame(fRootEmbeddedCanvas698, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
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
   fHorizontalFrame1060->AddFrame(fTextButton1075, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fVerticalFrame662->AddFrame(fHorizontalFrame1060, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
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
}  
