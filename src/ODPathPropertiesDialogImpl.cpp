/**************************************************************************
 * 
 * Project:  OpenCPN
 * Purpose:  ODPath Properties Support
 * Author:   Jon Gough
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "ODPathPropertiesDialogImpl.h"
#include "pathmanagerdialog.h"
#include "ODPath.h"
#include "Boundary.h"
#include "BoundaryPoint.h"
#include "EBL.h"
#include "DR.h"
#include "ocpn_draw_pi.h"
#include "ODConfig.h"
#include "ODEventHandler.h"
#include "chcanv.h"

enum {
    ID_FROM_POINT = 0,
    ID_TO_POINT,
    ID_DISTANCE_FROM_BOAT,
    ID_BEARING_FROM_BOAT,
    ID_LATITUDE,
    ID_LONGITUDE,
    ID_BEARING_FROM_TO,
    ID_DESCRIPTION,
    
    ID_POINTS_LIST_LAST
};

extern bool                 g_bShowMag;
extern ocpn_draw_pi         *g_ocpn_draw_pi;
extern double               g_dLat, g_dLon, gSog, gCog;
extern ODPlugIn_Position_Fix_Ex  g_pfFix;
extern PathList             *g_pPathList;
extern PathManagerDialog    *g_pPathManagerDialog;
extern ODConfig             *g_pODConfig;
extern wxColour             g_colourActivePathLineColour;
extern wxColour             g_colourInActivePathLineColour;
extern bool                 g_bExclusionBoundary;
extern bool                 g_bInclusionBoundary;
extern int                  g_iInclusionBoundarySize;
extern ODEventHandler       *g_ODEventHandler;

ODPathPropertiesDialogImpl::ODPathPropertiesDialogImpl() : ODPathPropertiesDialogDef( g_ocpn_draw_pi->m_parent_window )
{
    SetViewableItems();
}

ODPathPropertiesDialogImpl::ODPathPropertiesDialogImpl( wxWindow* parent ) : ODPathPropertiesDialogDef( parent )
{
    m_pPath = NULL;
    SetPointsListHeadings();    
    SetViewableItems();
}

ODPathPropertiesDialogImpl::ODPathPropertiesDialogImpl( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos,
                    const wxSize& size, long style ) : ODPathPropertiesDialogDef( parent )
{
    m_nSelected = 0;
    m_pEnroutePoint = NULL;
    m_bStartNow = false;
    m_pPath = NULL;
    
    wxFont *qFont = GetOCPNScaledFont_PlugIn(wxT("Dialog"), 0);
    SetFont( *qFont );
    
    SetPointsListHeadings();
    SetViewableItems();
    
    Centre();
}

void ODPathPropertiesDialogImpl::OnClose( wxCloseEvent& event )
{
    wxCommandEvent nullEvent;
    OnCancel( nullEvent );
}

void ODPathPropertiesDialogImpl::OnOK( wxCommandEvent& event )
{
    //    Look in the path list to be sure the path is still available
    //    (May have been deleted by PathManagerDialog...)
    
    wxPathListNode *node = g_pPathList->GetFirst();
    while( node ) {
        ODPath *pPath = node->GetData();
        
        if( pPath == m_pPath ) {
            m_pPath->m_bPathPropertiesBlink = false;
            SaveChanges();              // write changes to globals and update config
            m_pPath->ClearHighlights();
            break;
        }
        node = node->GetNext();
    }
    
    
    m_pEnroutePoint = NULL;
    m_bStartNow = false;
    
    if( g_pPathManagerDialog && g_pPathManagerDialog->IsShown() ) {
        g_pPathManagerDialog->UpdatePathListCtrl();
    }
    
    Hide();
    RequestRefresh( GetOCPNCanvasWindow() );
    
    event.Skip();
}

void ODPathPropertiesDialogImpl::OnCancel( wxCommandEvent& event )
{
    //    Look in the path list to be sure the path is still available
    //    (May have been deleted by PathMangerDialog...)
    
    wxPathListNode *node = g_pPathList->GetFirst();
    while( node ) {
        ODPath *pPath = node->GetData();
        
        if( pPath == m_pPath ) {
            m_pPath->m_bPathPropertiesBlink = false;
            m_pPath->ClearHighlights();
            break;
        }
        node = node->GetNext();
    }
    
    
    m_bStartNow = false;
    
    Hide();
    RequestRefresh( GetOCPNCanvasWindow() );
    event.Skip();
}

void ODPathPropertiesDialogImpl::OnRightClick( wxMouseEvent& event )
{
// TODO: Implement OnRightClick
    wxMenu menu;
    if( m_listCtrlODPoints->GetSelectedItemCount() == 0 ) return;
    
    if( ! m_pPath->m_bIsInLayer ) {
        wxString sPropertiesType( wxS("") );
        if ( m_pPath->m_sTypeString.IsNull() || m_pPath->m_sTypeString.IsEmpty() )
            sPropertiesType.append( wxS("OCPN Draw Point") );
        else {
            sPropertiesType.append( m_pPath->m_sTypeString );
            sPropertiesType.append( _(" Point") );
        }
        sPropertiesType.append( _(" &Properties...") );
        wxMenuItem* editItem = menu.Append( ID_PATHPROP_MENU_EDIT_PROPERTIES, sPropertiesType );
        editItem->Enable( m_listCtrlODPoints->GetSelectedItemCount() == 1 );
        
        wxMenuItem* delItem = menu.Append( ID_PATHPROP_MENU_REMOVE, _("&Remove Selected") );
        delItem->Enable( m_listCtrlODPoints->GetSelectedItemCount() > 0 );
    }
    ChartCanvas *l_parentcanvas = (ChartCanvas *)GetOCPNCanvasWindow();
    l_parentcanvas->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ODPathPropertiesDialogImpl::OnPathPropMenuSelected ), NULL, this );
    l_parentcanvas->PopupMenu( &menu );
    l_parentcanvas->Disconnect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ODPathPropertiesDialogImpl::OnPathPropMenuSelected ), NULL, this );
}

void ODPathPropertiesDialogImpl::OnLeftDoubleClick( wxMouseEvent& event )
{
// TODO: Implement OnPathPropertiesDoubleClick
    long item = -1;
    
    item = m_listCtrlODPoints->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
    
    if( item == -1 ) return;
    
    ODPoint *op = (ODPoint *) m_listCtrlODPoints->GetItemData( item );
    if( !op ) return;
    
    PathManagerDialog::ODPointShowPropertiesDialog( op, this );
}

void ODPathPropertiesDialogImpl::SetPathAndUpdate( ODPath *pP, bool only_points )
{
    if( NULL == pP )
        return;
    
    //  Fetch any config file values
    if ( !only_points )
    {
        //      long LMT_Offset = 0;                    // offset in seconds from UTC for given location (-1 hr / 15 deg W)
        m_tz_selection = 1;
        
        if( m_pPath ) {
            m_pPath->m_bPathPropertiesBlink = false;
        }
        m_pPath = pP;
        if(m_pPath->m_sTypeString == wxT("Boundary")) m_pBoundary = (Boundary *)pP;
        if(m_pPath->m_sTypeString == wxT("EBL")) m_pEBL = (EBL *)pP;
        if(m_pPath->m_sTypeString == wxT("DR")) m_pDR = (DR *)pP;
        m_pPath->m_bPathPropertiesBlink = true;
        
        m_textCtrlName->SetValue( m_pPath->m_PathNameString );
        
        m_textCtrlName->SetFocus();
    }
    m_listCtrlODPoints->DeleteAllItems();
    

    InitializeList();

    UpdateProperties( pP );

    if( m_pPath )
        m_listCtrlODPoints->Show();

    Refresh( false );

}

bool ODPathPropertiesDialogImpl::UpdateProperties( ODPath *pInPath )
{
    ODPath *pPath;
    Boundary *pBoundary = NULL;
    EBL *pEBL = NULL;
    DR *pDR = NULL;
    
    if( NULL == pInPath ) return false;
    ::wxBeginBusyCursor();
    
    if(pInPath->m_sTypeString == wxT("Boundary")) {
        pBoundary = (Boundary *)pInPath;
        pPath = pBoundary;
    } else if(pInPath->m_sTypeString == wxT("EBL")) {
        pEBL = (EBL *)pInPath;
        pPath = pEBL;
    } else if(pInPath->m_sTypeString == wxT("DR")) {
        pDR = (DR *)pInPath;
        pPath = pDR;
    } else
        pPath = pInPath;
        
    m_textCtrlName->SetValue( pPath->m_PathNameString );
    m_textCtrlDesctiption->SetValue( pPath->m_PathDescription);
    m_textCtrlGUID->SetValue( pPath->m_GUID );
    m_checkBoxActive->SetValue( pPath->IsActive() );
    
    double brgFromBoat = 0.;
    double distanceFromBoat = 0.;

    //  Total length
    double total_length = pPath->m_path_length;
    
    wxString slen;
    slen.Printf( wxT("%5.2f ") + getUsrDistanceUnit_Plugin(), toUsrDistance_Plugin( total_length ) );
    m_textCtrlTotalLength->SetValue( slen );
    
    wxString time_form;
    wxString tide_form;
    
    //  Iterate on Path Points
    wxODPointListNode *node = pPath->m_pODPointList->GetFirst();
    double slat, slon;
    slat = g_pfFix.Lat;
    slon = g_pfFix.Lon;
    wxString nullify = _T("----");
    long item_line_index = 0;
    wxString wxsLastPoint = _("Boat");
    while( node ) {
        ODPoint *pOp = node->GetData();
        wxString t;
        //  Leg
        m_listCtrlODPoints->SetItem( item_line_index, ID_FROM_POINT, wxsLastPoint );
        
        if(pOp->m_MarkName != _("Boat")) {
        //  ODPoint Name
            m_listCtrlODPoints->SetItem( item_line_index, ID_TO_POINT, pOp->GetName() );
            wxsLastPoint = pOp->GetName();
            // Store Description
            m_listCtrlODPoints->SetItem( item_line_index, ID_DESCRIPTION, pOp->GetDescription() );
            
            //  Distance
            //  Note that Distance/Bearing for Leg 000 is as from current position
            DistanceBearingMercator_Plugin( pOp->m_lat, pOp->m_lon, slat, slon, &brgFromBoat, &distanceFromBoat );
            
            t.Printf( _T("%6.2f ") + getUsrDistanceUnit_Plugin(), toUsrDistance_Plugin( distanceFromBoat ) );
            m_listCtrlODPoints->SetItem( item_line_index, ID_DISTANCE_FROM_BOAT, t );
            pOp->SetDistance( distanceFromBoat ); // save the course to the next point for printing.
            //  Bearing
            if( g_bShowMag )
                t.Printf( _T("%03.0f Deg. M"), g_ocpn_draw_pi->GetTrueOrMag( brgFromBoat ) );
            else
                t.Printf( _T("%03.0f Deg. T"), g_ocpn_draw_pi->GetTrueOrMag( brgFromBoat ) );
            
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_BOAT, t );
        } else {
            m_listCtrlODPoints->SetItem( item_line_index, ID_TO_POINT, wxT("") );
            m_listCtrlODPoints->SetItem( item_line_index, ID_DISTANCE_FROM_BOAT, wxT("") );
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_BOAT, wxT("") );
        }
        
        // calculation of bearging from current point to next point.
        double brgFromTo, tmp_leg_dist;
        wxODPointListNode *next_node = node->GetNext();
        ODPoint * p_next_pOp = (next_node)? next_node->GetData(): NULL;
        if ( p_next_pOp ) {
            DistanceBearingMercator_Plugin( p_next_pOp->m_lat, p_next_pOp->m_lon, pOp->m_lat, pOp->m_lon, &brgFromTo, &tmp_leg_dist );
        }else {
            brgFromTo = 0.0;
            tmp_leg_dist = 0.0;
        }
        
        if (p_next_pOp){
            if( g_bShowMag )
                t.Printf( _T("%03.0f Deg. M"), g_ocpn_draw_pi->GetTrueOrMag( brgFromTo ) );
            else
                t.Printf( _T("%03.0f Deg. T"), g_ocpn_draw_pi->GetTrueOrMag( brgFromTo ) );
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_TO, t );
        }
        else
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_TO, nullify );
        
        //  Lat/Lon
        wxString tlat = toSDMM_PlugIn( 1, pOp->m_lat, pOp->m_bIsInTrack );  // low precision for routes
        m_listCtrlODPoints->SetItem( item_line_index, ID_LATITUDE, tlat );
        
        wxString tlon = toSDMM_PlugIn( 2, pOp->m_lon, pOp->m_bIsInTrack );
        m_listCtrlODPoints->SetItem( item_line_index, ID_LONGITUDE, tlon );
        
        item_line_index++;

        node = node->GetNext();
        
    }
    
//    if( pPath->m_ActiveLineColour == wxEmptyString ) m_colourPickerLineColour->SetColour( g_colourActivePathLineColour );
    m_colourPickerLineColour->SetColour( pPath->m_wxcActiveLineColour );

    for( unsigned int i = 0; i < sizeof( ::StyleValues ) / sizeof(int); i++ ) {
        if( pPath->m_style == ::StyleValues[i] ) {
            m_choiceLineStyle->Select( i );
            break;
        }
    }
    
    for( unsigned int i = 0; i < sizeof( ::WidthValues ) / sizeof(int); i++ ) {
        if( pPath->m_width == ::WidthValues[i] ) {
            m_choiceLineWidth->Select( i );
            break;
        }
    }
    
    // Set column width correctly for data
    for(int i = 0; i < m_listCtrlODPoints->GetColumnCount(); i++) {
#ifdef WIN32
        m_listCtrlODPoints->SetColumnWidth( i, wxLIST_AUTOSIZE_USEHEADER );
#else WIN32        
        m_listCtrlODPoints->SetColumnWidth( i, wxLIST_AUTOSIZE );
#endif WIN32        
    }
    
    ::wxEndBusyCursor();
    
    return true;
}

bool ODPathPropertiesDialogImpl::UpdateProperties( void )
{
    ::wxBeginBusyCursor();
    
    //  Iterate on Path Points
    wxODPointListNode *node = m_pPath->m_pODPointList->GetFirst();
    
    double brgFromBoat = 0.;
    double distanceFromBoat = 0.;
    
    wxString nullify = _T("----");
    long item_line_index = 0;
    wxString wxsLastPoint = wxT("Boat");
    while( node ) {
        ODPoint *pOp = node->GetData();
        wxString t;
        //  Leg
        m_listCtrlODPoints->SetItem( item_line_index, ID_FROM_POINT, wxsLastPoint );

        if(pOp->m_MarkName != _("Boat")) {
            //  ODPoint Name
            m_listCtrlODPoints->SetItem( item_line_index, ID_TO_POINT, pOp->GetName() );
            wxsLastPoint = pOp->GetName();
            // Store Description
            m_listCtrlODPoints->SetItem( item_line_index, ID_DESCRIPTION, pOp->GetDescription() );
            
            //  Distance
            //  Note that Distance/Bearing for Leg 000 is as from current position
            DistanceBearingMercator_Plugin(  g_dLat, g_dLon, pOp->m_lat, pOp->m_lon, &brgFromBoat, &distanceFromBoat );
            
            t.Printf( _T("%6.2f ") + getUsrDistanceUnit_Plugin(), toUsrDistance_Plugin( distanceFromBoat ) );
            m_listCtrlODPoints->SetItem( item_line_index, ID_DISTANCE_FROM_BOAT, t );
            pOp->SetDistance( distanceFromBoat ); // save the course to the next point for printing.
            //  Bearing
            if( g_bShowMag )
                t.Printf( _T("%03.0f Deg. M"), g_ocpn_draw_pi->GetTrueOrMag( brgFromBoat ) );
            else
                t.Printf( _T("%03.0f Deg. T"), g_ocpn_draw_pi->GetTrueOrMag( brgFromBoat ) );
            
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_BOAT, t );
        } else {
            m_listCtrlODPoints->SetItem( item_line_index, ID_TO_POINT, wxT("") );
            m_listCtrlODPoints->SetItem( item_line_index, ID_DISTANCE_FROM_BOAT, wxT("") );
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_BOAT, wxT("") );
        }
        
        // calculation of bearging from current point to next point.
        double brgFromTo, tmp_leg_dist;
        wxODPointListNode *next_node = node->GetNext();
        ODPoint * _next_pOp = (next_node)? next_node->GetData(): NULL;
        if (_next_pOp ) {
            DistanceBearingMercator_Plugin( pOp->m_lat, pOp->m_lon, _next_pOp->m_lat, _next_pOp->m_lon, &brgFromTo, &tmp_leg_dist );
        }else {
            brgFromTo = 0.0;
            tmp_leg_dist = 0.0;
        }
        
        if (_next_pOp){
            if( g_bShowMag )
                t.Printf( _T("%03.0f Deg. M"), g_ocpn_draw_pi->GetTrueOrMag( brgFromTo ) );
            else
                t.Printf( _T("%03.0f Deg. T"), g_ocpn_draw_pi->GetTrueOrMag( brgFromTo ) );
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_TO, t );
        }
        else
            m_listCtrlODPoints->SetItem( item_line_index, ID_BEARING_FROM_TO, nullify );
        
        //  Lat/Lon
        wxString tlat = toSDMM_PlugIn( 1, pOp->m_lat, pOp->m_bIsInTrack );  // low precision for routes
        m_listCtrlODPoints->SetItem( item_line_index, ID_LATITUDE, tlat );
        
        wxString tlon = toSDMM_PlugIn( 2, pOp->m_lon, pOp->m_bIsInTrack );
        m_listCtrlODPoints->SetItem( item_line_index, ID_LONGITUDE, tlon );
        
        item_line_index++;
        node = node->GetNext();
        
    }

    // Set column width correctly for data
    for(int i = 0; i < m_listCtrlODPoints->GetColumnCount(); i++) {
#ifdef WIN32
        m_listCtrlODPoints->SetColumnWidth( i, wxLIST_AUTOSIZE_USEHEADER );
#else WIN32        
        m_listCtrlODPoints->SetColumnWidth( i, wxLIST_AUTOSIZE );
#endif WIN32        
    }
    
    this->GetSizer()->Fit( this );
    this->Layout();
    
    ::wxEndBusyCursor();
    
    return true;
}

void ODPathPropertiesDialogImpl::SetDialogTitle(const wxString & title)
{
    SetTitle(title);
}

bool ODPathPropertiesDialogImpl::SaveChanges( void )
{
    
    if( m_pPath && !m_pPath->m_bIsInLayer ) {
        //  Get User input Text Fields
        m_pPath->m_PathNameString = m_textCtrlName->GetValue();
        m_pPath->m_PathDescription = m_textCtrlDesctiption->GetValue();
        m_pPath->m_bPathIsActive = m_checkBoxActive->GetValue();
        
        m_pPath->m_wxcActiveLineColour = m_colourPickerLineColour->GetColour();
        m_pPath->SetActiveColours();
        m_pPath->m_style = ::StyleValues[m_choiceLineStyle->GetSelection()];
        m_pPath->m_width = ::WidthValues[m_choiceLineWidth->GetSelection()];
        
        g_pODConfig->UpdatePath( m_pPath );
        //g_ocpn_draw_pi->SaveConfig();
    }
    
    if( m_pPath->IsActive() )
    {
        wxString msg_id( wxS("OCPN_PATH_ACTIVATED") );
        wxString msg;
        msg.append( wxS("Name: ") );
        msg.append( m_pPath->m_PathNameString.c_str() );
        msg.append( wxS(", GUID: ") );
        msg.append( m_pPath->m_GUID );
        SendPluginMessage( msg_id, msg );
        
    }
    
    return true;
}

void ODPathPropertiesDialogImpl::InitializeList()
{
    if( NULL == m_pPath ) return;
    
    //  Iterate on Route Points, inserting blank fields starting with index 0
    wxODPointListNode *pnode = m_pPath->m_pODPointList->GetFirst();
    int in = 0;
    while( pnode ) {
        m_listCtrlODPoints->InsertItem( in, _T(""), 0 );
        m_listCtrlODPoints->SetItemPtrData( in, (wxUIntPtr)pnode->GetData() );
        in++;
        if( pnode->GetData()->m_seg_etd.IsValid() ) {
            m_listCtrlODPoints->InsertItem( in, _T(""), 0 );
            in++;
        }
        pnode = pnode->GetNext();
    }
}

void ODPathPropertiesDialogImpl::SetPointsListHeadings()
{
    m_listCtrlODPoints->DeleteAllColumns();
    m_listCtrlODPoints->InsertColumn( ID_FROM_POINT, _("From Point"), wxLIST_FORMAT_LEFT );
    m_listCtrlODPoints->InsertColumn( ID_TO_POINT, _("To Point"), wxLIST_FORMAT_LEFT );
    m_listCtrlODPoints->InsertColumn( ID_DISTANCE_FROM_BOAT, _("Distance From Boat"), wxLIST_FORMAT_RIGHT );
    
    if(g_bShowMag)
        m_listCtrlODPoints->InsertColumn( ID_BEARING_FROM_BOAT, _("Bearing (M)"), wxLIST_FORMAT_LEFT );
    else
        m_listCtrlODPoints->InsertColumn( ID_BEARING_FROM_BOAT, _("Bearing"), wxLIST_FORMAT_LEFT );
    
    m_listCtrlODPoints->InsertColumn( ID_LATITUDE, _("Latitude"), wxLIST_FORMAT_LEFT );
    m_listCtrlODPoints->InsertColumn( ID_LONGITUDE, _("Longitude"), wxLIST_FORMAT_LEFT );
    if(g_bShowMag)
        m_listCtrlODPoints->InsertColumn( ID_BEARING_FROM_TO, _("Bearing From-To (M)"), wxLIST_FORMAT_LEFT );
    else
        m_listCtrlODPoints->InsertColumn( ID_BEARING_FROM_TO, _("Bearing From-To"), wxLIST_FORMAT_LEFT );
    m_listCtrlODPoints->InsertColumn( ID_DESCRIPTION, _("Description"), wxLIST_FORMAT_LEFT );
}

void ODPathPropertiesDialogImpl::SetViewableItems()
{
    m_pPath = NULL;
    SetPointsListHeadings();
    m_staticTextFillColour->Hide();
    m_staticTextFillColour->Enable( false );
    m_colourPickerFillColour->Hide();
    m_colourPickerFillColour->Enable( false );
    m_staticTextFillTransparency->Hide();
    m_staticTextFillTransparency->Enable( false );
    m_sliderFillTransparency->Hide();
    m_sliderFillTransparency->Enable( false );
    m_staticTextIncluseionBoundarySize->Hide();
    m_sliderInclusionBoundarySize->Hide();
    m_sliderInclusionBoundarySize->Enable( false );
    m_bSizerBoundaryType->ShowItems( false );
    m_radioBoxBoundaryType->Hide();
    m_radioBoxBoundaryType->Enable( false );
    m_fgSizerPath->ShowItems( false );
    m_radioBoxPathPersistence->Hide();
    m_radioBoxPathPersistence->Enable( false );
    m_fgSizerEBL->ShowItems( false );
    m_checkBoxEBLFixedEndPosition->Hide();
    m_checkBoxEBLFixedEndPosition->Enable( false );
    m_checkBoxPathShowArrow->Hide();
    m_checkBoxPathShowArrow->Enable( false );
    m_radioBoxPathPersistence->Hide();
    m_radioBoxPathPersistence->Enable( false );
    
    return;
}

void ODPathPropertiesDialogImpl::OnPathPropMenuSelected( wxCommandEvent& event )
{
    switch( event.GetId() ) {
        case ID_PATHPROP_MENU_DELETE: {
            //OnRoutepropCopyTxtClick( event );
            break;
        }
        case ID_PATHPROP_MENU_REMOVE: {
            int dlg_return = OCPNMessageBox( this, _("Are you sure you want to remove this point?"),
                                             _("OD Remove ODPoint"), (long) wxYES_NO | wxCANCEL | wxYES_DEFAULT );
            
            if( dlg_return == wxID_YES ) {
                long item = -1;
                item = m_listCtrlODPoints->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
                
                if( item == -1 ) break;
                
                ODPoint *odp;
                odp = (ODPoint *) m_listCtrlODPoints->GetItemData( item );
                
                m_pPath->RemovePointFromPath( odp, m_pPath );
            }
            break;
        }
        case ID_PATHPROP_MENU_EDIT_PROPERTIES: {
            long item = -1;
            
            item = m_listCtrlODPoints->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
            
            if( item == -1 ) break;
            
            ODPoint *odp;
            odp = (ODPoint *) m_listCtrlODPoints->GetItemData( item );
            if( !odp ) break;
            
            g_pPathManagerDialog->ODPointShowPropertiesDialog( odp, GetParent() );
            break;
        }
    }
}

