/*
 Rocrail - Model Railroad Software

 Copyright (C) 2002-2007 - Rob Versluis <r.j.versluis@rocrail.net>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
    #pragma implementation "rocgui.cpp"
    #pragma interface "rocgui.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif


#include "rocs/public/node.h"
#include "rocs/public/str.h"
#include "rocs/public/strtok.h"
#include "rocs/public/trace.h"
#include "rocs/public/mem.h"
#include "rocgui/public/guiapp.h"
#include "rocgui/public/base.h"
#include "rocgui/public/item.h"

#include "rocgui/dialogs/switchdialog.h"
#include "rocgui/dialogs/signaldialog.h"
#include "rocgui/dialogs/outputdialog.h"
#include "rocgui/dialogs/feedbackdialog.h"
#include "rocgui/dialogs/locdialog.h"
#include "rocgui/dialogs/blockdialog.h"
#include "rocgui/dialogs/scheduledialog.h"
#include "rocgui/dialogs/trackdialog.h"
#include "rocgui/dialogs/turntabledialog.h"
#include "rocgui/dialogs/textdialog.h"
#include "rocgui/dialogs/locseldlg.h"
#include "rocgui/dialogs/seltabdlg.h"

#include "rocrail/wrapper/public/AutoCmd.h"
#include "rocrail/wrapper/public/SysCmd.h"
#include "rocrail/wrapper/public/ModelCmd.h"
#include "rocrail/wrapper/public/Plan.h"
#include "rocrail/wrapper/public/Item.h"
#include "rocrail/wrapper/public/Block.h"
#include "rocrail/wrapper/public/Schedule.h"
#include "rocrail/wrapper/public/Switch.h"
#include "rocrail/wrapper/public/Signal.h"
#include "rocrail/wrapper/public/Output.h"
#include "rocrail/wrapper/public/Track.h"
#include "rocrail/wrapper/public/Turntable.h"
#include "rocrail/wrapper/public/TTTrack.h"
#include "rocrail/wrapper/public/SelTab.h"
#include "rocrail/wrapper/public/SelTabPos.h"
#include "rocrail/wrapper/public/Loc.h"
#include "rocrail/wrapper/public/Feedback.h"
#include "rocrail/wrapper/public/Text.h"
#include "rocrail/wrapper/public/ZLevel.h"

#include "rocgui/wrapper/public/Gui.h"
#include "rocgui/wrapper/public/PlanPanel.h"

#include <math.h>


static double PI25DT = 3.141592653589793238462643;

enum {
    // menu items
    ME_Props = 1,
    ME_Rotate,
    ME_North,
    ME_East,
    ME_South,
    ME_West,
    ME_Type,
    ME_Delete,
    ME_CmdStraight,
    ME_CmdTurnout,
    ME_CmdLeft,
    ME_CmdRight,
    ME_CmdSignalManual,
    ME_CmdSignalAuto,
    ME_CmdSignalRed,
    ME_CmdSignalGreen,
    ME_CmdSignalYellow,
    ME_CmdSignalWhite,
    ME_Loc,
    ME_UnLoc,
    ME_LocGoTo,
    ME_LocSchedule,
    ME_LocGo,
    ME_LocGoManual,
    ME_LocStop,
    ME_LocReset,
    ME_LocMIC,
    ME_CloseBlock,
    ME_OpenBlock,
    ME_Info,
    ME_Timer,
    ME_TTLightOn,
    ME_TTLightOff,
    ME_TTNext,
    ME_TTPrev,
    ME_TT180,
    ME_TTTrack // Should be the last one in the enum list
               // because ME_TTTrack+0...ME_TTTrack+47 are also used!!!.
};

BEGIN_EVENT_TABLE(Symbol, wxWindow)
  EVT_PAINT        (Symbol::OnPaint     )
  EVT_LEFT_UP      (Symbol::OnLeftUp    )
  EVT_LEFT_DOWN    (Symbol::OnLeftDown  )
  EVT_MOTION       (Symbol::OnMotion    )
  EVT_RIGHT_UP     (Symbol::OnPopup     )
  EVT_ENTER_WINDOW (Symbol::OnMouseEnter)
  EVT_LEAVE_WINDOW (Symbol::OnMouseLeave)

  EVT_TIMER (ME_Timer, Symbol::OnTimer)

  EVT_LEFT_DCLICK(Symbol::OnLeftDClick  )

  EVT_MENU     (ME_Props  , Symbol::OnProps  )
  EVT_MENU     (ME_Loc    , Symbol::OnLoc    )
  EVT_MENU     (ME_UnLoc  , Symbol::OnUnLoc  )
  EVT_MENU     (ME_PanelSelect , Symbol::OnSelect )
  EVT_MENU     (ME_Rotate , Symbol::OnRotate )
  EVT_MENU     (ME_North , Symbol::OnRotate )
  EVT_MENU     (ME_East , Symbol::OnRotate )
  EVT_MENU     (ME_South , Symbol::OnRotate )
  EVT_MENU     (ME_West , Symbol::OnRotate )
  EVT_MENU     (ME_Type   , Symbol::OnType   )
  EVT_MENU     (ME_Delete , Symbol::OnDelete )
  EVT_MENU     (ME_LocGoTo, Symbol::OnLocGoTo)
  EVT_MENU     (ME_LocSchedule, Symbol::OnLocSchedule)
  EVT_MENU     (ME_LocGo  , Symbol::OnLocGo  )
  EVT_MENU     (ME_LocGoManual  , Symbol::OnLocGoManual  )
  EVT_MENU     (ME_LocStop, Symbol::OnLocStop)
  EVT_MENU     (ME_LocReset, Symbol::OnLocReset)
  EVT_MENU     (ME_LocMIC  , Symbol::OnLocMIC  )
  EVT_MENU     (ME_CloseBlock, Symbol::OnCloseBlock)
  EVT_MENU     (ME_OpenBlock, Symbol::OnOpenBlock)

  EVT_MENU     (ME_CmdStraight, Symbol::OnCmdStraight )
  EVT_MENU     (ME_CmdTurnout, Symbol::OnCmdTurnout )
  EVT_MENU     (ME_CmdLeft, Symbol::OnCmdLeft )
  EVT_MENU     (ME_CmdRight, Symbol::OnCmdRight )

  EVT_MENU     (ME_CmdSignalAuto, Symbol::OnCmdSignalAuto )
  EVT_MENU     (ME_CmdSignalManual, Symbol::OnCmdSignalManual )
  EVT_MENU     (ME_CmdSignalRed, Symbol::OnCmdSignalRed )
  EVT_MENU     (ME_CmdSignalYellow, Symbol::OnCmdSignalYellow )
  EVT_MENU     (ME_CmdSignalGreen, Symbol::OnCmdSignalGreen )
  EVT_MENU     (ME_CmdSignalWhite, Symbol::OnCmdSignalWhite )

  EVT_MENU     (ME_Info, Symbol::OnInfo)

  EVT_MENU     (ME_TTLightOn, Symbol::OnTTLight)
  EVT_MENU     (ME_TTLightOff, Symbol::OnTTLight)
  EVT_MENU     (ME_TTNext, Symbol::OnTTNext)
  EVT_MENU     (ME_TTPrev, Symbol::OnTTPrev)
  EVT_MENU     (ME_TT180, Symbol::OnTT180)

  EVT_MENU     (ME_TTTrack+0, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+1, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+2, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+3, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+4, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+5, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+6, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+7, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+8, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+9, Symbol::OnTTTrack)

  EVT_MENU     (ME_TTTrack+10, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+11, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+12, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+13, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+14, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+15, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+16, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+17, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+18, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+19, Symbol::OnTTTrack)

  EVT_MENU     (ME_TTTrack+20, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+21, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+22, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+23, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+24, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+25, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+26, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+27, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+28, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+29, Symbol::OnTTTrack)

  EVT_MENU     (ME_TTTrack+30, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+31, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+32, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+33, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+34, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+35, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+36, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+37, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+38, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+39, Symbol::OnTTTrack)

  EVT_MENU     (ME_TTTrack+40, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+41, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+42, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+43, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+44, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+45, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+46, Symbol::OnTTTrack)
  EVT_MENU     (ME_TTTrack+47, Symbol::OnTTTrack)
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// Item
// ----------------------------------------------------------------------------

Symbol::Symbol( PlanPanel *parent, iONode props, int itemsize, int z, double scale, double bktext )
             : wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, 0 )
{
  m_Z = z;
  m_Scale = scale;
  m_Bktext = bktext;
  m_ItemSize = itemsize;
  m_PlanPanel = parent;
  m_Props = props;
  m_isDragged = false;
  m_locidStr = NULL;
  m_RouteID = NULL;
  m_locidStr = NULL;
  m_Timer = new wxTimer( this, ME_Timer );

  int itemidps = 7;
  iONode planpanelIni = wGui.getplanpanel(wxGetApp().getIni());
  if( planpanelIni != NULL ) {
    itemidps = wPlanPanel.getitemidps(planpanelIni);
  }

  m_Renderer = new SymbolRenderer( props, this, wxGetApp().getFrame()->getSymMap(), itemidps );

  const char* tip = wItem.getid( m_Props );
  if( StrOp.len( wItem.getdesc( m_Props ) ) > 0 )
    tip = wItem.getdesc( m_Props );

  SetToolTip( wxString(tip,wxConvUTF8) );
  modelEvent(m_Props);

  if( StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) ) ) {
    bridgepos = 0.0;
    iONode track = wTurntable.gettrack( m_Props );
    while( track != NULL ) {
      m_PlanPanel->addItem( NodeOp.getName(track), wTTTrack.getposfb( track ), this );
      track = wTurntable.nexttrack( m_Props, track );
    }
  }

  //if( StrOp.equals( wText.name(), NodeOp.getName( m_Props ) ) && StrOp.endsWithi(wText.gettext(m_Props), ".png") )
    //Lower();



  sizeToScale();
}

void Symbol::sizeToScale() {
  int x_off, y_off;
  m_PlanPanel->GetViewStart( &x_off, &y_off );

  int x = wItem.getx( m_Props ) - x_off;
  int y = wItem.gety( m_Props ) - y_off;

  int z = wItem.getz( m_Props );

  const char* name = NodeOp.getName( m_Props );

  if( m_Z != z )
    Show( false );
  else if( StrOp.equals( wOutput.name(), name ) || StrOp.equals( wFeedback.name(), name ) ) {
    Show(wOutput.isshow(m_Props));
  }

  int cx = 1;
  int cy = 1;
  double c = getSize();
  m_Renderer->sizeToScale( c, m_Scale, m_Bktext, &cx, &cy );
  //updateLabel();

  SetSize( (int)(x*c), (int)(y*c), (int)(c*cx), (int)(c*cy) );

  //SetBackgroundColour( *wxWHITE );
}

void Symbol::blockEvent( const char* id ) {
  if( StrOp.equals( wTrack.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSwitch.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wOutput.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wFeedback.name(), NodeOp.getName( m_Props ) )) {
    if( StrOp.equals( wTrack.getblockid( m_Props ), id ) ) {
      TraceOp.trc( "item", TRCLEVEL_DEBUG, __LINE__, 9999, "block[%s] occ id=[%s]", id, wItem.getid(m_Props) );
      Refresh();
    }
  }
}

void Symbol::routeEvent( const char* id ) {
  if( StrOp.equals( wTrack.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wOutput.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wFeedback.name(), NodeOp.getName( m_Props ) )) {
    if( wItem.getrouteids( m_Props ) != NULL && StrOp.len(wItem.getrouteids( m_Props )) > 0 ) {
      StrOp.free(m_RouteID);
      m_RouteID = StrOp.dup(id);
      Refresh();
    }
  }
}

void Symbol::updateLabel() {
  modelEvent(m_Props);
}


const char* Symbol::getId() {
  return wItem.getid( m_Props );
}

iONode Symbol::getProperties() {
  return m_Props;
}

void Symbol::reScale( double scale) {
  m_Scale = scale;
  sizeToScale();
}

void Symbol::disable() {
  m_Props = NULL;
}

void Symbol::OnPaint(wxPaintEvent& event)
{
  const char* name = NodeOp.getName( m_Props );

  wxPaintDC dc(this);
  if( m_Props == NULL )
    return;

  if( !wZLevel.isactive(m_PlanPanel->getZLevel()) && !m_PlanPanel->isModView() )
    return;

  int z = wItem.getz( m_Props );

  if( m_Z == z ) {
    //Show( true );

    if( StrOp.equals( wOutput.name(), name ) || StrOp.equals( wFeedback.name(), name ) ) {
      Show(wOutput.isshow(m_Props));
    }

    dc.SetPen( *wxLIGHT_GREY_PEN );
    double c = getSize();
    int cx = m_Renderer->getcx();
    int cy = m_Renderer->getcy();
    bool occupied = false;
    bool actroute = false;

    if( m_RouteID != NULL ) {
      iOStrTok tok = StrTokOp.inst( wItem.getrouteids( m_Props ), ',' );

      const char* routeid = StrTokOp.nextToken(tok);

      while( routeid != NULL ) {
        if( StrOp.equals( routeid, m_RouteID )) {
          actroute = true;
          break;
        }
        routeid = StrTokOp.nextToken(tok);
      };

      StrTokOp.base.del( tok );
    }


    if( StrOp.equals( wTrack.name()   , NodeOp.getName( m_Props ) ) ||
        StrOp.equals( wSignal.name()  , NodeOp.getName( m_Props ) ) ||
        StrOp.equals( wFeedback.name(), NodeOp.getName( m_Props ) ) ||
        StrOp.equals( wOutput.name(), NodeOp.getName( m_Props ) ) ||
        StrOp.equals( wSwitch.name()  , NodeOp.getName( m_Props ) ) )
      {
      if( wTrack.getblockid( m_Props ) != NULL && StrOp.len( wItem.getblockid( m_Props )) > 0 ) {
        occupied = m_PlanPanel->isBlockOccupied( wItem.getblockid( m_Props ) );
        TraceOp.trc( "item", TRCLEVEL_DEBUG, __LINE__, 9999, "%s is %s occupied by %s",
            wItem.getid( m_Props ), occupied? "":"not", wItem.getblockid( m_Props ));
      }

    }
    else if( StrOp.equals( wSelTab.name(), NodeOp.getName( m_Props ) ) ) {
      occupied = wSelTab.ispending(m_Props);
    }

    if( wxGetApp().getFrame()->isRaster() ) {
      dc.DrawLine( 0, 0, (int)(c*cx), 0 );
      dc.DrawLine( 0, 0, 0, (int)(c*cy) );
    }

    dc.SetUserScale( m_Scale, m_Scale );
/*
*/
    dc.SetPen( *wxBLACK_PEN );
    wxPen pen = dc.GetPen();
    pen.SetWidth(1);
    dc.SetPen(pen);
    m_Renderer->drawShape( dc, wxGetApp().getFrame()->isFill(), occupied, actroute, &bridgepos, wxGetApp().getFrame()->isShowID() );

    /*
    if( StrOp.equals( wText.name(), NodeOp.getName( m_Props ) ) && StrOp.endsWithi(wText.gettext(m_Props), ".png") )
      Lower();
    else
      Raise();
    */
  }
  else {
    Show( false );
  }
}

void Symbol::setPosition() {
  double c = getSize();
  wxPoint p = GetPosition();
  wxSize s = GetSize();

  int x_off, y_off;
  m_PlanPanel->GetViewStart( &x_off, &y_off );
  int x = wItem.getx( m_Props ) - x_off;
  int y = wItem.gety( m_Props ) - y_off;

  SetSize( (int)(x*c), (int)(y*c), s.GetWidth(), s.GetHeight() );
}


void Symbol::OnCmdStraight(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSwitch.name(), NULL, ELEMENT_NODE );
  wSwitch.setid( cmd, wSwitch.getid( m_Props ) );
  wSwitch.setcmd( cmd, wSwitch.straight );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}
void Symbol::OnCmdTurnout(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSwitch.name(), NULL, ELEMENT_NODE );
  wSwitch.setid( cmd, wSwitch.getid( m_Props ) );
  wSwitch.setcmd( cmd, wSwitch.turnout );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}
void Symbol::OnCmdLeft(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSwitch.name(), NULL, ELEMENT_NODE );
  wSwitch.setid( cmd, wSwitch.getid( m_Props ) );
  wSwitch.setcmd( cmd, wSwitch.left );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}
void Symbol::OnCmdRight(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSwitch.name(), NULL, ELEMENT_NODE );
  wSwitch.setid( cmd, wSwitch.getid( m_Props ) );
  wSwitch.setcmd( cmd, wSwitch.right );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnCmdSignalRed(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
  wSignal.setid( cmd, wSignal.getid( m_Props ) );
  wSignal.setcmd( cmd, wSignal.red );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnCmdSignalGreen(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
  wSignal.setid( cmd, wSignal.getid( m_Props ) );
  wSignal.setcmd( cmd, wSignal.green );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnCmdSignalYellow(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
  wSignal.setid( cmd, wSignal.getid( m_Props ) );
  wSignal.setcmd( cmd, wSignal.yellow );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnCmdSignalWhite(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
  wSignal.setid( cmd, wSignal.getid( m_Props ) );
  wSignal.setcmd( cmd, wSignal.white );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnCmdSignalAuto(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
  wSignal.setid( cmd, wSignal.getid( m_Props ) );
  wSignal.setcmd( cmd, wSignal.autooperated );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnCmdSignalManual(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
  wSignal.setid( cmd, wSignal.getid( m_Props ) );
  wSignal.setcmd( cmd, wSignal.manualoperated );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}




void Symbol::OnLeftDClick(wxMouseEvent& event) {
  if( wxGetApp().getFrame()->isEditMode() ) {
    wxCommandEvent cmd;
    OnProps(cmd);
  }
  else {
    OnLeftUp(event);
  }
}

void Symbol::OnLeftUp(wxMouseEvent& event) {
  if( m_isDragged ) {
    m_isDragged = false;
    ReleaseMouse();
    double c = getSize();
    wxPoint p = GetPosition();
    p.x = (int)((c/2+p.x) / c);
    p.y = (int)((c/2+p.y) / c);

    int x_off, y_off;
    m_PlanPanel->GetViewStart( &x_off, &y_off );
    wItem.setx( m_Props, p.x + x_off );
    wItem.sety( m_Props, p.y + y_off );
    setPosition();

    /* Notify RocRail. */
    TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999,
        "Change position to %d,%d", wItem.getx(m_Props), wItem.gety(m_Props) );
    iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
    wModelCmd.setcmd( cmd, wModelCmd.modify );
    iONode item = (iONode)NodeOp.base.clone( m_Props );
    NodeOp.addChild( cmd, item );
    wxGetApp().sendToRocrail( cmd );
    cmd->base.del(cmd);
  }
  else {
    const char* nodeName = NodeOp.getName( m_Props );
    if( StrOp.equals( wSwitch.name(), nodeName ) ) {
      iONode cmd = NodeOp.inst( wSwitch.name(), NULL, ELEMENT_NODE );
      wSwitch.setid( cmd, wSwitch.getid( m_Props ) );
      wSwitch.setcmd( cmd, wSwitch.flip );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    else if( StrOp.equals( wSignal.name(), nodeName ) ) {
      iONode cmd = NodeOp.inst( wSignal.name(), NULL, ELEMENT_NODE );
      wSignal.setid( cmd, wSignal.getid( m_Props ) );
      wSignal.setcmd( cmd, wSignal.flip );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    else if( StrOp.equals( wOutput.name(), nodeName ) ) {
      iONode cmd = NodeOp.inst( wOutput.name(), NULL, ELEMENT_NODE );
      wOutput.setid( cmd, wOutput.getid( m_Props ) );
      if( StrOp.equals( wOutput.on, wOutput.getstate( m_Props ) ) )
        wOutput.setcmd( cmd, wOutput.off );
      else
        wOutput.setcmd( cmd, wOutput.on );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    else if( StrOp.equals( wTurntable.name(), nodeName ) ) {
      const char* dir = wTurntable.next;
      if( event.ShiftDown() )
        dir = wTurntable.prev;
      iONode cmd = NodeOp.inst( wTurntable.name(), NULL, ELEMENT_NODE );
      wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
      wTurntable.setcmd( cmd, dir );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    else if( StrOp.equals( wSelTab.name(), nodeName ) ) {
      const char* dir = wSelTab.next;
      if( event.ShiftDown() )
        dir = wSelTab.prev;
      iONode cmd = NodeOp.inst( wSelTab.name(), NULL, ELEMENT_NODE );
      wSelTab.setid( cmd, wSelTab.getid( m_Props ) );
      wSelTab.setcmd( cmd, dir );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    else if( StrOp.equals( wBlock.name(), nodeName ) ) {
      const char* locId = wBlock.getlocid( m_Props );
      Boolean hasLoc = StrOp.len( locId ) > 0 ? True:False;
      if( hasLoc  && wxGetApp().getFrame()->isAutoMode() ) {
        iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
        wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
        wLoc.setcmd( cmd, wLoc.stopgo );
        wxGetApp().sendToRocrail( cmd );
        cmd->base.del(cmd);
      }
    }
    else if( StrOp.equals( wFeedback.name(), nodeName ) ) {
      iONode cmd = NodeOp.inst( wFeedback.name(), NULL, ELEMENT_NODE );
      wFeedback.setid( cmd, wFeedback.getid( m_Props ) );
      // simulate the invert state:
      wFeedback.setstate( cmd, wFeedback.isstate(m_Props) ? False:True);
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }

  }
}

void Symbol::OnLeftDown(wxMouseEvent& event) {
  int x;
  int y;

  if( !StrOp.equals( wText.name(), NodeOp.getName( m_Props ) ) )
    Raise();

  wxGetMousePosition( &x, &y );

  m_dragX = x;
  m_dragY = y;

  char* text = StrOp.fmt( "(%d,%d)", x, y );
  wxGetApp().getFrame()->setInfoText( text );
  StrOp.free( text );
  /*
  m_X = (int)(m_mouseX / (itemSize*m_Scale));
  m_Y = (int)(m_mouseY / (itemSize*m_Scale));
  */

  /*
  if( event.ControlDown() ) {
    // group selection...
    SetBackgroundColour( Base::getYellow() );
    Refresh();
  }
  else {
    SetBackgroundColour( Base::getWhite() );
    Refresh();
  }
  */
}

void Symbol::OnTimer(wxTimerEvent& event) {
  const char* locId = wBlock.getlocid( m_Props );
  Boolean hasLoc = StrOp.len( locId ) > 0 ? True:False;
  if( hasLoc && m_hasMouse ) {
    wxGetApp().getFrame()->setLocID( locId );
  }
}


void Symbol::OnMouseEnter(wxMouseEvent& event) {
  m_hasMouse = true;
  if( StrOp.equals( wSwitch.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wOutput.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSelTab.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) )
      ) {
    wxCursor cursor = wxCursor(wxCURSOR_HAND);
    SetCursor( cursor );
    //SetBackgroundColour( Base::getYellow() );
    //Refresh();
  }
  else if( StrOp.equals( wBlock.name(), NodeOp.getName( m_Props ) ) ) {
    const char* locId = wBlock.getlocid( m_Props );
    const char* state = wBlock.getstate( m_Props );
    Boolean hasLoc = StrOp.len( locId ) > 0 ? True:False;
    if( hasLoc ) {
      m_Timer->Start( 1000, true );
      //wxGetApp().getFrame()->setLocID( locId );
      if( wxGetApp().getFrame()->isAutoMode() ) {
        wxCursor cursor = wxCursor(wxCURSOR_HAND);
        SetCursor( cursor );
      }
      //SetBackgroundColour( Base::getYellow() );
      //Refresh();
    }
  }
}

void Symbol::OnMouseLeave(wxMouseEvent& event) {
  m_hasMouse = false;
  if( StrOp.equals( wSwitch.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wOutput.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wSelTab.name(), NodeOp.getName( m_Props ) ) ||
      StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) )
      ) {
    wxCursor cursor = wxCursor(wxCURSOR_ARROW);
    SetCursor( cursor );
    //SetBackgroundColour( Base::getWhite() );
    //Refresh();
  }
  else if( StrOp.equals( wBlock.name(), NodeOp.getName( m_Props ) ) ) {
    wxCursor cursor = wxCursor(wxCURSOR_ARROW);
    SetCursor( cursor );
    //SetBackgroundColour( Base::getWhite() );
    //Refresh();
  }
}

void Symbol::OnMotion(wxMouseEvent& event) {
  if( m_Props == NULL)
    return;

  if( !wxGetApp().getFrame()->isEditMode() ) {
    m_hasMouse = true;
    if( StrOp.equals( wBlock.name(), NodeOp.getName( m_Props ) ) ) {
      const char* locId = wBlock.getlocid( m_Props );
      Boolean hasLoc = StrOp.len( locId ) > 0 ? True:False;
      if( hasLoc ) {
        if( wxGetApp().getFrame()->isAutoMode() ) {
          wxCursor cursor = wxCursor(wxCURSOR_HAND);
          SetCursor( cursor );
        }
      }
    }
    char* text = StrOp.fmt( "(%d,%d)", wItem.getx(m_Props), wItem.gety(m_Props) );
    wxGetApp().getFrame()->setInfoText( text );
    StrOp.free( text );
    return;
  }

  int x;
  int y;

  wxGetMousePosition( &x, &y );

  bool dragging = event.Dragging();


  if( dragging && event.m_leftDown && !m_isDragged ) {
    m_isDragged = true;
    CaptureMouse();
  }

  if( dragging && event.m_leftDown ) {
    int cx = x - m_dragX;
    int cy = y - m_dragY;
    if( cx != 0 || cy != 0 ) {
      wxPoint p = GetPosition();
      wxSize s = GetSize();
      p.x = p.x + cx;
      p.y = p.y + cy;
      SetSize( p.x, p.y, s.GetWidth(), s.GetHeight() );
      m_dragX = x;
      m_dragY = y;
    }
  }

  char* text = StrOp.fmt( "%s(%d,%d)", dragging?"drag":"", wItem.getx(m_Props), wItem.gety(m_Props) );
  wxGetApp().getFrame()->setInfoText( text );
  StrOp.free( text );
}

void Symbol::OnPopup(wxMouseEvent& event)
{
    wxMenu menu( wxString(wItem.getid( m_Props ),wxConvUTF8) );

    if( StrOp.equals( wBlock.name(), NodeOp.getName( m_Props ) ) ) {
      const char* locId = wBlock.getlocid( m_Props );
      const char* state = wBlock.getstate( m_Props );
      Boolean hasLoc = StrOp.len( locId ) > 0 ? True:False;
      if( hasLoc ) {
        menu.Append( ME_UnLoc, wxGetApp().getMenu("resetlocid") );
        menu.AppendSeparator();
        menu.Append( ME_LocGo, wxGetApp().getMenu("startloc") );
        menu.Append( ME_LocGoManual, wxGetApp().getMenu("gomanual") );
        menu.Append( ME_LocStop, wxGetApp().getMenu("stoploc") );
        menu.Append( ME_LocMIC, wxGetApp().getMenu("mic") );
        menu.Append( ME_LocGoTo, wxGetApp().getMenu("gotoblock"), wxGetApp().getTip("gotoblock") );
        menu.Append( ME_LocSchedule, wxGetApp().getMenu("selectschedule"), wxGetApp().getTip("selectschedule") );
        wxMenuItem *mi = menu.FindItem( ME_LocGo );
        if( !wxGetApp().getFrame()->isAutoMode() )
          mi->Enable( false );
        mi = menu.FindItem( ME_LocGoManual );
        if( !wxGetApp().getFrame()->isAutoMode() )
          mi->Enable( false );
        mi = menu.FindItem( ME_UnLoc );
        //if( wxGetApp().getFrame()->isAutoMode() )
        //  mi->Enable( false );
        menu.AppendSeparator();
        menu.Append( ME_LocReset, wxGetApp().getMenu("softresetall") );
        mi = menu.FindItem( ME_LocReset );
        if( wxGetApp().getFrame()->isAutoMode() )
          mi->Enable( false );
        menu.AppendSeparator();
      }
      else if( !hasLoc && StrOp.equals( wBlock.open, state ) ) {
        menu.Append( ME_Loc, wxGetApp().getMenu("setlocid") );
        menu.AppendSeparator();
      }
      if( StrOp.equals( wBlock.open, state ) ) {
        menu.Append( ME_CloseBlock, wxGetApp().getMenu("outoforder") );
        wxMenuItem *mi_Close = menu.FindItem( ME_CloseBlock );
        //if( wxGetApp().getFrame()->isAutoMode() )
        //  mi_Close->Enable( false );
      }
      else {
        menu.Append( ME_OpenBlock, wxGetApp().getMenu("operational") );
      }
    }
    else if( StrOp.equals( wSwitch.name(), NodeOp.getName( m_Props ) ) ) {
      //if( !wxGetApp().getFrame()->isAutoMode() )
      menu.Append( ME_UnLoc, wxGetApp().getMenu("resetlocid") );
      wxMenu* menuSwCmd = new wxMenu();
      menuSwCmd->Append( ME_CmdStraight, wxGetApp().getMenu("straight") );

      const char* type = wSwitch.gettype( m_Props );
      if( !StrOp.equals( wSwitch.decoupler, type ) ) {

        if( !StrOp.equals( wSwitch.threeway, type ) )
          menuSwCmd->Append( ME_CmdTurnout, wxGetApp().getMenu("turnout") );

        if( StrOp.equals( wSwitch.dcrossing, type ) || StrOp.equals( wSwitch.threeway, type ) ) {
          menuSwCmd->Append( ME_CmdLeft, wxGetApp().getMenu("left") );
          menuSwCmd->Append( ME_CmdRight, wxGetApp().getMenu("right") );
        }
      }
      menu.Append( -1, wxGetApp().getMenu("command"), menuSwCmd );
    }
    else if( StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ) {
      wxMenu* menuSgCmd = new wxMenu();
      if( wSignal.ismanual( m_Props ) ) {
        menuSgCmd->Append( ME_CmdSignalAuto, wxGetApp().getMenu("autooperated") );
      }
      else {
        menuSgCmd->Append( ME_CmdSignalManual, wxGetApp().getMenu("manualoperated") );
      }
      menuSgCmd->Append( ME_CmdSignalRed, wxGetApp().getMenu("red") );
      menuSgCmd->Append( ME_CmdSignalGreen, wxGetApp().getMenu("green") );
      if( wSignal.getaspects( m_Props ) > 2 )
        menuSgCmd->Append( ME_CmdSignalYellow, wxGetApp().getMenu("yellow") );
      if( wSignal.getaspects( m_Props ) > 3 )
        menuSgCmd->Append( ME_CmdSignalWhite, wxGetApp().getMenu("white") );
      menu.Append( -1, wxGetApp().getMenu("command"), menuSgCmd );
    }
    else if( StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) ) ) {
      menu.Append( ME_TTLightOn, wxGetApp().getMenu("lights") + _T(" ") + wxGetApp().getMenu("on") );
      menu.Append( ME_TTLightOff, wxGetApp().getMenu("lights") + _T(" ") + wxGetApp().getMenu("off") );
      menu.Append( ME_TTNext, wxGetApp().getMenu("nexttrack") );
      menu.Append( ME_TTPrev, wxGetApp().getMenu("prevtrack") );
      menu.Append( ME_TT180, wxGetApp().getMenu("turn180") );
      wxMenu* trackmenu = new wxMenu();

      iONode track = wTurntable.gettrack( m_Props );
      while( track != NULL ) {
        m_PlanPanel->addItem( NodeOp.getName(track), wTTTrack.getposfb( track ), this );
        int tracknr = wTTTrack.getnr( track );
        if( tracknr >=0 && tracknr < 48 ) {
          char* nrstr = StrOp.fmt( "%d %s", tracknr, wTTTrack.getdesc(track) );
          trackmenu->Append( ME_TTTrack+tracknr, wxString(nrstr,wxConvUTF8) );
          StrOp.free( nrstr );
        }
        track = wTurntable.nexttrack( m_Props, track );
      }
      menu.Append( -1, wxGetApp().getMenu("gototrack"), trackmenu);
      menu.AppendSeparator();
      menu.Append( ME_Info, wxGetApp().getMenu("info") );
      menu.Append( ME_UnLoc, wxGetApp().getMenu("resetlocid") );
      menu.AppendSeparator();
    }
    else if( StrOp.equals( wSelTab.name(), NodeOp.getName( m_Props ) ) ) {
      menu.Append( ME_TTNext, wxGetApp().getMenu("nexttrack") );
      menu.Append( ME_TTPrev, wxGetApp().getMenu("prevtrack") );
      wxMenu* trackmenu = new wxMenu();

      iONode track = wSelTab.getseltabpos( m_Props );
      while( track != NULL ) {
        int tracknr = wSelTabPos.getnr( track );
        if( tracknr >=0 && tracknr < 64 ) {
          char* nrstr = StrOp.fmt( "%d %s", tracknr, wSelTabPos.getdesc(track) );
          trackmenu->Append( ME_TTTrack+tracknr, wxString(nrstr,wxConvUTF8) );
          StrOp.free( nrstr );
        }
        track = wSelTab.nextseltabpos( m_Props, track );
      }
      menu.Append( -1, wxGetApp().getMenu("gototrack"), trackmenu);
      menu.AppendSeparator();
      menu.Append( ME_UnLoc, wxGetApp().getMenu("resetlocid") );

      const char* state = wBlock.getstate( m_Props );
      if( StrOp.equals( wBlock.open, state ) ) {
        menu.Append( ME_CloseBlock, wxGetApp().getMenu("outoforder") );
        wxMenuItem *mi_Close = menu.FindItem( ME_CloseBlock );
        //if( wxGetApp().getFrame()->isAutoMode() )
        //  mi_Close->Enable( false );
      }
      else {
        menu.Append( ME_OpenBlock, wxGetApp().getMenu("operational") );
      }
      menu.AppendSeparator();
    }

    //menu.AppendSeparator();
    wxMenuItem* mi = menu.Append( ME_Props , wxGetApp().getMenu("properties") );
    mi->Enable( !wxGetApp().getFrame()->isAutoMode() || !wxGetApp().isRestrictedEdit() );

    if( wxGetApp().getFrame()->isEditMode() ) {
      menu.Append( ME_Delete, wxGetApp().getMenu("delete") );

      if( m_Renderer->isRotateable() ) {
        wxMenu* menuOrientation = new wxMenu();
        menuOrientation->Append( ME_Rotate, wxGetApp().getMenu("rotate") );
        menuOrientation->AppendSeparator();
        menuOrientation->Append( ME_North, wxGetApp().getMenu("north") );
        menuOrientation->Append( ME_East, wxGetApp().getMenu("east") );
        menuOrientation->Append( ME_South, wxGetApp().getMenu("south") );
        menuOrientation->Append( ME_West, wxGetApp().getMenu("west") );
        menu.Append( -1, wxGetApp().getMenu("orientation"), menuOrientation );
        menu.AppendSeparator();
        menu.Append( ME_PanelSelect, wxGetApp().getMenu("select") );
      }

      if( StrOp.equals( wTrack.name(), NodeOp.getName( m_Props ) ) ) {
        menu.AppendSeparator();
        menu.Append( ME_Type  , wxGetApp().getMenu("type") );
      }
    }

    PopupMenu(&menu, 10, 10 );
}

void Symbol::OnLoc(wxCommandEvent& event) {
  iONode model = wxGetApp().getModel();
  if( model == NULL )
    return;

  //LocDialog*  dlg = new LocDialog(this, NULL, false );
  LocSelDlg*  dlg = new LocSelDlg(this, NULL, false );

  if( wxID_OK == dlg->ShowModal() ) {
    /* Notify RocRail. */
    iONode sel = dlg->getProperties();
    if( sel != NULL ) {
      const char* id = wLoc.getid( sel );
      if( id != NULL ) {
        /* Inform RocRail... */
        iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
        wLoc.setid( cmd, id );
        wLoc.setcmd( cmd, wLoc.block );
        wLoc.setblockid( cmd, wBlock.getid( m_Props ) );
        wxGetApp().sendToRocrail( cmd );
        cmd->base.del(cmd);
      }
    }
  }
  dlg->Destroy();

}

void Symbol::OnUnLoc(wxCommandEvent& event) {
  iONode model = wxGetApp().getModel();
  if( model == NULL )
    return;
  /* Inform RocRail... */
  if( StrOp.equals( wBlock.name(), NodeOp.getName( m_Props ) ) ) {
    iONode cmd = NodeOp.inst( wBlock.name(), NULL, ELEMENT_NODE );
    wBlock.setid( cmd, wBlock.getid( m_Props ) );
    wBlock.setlocid( cmd, "" );
    wBlock.setcmd( cmd, wBlock.loc );
    wxGetApp().sendToRocrail( cmd );
    cmd->base.del(cmd);
  }
  else if( StrOp.equals( wSwitch.name(), NodeOp.getName( m_Props ) ) ) {
    iONode cmd = NodeOp.inst( wSwitch.name(), NULL, ELEMENT_NODE );
    wSwitch.setid( cmd, wSwitch.getid( m_Props ) );
    wSwitch.setcmd( cmd, wSwitch.unlock );
    wxGetApp().sendToRocrail( cmd );
    cmd->base.del(cmd);
  }
  else if( StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) ) ) {
    iONode cmd = NodeOp.inst( wTurntable.name(), NULL, ELEMENT_NODE );
    wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
    wTurntable.setcmd( cmd, wSwitch.unlock );
    wxGetApp().sendToRocrail( cmd );
    cmd->base.del(cmd);
  }
  else if( StrOp.equals( wSelTab.name(), NodeOp.getName( m_Props ) ) ) {
    iONode cmd = NodeOp.inst( wSelTab.name(), NULL, ELEMENT_NODE );
    wSelTab.setid( cmd, wSelTab.getid( m_Props ) );
    wSelTab.setcmd( cmd, wSwitch.unlock );
    wxGetApp().sendToRocrail( cmd );
    cmd->base.del(cmd);
  }
}

void Symbol::OnLocGoTo(wxCommandEvent& event) {
  BlockDialog* blockDlg = new BlockDialog( this, NULL, false );
  if( wxID_OK == blockDlg->ShowModal() ) {
    iONode sel = blockDlg->getProperties();
    if( sel != NULL ) {
      const char* id = wBlock.getid( sel );

      if( id != NULL ) {
        /* Inform RocRail... */
        iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
        wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
        wLoc.setcmd( cmd, wLoc.gotoblock );
        wLoc.setblockid( cmd, id );
        wxGetApp().sendToRocrail( cmd );
        cmd->base.del(cmd);
      }
    }
  }
  blockDlg->Destroy();
}

void Symbol::OnLocSchedule(wxCommandEvent& event) {
  ScheduleDialog* dlg = new ScheduleDialog( this, (iONode)NULL, false, wBlock.getid(m_Props) );
  if( wxID_OK == dlg->ShowModal() ) {
    iONode sel = dlg->getProperties();
    if( sel != NULL ) {
      const char* id = wSchedule.getid( sel );

      if( id != NULL ) {
        /* Inform RocRail... */
        iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
        wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
        wLoc.setcmd( cmd, wLoc.useschedule );
        wLoc.setscheduleid( cmd, id );
        wxGetApp().sendToRocrail( cmd );
        cmd->base.del(cmd);
      }
    }
  }
  dlg->Destroy();
}

void Symbol::OnLocGo(wxCommandEvent& event) {
  /* Inform RocRail... */
  iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
  wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
  wLoc.setcmd( cmd, wLoc.go );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnLocGoManual(wxCommandEvent& event) {
  /* Inform RocRail... */
  iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
  wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
  wLoc.setcmd( cmd, wLoc.gomanual );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnLocStop(wxCommandEvent& event) {
  /* Inform RocRail... */
  iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
  wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
  wLoc.setcmd( cmd, wLoc.stop );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnLocReset(wxCommandEvent& event) {
  /* Inform RocRail... */
  iONode cmd = NodeOp.inst( wLoc.name(), NULL, ELEMENT_NODE );
  wLoc.setid( cmd, wBlock.getlocid( m_Props ) );
  wLoc.setcmd( cmd, wLoc.reset );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnLocMIC(wxCommandEvent& event) {
  LocSelDlg* dlg = new LocSelDlg(this, NULL, true, wBlock.getlocid( m_Props ) );
  dlg->ShowModal();
  dlg->Destroy();
}

void Symbol::OnCloseBlock(wxCommandEvent& event) {
  /* Inform RocRail... */
  iONode cmd = NodeOp.inst( NodeOp.getName(m_Props), NULL, ELEMENT_NODE );
  wBlock.setid( cmd, wBlock.getid( m_Props ) );
  wBlock.setstate( cmd, wBlock.closed );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnOpenBlock(wxCommandEvent& event) {
  /* Inform RocRail... */
  iONode cmd = NodeOp.inst( NodeOp.getName(m_Props), NULL, ELEMENT_NODE );
  wBlock.setid( cmd, wBlock.getid( m_Props ) );
  wBlock.setstate( cmd, wBlock.open );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnInfo(wxCommandEvent& event) {
  const char* name = NodeOp.getName( m_Props );
  if( StrOp.equals( wTurntable.name(), name ) ) {
    char* msg = NULL;

    iONode track = wTurntable.gettrack( m_Props );
    while( track != NULL ) {
      char* tmp = msg;
      msg = StrOp.fmt( "%s\n%s", msg==NULL?"":msg, wTTTrack.getdesc( track ) );
      StrOp.free(tmp);
      track = wTurntable.nexttrack( m_Props, track );
    }
    wxMessageBox( wxString(msg,wxConvUTF8), wxString(wTurntable.getdesc(m_Props),wxConvUTF8), wxOK, this);
    StrOp.free(msg);
  }
}

void Symbol::OnProps(wxCommandEvent& event) {
  const char* name = NodeOp.getName( m_Props );

  if( StrOp.equals( wBlock.name(), name ) ) {
    BlockDialog* blockDlg = new BlockDialog( this, m_Props );
    if( wxID_OK == blockDlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    updateLabel();
    Show(FALSE);
    Refresh();
    Show(TRUE);
    blockDlg->Destroy();
  }
  else if( StrOp.equals( wSwitch.name(), name ) ) {
    SwitchDialog* swDlg = new SwitchDialog( this, m_Props );
    if( wxID_OK == swDlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    sizeToScale();
    Show(wSwitch.isshow(m_Props));
    Refresh();
    swDlg->Destroy();
  }
  else if( StrOp.equals( wSignal.name(), name ) ) {
    SignalDialog* sgDlg = new SignalDialog( this, m_Props );
    if( wxID_OK == sgDlg->ShowModal() ) {
      // Notify RocRail.
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Refresh();
    sgDlg->Destroy();
  }
  else if( StrOp.equals( wOutput.name(), name ) ) {
    OutputDialog* coDlg = new OutputDialog( this, m_Props );
    if( wxID_OK == coDlg->ShowModal() ) {
      // Notify RocRail.
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Show(wOutput.isshow(m_Props));
    Refresh();
    coDlg->Destroy();
  }
  else if( StrOp.equals( wFeedback.name(), name ) ) {
    FeedbackDialog* fbDlg = new FeedbackDialog( this, m_Props );
    if( wxID_OK == fbDlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Show(wFeedback.isshow(m_Props));
    Refresh();
    fbDlg->Destroy();
  }
  else if( StrOp.equals( wTrack.name(), name ) ) {
    TrackDialog* tkDlg = new TrackDialog( this, m_Props );
    if( wxID_OK == tkDlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Refresh();
    tkDlg->Destroy();
  }
  else if( StrOp.equals( wText.name(), name ) ) {
    TextDialog* txDlg = new TextDialog( this, m_Props );
    if( wxID_OK == txDlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Refresh();
    txDlg->Destroy();
  }
  else if( StrOp.equals( wTurntable.name(), name ) ) {
    TurntableDialog* ttDlg = new TurntableDialog( this, m_Props );
    if( wxID_OK == ttDlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Refresh();
    ttDlg->Destroy();
  }
  else if( StrOp.equals( wSelTab.name(), name ) ) {
    /* TODO: dialog for selection table */
    SelTabDialog* dlg = new SelTabDialog( this, m_Props );
    if( wxID_OK == dlg->ShowModal() ) {
      /* Notify RocRail. */
      iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
      wModelCmd.setcmd( cmd, wModelCmd.modify );
      NodeOp.addChild( cmd, (iONode)m_Props->base.clone( m_Props ) );
      wxGetApp().sendToRocrail( cmd );
      cmd->base.del(cmd);
    }
    Refresh();
    dlg->Destroy();
  }
}


void Symbol::OnSelect(wxCommandEvent& event) {
  // inform parent panel of a select and position
  wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ME_PanelSelect );
  iONode sel = NodeOp.inst( "selection", NULL, ELEMENT_NODE );
  NodeOp.setInt( sel, "x", wItem.getx(m_Props) );
  NodeOp.setInt( sel, "y", wItem.gety(m_Props) );
  NodeOp.setInt( sel, "cx", 1 );
  NodeOp.setInt( sel, "cy", 1 );
  evt.SetClientData( sel );
  wxPostEvent( m_PlanPanel, evt );
}


void Symbol::OnRotate(wxCommandEvent& event) {
  const char* ori = wItem.getori( m_Props );

  if( event.GetId() == ME_North )
    wItem.setori( m_Props, wItem.north );
  else if( event.GetId() == ME_East )
    wItem.setori( m_Props, wItem.east );
  else if( event.GetId() == ME_South )
    wItem.setori( m_Props, wItem.south );
  else if( event.GetId() == ME_West )
    wItem.setori( m_Props, wItem.west );
  else {

    if( ori == NULL || StrOp.len(ori) == 0 ) {
      ori = wItem.west;
      wItem.setori( m_Props, wItem.west );
    }

    if( StrOp.equals( wItem.west, ori ) )
      wItem.setori( m_Props, wItem.north );
    else if( StrOp.equals( wItem.north, ori ) )
      wItem.setori( m_Props, wItem.east );
    else if( StrOp.equals( wItem.east, ori ) )
      wItem.setori( m_Props, wItem.south );
    else if( StrOp.equals( wItem.south, ori ) )
      wItem.setori( m_Props, wItem.west );
  }

  Refresh();

  /* Notify RocRail. */
  iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
  wModelCmd.setcmd( cmd, wModelCmd.modify );
  iONode item = NodeOp.inst( NodeOp.getName( m_Props ), NULL, ELEMENT_NODE );
  wItem.setid( item, wItem.getid( m_Props ) );
  wItem.setori( item, wItem.getori( m_Props ) );
  NodeOp.addChild( cmd, item );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del( cmd );
}

void Symbol::OnType(wxCommandEvent& event) {
  const char* name = NodeOp.getName( m_Props );

  if( StrOp.equals( wTrack.name(), name ) ) {
    const char* type = wTrack.gettype( m_Props );
    if( StrOp.equals( wTrack.straight, type ) )
      wTrack.settype( m_Props, wTrack.curve );
    else
      wTrack.settype( m_Props, wTrack.straight );
  }
  else if( StrOp.equals( wSwitch.name(), name ) ) {
    const char* type = wSwitch.gettype( m_Props );
    if( StrOp.equals( wSwitch.crossing, type ) )
      wSwitch.settype( m_Props, wSwitch.right );
    else if( StrOp.equals( wSwitch.right, type ) )
      wSwitch.settype( m_Props, wSwitch.left );
    else if( StrOp.equals( wSwitch.left, type ) )
      wSwitch.settype( m_Props, wSwitch.threeway );
    else
      wSwitch.settype( m_Props, wSwitch.crossing );
  }
  else
    return;

  Refresh();

  /* Notify RocRail. */
  iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
  wModelCmd.setcmd( cmd, wModelCmd.modify );
  iONode item = NodeOp.inst( NodeOp.getName( m_Props ), NULL, ELEMENT_NODE );
  wItem.setid( item, wItem.getid( m_Props ) );
  wItem.settype( item, wItem.gettype( m_Props ) );
  NodeOp.addChild( cmd, item );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del( cmd );
}

void Symbol::OnDelete(wxCommandEvent& event) {

  wxGetApp().pushUndoItem( (iONode)NodeOp.base.clone( m_Props ) );

  if( wxGetApp().isOffline() ) {
    m_PlanPanel->removeItemFromList( m_Props );
  }

  /* Notify RocRail. */
  iONode cmd = NodeOp.inst( wModelCmd.name(), NULL, ELEMENT_NODE );
  wModelCmd.setcmd( cmd, wModelCmd.remove );
  iONode item = NodeOp.inst( NodeOp.getName( m_Props ), NULL, ELEMENT_NODE );
  wItem.setid( item, wItem.getid( m_Props ) );
  NodeOp.addChild( cmd, item );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del( cmd );

}

void Symbol::OnTTLight(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wTurntable.name(), NULL, ELEMENT_NODE );
  wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
  wTurntable.setcmd( cmd, (event.GetId()==ME_TTLightOn?wTurntable.lighton:wTurntable.lightoff) );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnTTNext(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( NodeOp.getName(m_Props), NULL, ELEMENT_NODE );
  wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
  wTurntable.setcmd( cmd, wTurntable.next );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnTTPrev(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( NodeOp.getName(m_Props), NULL, ELEMENT_NODE );
  wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
  wTurntable.setcmd( cmd, wTurntable.prev );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnTT180(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( wTurntable.name(), NULL, ELEMENT_NODE );
  wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
  wTurntable.setcmd( cmd, wTurntable.turn180 );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::OnTTTrack(wxCommandEvent& event) {
  iONode cmd = NodeOp.inst( NodeOp.getName(m_Props), NULL, ELEMENT_NODE );
  wTurntable.setid( cmd, wTurntable.getid( m_Props ) );
  char* tracknr = StrOp.fmt( "%d", event.GetId() - ME_TTTrack );
  wTurntable.setcmd( cmd, tracknr );
  StrOp.free( tracknr );
  wxGetApp().sendToRocrail( cmd );
  cmd->base.del(cmd);
}

void Symbol::modelEvent( iONode node ) {
  bool refresh = false;
  const char* id = wItem.getid( node );

  TraceOp.trc( "item", TRCLEVEL_DEBUG, __LINE__, 9999,
      "id=[%s] ori=%s state=%s", id, wItem.getori(node)!=NULL?wItem.getori(node):"-", wItem.getstate(node) );

  if( StrOp.equals( NodeOp.getName( node ), NodeOp.getName( m_Props ) ) ) {
    int x = wItem.getx( node );
    int y = wItem.gety( node );
    if( x != -1 && y != -1 ) {
      wItem.setx( m_Props, x );
      wItem.sety( m_Props, y );
      if( !StrOp.equals( wModelCmd.getcmd( node), wModelCmd.move) &&  wItem.getori( node ) != NULL )
        wItem.setori( m_Props, wItem.getori( node ) );
      setPosition();
    }
    m_Renderer->initSym();
    sizeToScale();
  }

  if( !StrOp.equals( id, wItem.getid( m_Props ) ) ) {
    if( StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) ) ) {
      // Could be a turntable (invisible) feedback...
      iONode track = wTurntable.gettrack( m_Props );
      TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999, "sensor [%s] event for turntable", id );
      while( track != NULL ) {
        if( StrOp.equals( id, wTTTrack.getposfb( track ) ) ) {
          TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999, "posfb \"%s\" for nr %d = %s",
                       id, wTTTrack.getnr( track ),
                       wFeedback.isstate( node ) ? "ON":"OFF" );
          wTTTrack.setstate( track, wFeedback.isstate( node ) );
          if( wFeedback.isstate( node ) )
            wTurntable.setbridgepos(m_Props, wTTTrack.getnr( track ));
        }
        track = wTurntable.nexttrack( m_Props, track );
      }
    }
    Refresh();
    return;
  }

  if( StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ) {
    TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999, "signal set to %s", wSignal.ismanual(node) ? "manual":"auto" );
    wSignal.setmanual( m_Props, wSignal.ismanual(node) );
  }

  if( StrOp.equals( wTurntable.name(), NodeOp.getName( m_Props ) ) ) {
    // turntable bridge position
    int pos = wTurntable.getbridgepos( node );
    wTurntable.setbridgepos(m_Props, pos );
    wTurntable.setstate1(m_Props, wTurntable.isstate1( node ) );
    wTurntable.setstate2(m_Props, wTurntable.isstate2( node ) );
    TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999, "bridgepos=%d", pos );
    {
      iONode track = wTurntable.gettrack( m_Props );
      while( track != NULL ) {
        if( StrOp.equals( id, wTTTrack.getposfb( track ) ) ) {
          int tracknr = wTTTrack.getnr( track );
          wTTTrack.setstate( track, tracknr == pos ? True:False );
        }
        track = wTurntable.nexttrack( m_Props, track );
      }
    }
    refresh = true;
  }

  const char* type = wItem.gettype( node );
  if( type != NULL ) {
    if( !StrOp.equals( type, wItem.gettype( m_Props ) ) ) {
      wItem.settype( m_Props, type );
      refresh = true;
    }
  }

  const char* ori = wItem.getori( node );
  if( ori != NULL && !StrOp.equals( wModelCmd.getcmd( node), wModelCmd.move) ) {
    if( !StrOp.equals( ori, wItem.getori( m_Props ) ) ) {
        wItem.setori( m_Props, ori );
      refresh = true;
    }
  }

  if( wItem.getstate( node ) != NULL && StrOp.len(wItem.getstate( node )) > 0 ) {
    if( !StrOp.equals( wItem.getstate( node ), wItem.getstate( m_Props ) ) ) {
      wItem.setstate( m_Props, wItem.getstate( node ) );
      refresh = true;
    }
  }


  if( StrOp.equals( wFeedback.name(), NodeOp.getName( m_Props ) ) ) {
    Boolean state = wFeedback.isstate( node );
    int ident = wFeedback.getidentifier( node );
    int val = wFeedback.getval( node );
    int addr = wFeedback.getaddr( m_Props );
    const char* info = wFeedback.getinfo( node );

    char* str = StrOp.fmt( "%s addr=%d ident=%d val=%d info=%s",
                           wFeedback.getid( node ), addr, ident, val, info );
    SetToolTip( wxString(str,wxConvUTF8) );
    if( ident > 0 )
      wxGetApp().getFrame()->setInfoText( str );
    StrOp.free( str );

    if( state != wFeedback.isstate( m_Props ) ) {
      wFeedback.setstate( m_Props, state );
      refresh = true;
    }
  }
  else if( StrOp.equals( wSwitch.name(), NodeOp.getName( m_Props ) ) ) {
    const char* state = wSwitch.getstate( node );
    const char* locid = wSwitch.getlocid( node );
    Boolean isSet = wSwitch.isset(node);
    int port = wSwitch.getport1( m_Props );
    int addr = wSwitch.getaddr1( m_Props );
    int gate = wSwitch.getgate1( m_Props );
    int pada = 0;

    char* l_locidStr = NULL;
    if( state != NULL ) {
      wSwitch.setstate( m_Props, state );
      refresh = true;
    }

    wSwitch.setswitched( m_Props, wSwitch.getswitched(node) );

    SetBackgroundColour( isSet? m_PlanPanel->GetBackgroundColour():*wxRED );

    if( addr > 0 && port > 0 ) {
      pada = (addr-1) * 4 + port;
      addr = (addr-1) * 8 + (port-1) * 2 + gate;
    }
    else if( addr == 0 && port > 0 )
      addr = port;

    if( !StrOp.equals( wSwitch.decoupler, wSwitch.gettype( m_Props ) ) ) {
      l_locidStr = StrOp.fmt( "%s addr=%d(%d) lock=%s",
                             wSwitch.getid( node ),
                             addr, pada,
                             wSwitch.getlocid( node )==NULL?"unlocked":wSwitch.getlocid( node ) );
      SetToolTip( wxString(l_locidStr,wxConvUTF8) );
      StrOp.free( m_locidStr );
      m_locidStr = l_locidStr;
    }

  }
  else if( StrOp.equals( wSignal.name(), NodeOp.getName( m_Props ) ) ) {
    const char* state = wSignal.getstate( node );
    if( state != NULL ) {
      wSignal.setstate( m_Props, state );
      refresh = true;
    }

    int port = wSignal.getport1( m_Props );
    int addr = wSignal.getaddr( m_Props );
    int gate = wSignal.getgate1( m_Props );
    int pada = 0;

    char* l_tipStr = NULL;

    if( addr > 0 && port > 0 ) {
      pada = (addr-1) * 4 + port;
      addr = (addr-1) * 8 + (port-1) * 2 + gate;
    }
    else if( addr == 0 && port > 0 )
      pada = port;

    l_tipStr = StrOp.fmt( "%s addr=%d(%d)",
                           wSignal.getid( node ),
                           addr, pada );
    SetToolTip( wxString(l_tipStr,wxConvUTF8) );
    StrOp.free( m_locidStr );
    m_locidStr = l_tipStr;

  }
  else if( StrOp.equals( wOutput.name(), NodeOp.getName( m_Props ) ) ) {
    const char* state = wOutput.getstate( node );
    if( state != NULL ) {
      wOutput.setstate( m_Props, state );
      refresh = true;
    }
  }
  else if( StrOp.equals( wSelTab.name(), NodeOp.getName( m_Props ) ) ) {
    char*  l_locidStr = NULL;
    const char* locId = wSelTab.getlocid( node );
    const char* state = wSelTab.getstate( node );
    Boolean   pending = wSelTab.ispending(node);

    wSelTab.setstate( m_Props, state );
    if( StrOp.equals( wBlock.open, state ) ) {
      wSelTab.setpending( m_Props, pending );
      wSelTab.setlocid( m_Props, locId );

      if( locId != NULL && StrOp.equals( locId, "(null)") )
        locId = NULL;

      int occupied = 0;
      int tablepos = wSelTab.getpos( node );
      if( tablepos != -1 )
        wSelTab.setpos( m_Props, wSelTab.getpos( node ));
      else
        tablepos = wSelTab.getpos( m_Props );

      l_locidStr = StrOp.fmt( "%s[%d] %s",
          wSelTab.getid( m_Props ), tablepos, locId==NULL?"unlocked":locId );
    }
    else {
      l_locidStr = StrOp.fmt( "%s: CLOSED", wSelTab.getid( m_Props ) );
    }

    char* l_locidTooltipStr = StrOp.dup( l_locidStr );

    SetToolTip( wxString(l_locidTooltipStr,wxConvUTF8) );
    StrOp.free(l_locidTooltipStr);
    m_Renderer->setLabel( l_locidStr, pending );
    StrOp.free( m_locidStr );
    m_locidStr = l_locidStr;

    TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999, "id=[%s] pending=[%s] state=[%s] %s",
        id, pending?"true":"false", state, m_locidStr );

  }
  else if( StrOp.equals( wBlock.name(), NodeOp.getName( m_Props ) ) ) {
    char* l_locidStr = NULL;
    const char* state = wBlock.getstate( node );
    const char* locid = wBlock.getlocid( node );
    int occupied = 0;
    Boolean showID = True;

    iONode planpanelIni = wGui.getplanpanel(wxGetApp().getIni());
    if( planpanelIni != NULL ) {
      showID = wPlanPanel.isbktextid(planpanelIni);
    }


    wBlock.setstate( m_Props, state );
    if( StrOp.equals( wBlock.open, state ) ) {
      Boolean isReserved = wBlock.isreserved( node );
      Boolean isEntering = wBlock.isentering( node );

      wBlock.setreserved( m_Props, isReserved );
      wBlock.setlocid( m_Props, locid );

      if(showID) {
        if( wBlock.issmallsymbol(m_Props) && locid!=NULL && StrOp.len(locid)>0)
          l_locidStr = StrOp.fmt( "%s", locid );
        else
          l_locidStr = StrOp.fmt( "%s %s", wBlock.getid( node ), locid==NULL?"":locid );
      }
      else
        l_locidStr = StrOp.fmt( "%s", locid==NULL?"":locid );

      if( locid != NULL && StrOp.len( locid ) > 0 ) {
        char* tip = StrOp.fmt( wxGetApp().getMsg("clickblock").mb_str(wxConvUTF8), locid );
        SetToolTip( wxString(wBlock.getid( node ),wxConvUTF8) + _T(": ") + wxString(tip,wxConvUTF8) );
        StrOp.free( tip );
        occupied = isReserved ? 2:1;
        occupied = isEntering ? 3:occupied;
        occupied = StrOp.equals(wBlock.closed,wBlock.getstate( node ))?4:occupied;
      }
      else {
        SetToolTip( wxString(wBlock.getid( node ),wxConvUTF8) );
      }
    }
    else {
      wBlock.setlocid( m_Props, locid );
      if( StrOp.equals( wBlock.closed, state ) ) {
        l_locidStr = StrOp.fmt( "%s CLOSED", wBlock.getid( node ) );
        occupied = 4;
      }
      else if( StrOp.equals( wBlock.ghost, state ) ) {
        l_locidStr = StrOp.fmt( "%s GHOST", wBlock.getid( node ) );
        occupied = 5;
      }
      else if( StrOp.equals( wBlock.shortcut, state ) ) {
        if(showID)
          l_locidStr = StrOp.fmt( "%s %s", wBlock.getid( node ), locid==NULL?"":locid );
        else
          l_locidStr = StrOp.fmt( "%s", locid==NULL?"":locid );
        occupied = 6;
      }
      else
        l_locidStr = StrOp.fmt( "%s", wBlock.getid( node ) );

      SetToolTip( wxString(l_locidStr,wxConvUTF8) );

      if(!showID) {
        StrOp.free(l_locidStr);
        l_locidStr = StrOp.fmt( "" );
      }
    }

    TraceOp.trc( "item", TRCLEVEL_INFO, __LINE__, 9999, "id=[%s] occupied=[%d] state=[%s] locid=[%s]",
        id, occupied, state, locid!=NULL?locid:"-" );


    m_Renderer->setLabel( l_locidStr, occupied );
    StrOp.free( m_locidStr );
    m_locidStr = l_locidStr;

    m_PlanPanel->blockEvent( wBlock.getid( m_Props ) );

  }

  // In case of 2 or more panels we must refresh always because the state could be set already.
  //if( refresh )
    Refresh();

}

double Symbol::getSize() {
  double itemSize = m_ItemSize;
  return itemSize * m_Scale;
}

