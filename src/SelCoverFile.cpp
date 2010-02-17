// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2009 J.Rios
//	anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2, or (at your option)
//    any later version.
//
//    This Program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; see the file LICENSE.  If not, write to
//    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//    http://www.gnu.org/copyleft/gpl.html
//
// -------------------------------------------------------------------------------- //
#include "SelCoverFile.h"

#include <wx/filedlg.h>

// -------------------------------------------------------------------------------- //
guSelCoverFile::guSelCoverFile( wxWindow * parent, guDbLibrary * db, const int albumid ) :
    wxDialog( parent, wxID_ANY, _( "Select Cover File" ), wxDefaultPosition, wxSize( 400, 125 ), wxDEFAULT_DIALOG_STYLE )
{
    m_Db = db;
    if( albumid != wxNOT_FOUND )
    {
        wxString AlbumName;
        wxString ArtistName;
        if( !m_Db->GetAlbumInfo( albumid, &AlbumName, &ArtistName, &m_AlbumPath ) )
        {
            wxMessageBox( _( "Could not find the Album in the songs library.\n"\
                             "You should update the library." ), _( "Error" ), wxICON_ERROR | wxOK );
        }
    }

    // GUI
	SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer * MainSizer;
	MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer * ControlsSizer;
	ControlsSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText * CoverLabel = new wxStaticText( this, wxID_ANY, _( "Cover:" ), wxDefaultPosition, wxDefaultSize, 0 );
	CoverLabel->Wrap( -1 );
	ControlsSizer->Add( CoverLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_FileLink = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	ControlsSizer->Add( m_FileLink, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_SelFileBtn = new wxButton( this, wxID_ANY, wxT("..."), wxDefaultPosition, wxSize( 25,-1 ), 0 );
	ControlsSizer->Add( m_SelFileBtn, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	MainSizer->Add( ControlsSizer, 1, wxEXPAND, 5 );

	wxStdDialogButtonSizer * StdBtnSizer = new wxStdDialogButtonSizer();
	wxButton * StdBtnSizerOK = new wxButton( this, wxID_OK );
	StdBtnSizer->AddButton( StdBtnSizerOK );
	wxButton * StdBtnSizerCancel = new wxButton( this, wxID_CANCEL );
	StdBtnSizer->AddButton( StdBtnSizerCancel );
	StdBtnSizer->Realize();
	MainSizer->Add( StdBtnSizer, 0, wxEXPAND|wxALL, 5 );

	this->SetSizer( MainSizer );
	this->Layout();

	m_SelFileBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( guSelCoverFile::OnSelFileClicked ), NULL, this );
}

// -------------------------------------------------------------------------------- //
guSelCoverFile::~guSelCoverFile()
{
	m_SelFileBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( guSelCoverFile::OnSelFileClicked ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guSelCoverFile::OnSelFileClicked( wxCommandEvent& event )
{
    wxFileDialog * FileDialog = new wxFileDialog( this, _( "Select the cover filename" ),
        m_AlbumPath, wxT( "cover.jpg" ), wxT( "*.jpg;*.png" ),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW );

    if( FileDialog )
    {
        if( FileDialog->ShowModal() == wxID_OK )
        {
            m_FileLink->SetValue( FileDialog->GetPath() );
        }
        FileDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
wxString guSelCoverFile::GetSelFile( void )
{
    return m_FileLink->GetValue();
}

// -------------------------------------------------------------------------------- //