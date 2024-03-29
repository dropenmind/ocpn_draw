///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ODToolbarDef.h"

///////////////////////////////////////////////////////////////////////////

ODToolbarDialog::ODToolbarDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizerToolbar;
	bSizerToolbar = new wxBoxSizer( wxVERTICAL );
	
	bSizerToolbar->SetMinSize( wxSize( 20,20 ) ); 
	m_toolBarODToolbar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxNO_BORDER ); 
	m_toolBarODToolbar->Realize(); 
	
	bSizerToolbar->Add( m_toolBarODToolbar, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerToolbar );
	this->Layout();
	bSizerToolbar->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_ACTIVATE, wxActivateEventHandler( ODToolbarDialog::OnActivate ) );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ODToolbarDialog::OnClose ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( ODToolbarDialog::OnSize ) );
}

ODToolbarDialog::~ODToolbarDialog()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler( ODToolbarDialog::OnActivate ) );
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ODToolbarDialog::OnClose ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( ODToolbarDialog::OnSize ) );
	
}
