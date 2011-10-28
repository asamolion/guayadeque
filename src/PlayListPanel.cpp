// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2011 J.Rios
//	anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3, or (at your option)
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
#include "PlayListPanel.h"

#include "Accelerators.h"
#include "AuiNotebook.h"
#include "AuiDockArt.h"
#include "Commands.h"
#include "Config.h"
#include "DbLibrary.h"
#include "DynamicPlayList.h"
#include "EditWithOptions.h"
#include "FileRenamer.h"
#include "Images.h"
#include "LabelEditor.h"
#include "MainFrame.h"
#include "MediaViewer.h"
#include "PlayListAppend.h"
#include "PlayListFile.h"
#include "TagInfo.h"
#include "TrackEdit.h"
#include "Utils.h"

//#define guPLAYLIST_TIMER_TEXTSEARCH        5
//#define guPLAYLIST_TIMER_TEXTSEARCH_VALUE  500

// -------------------------------------------------------------------------------- //
class guPLNamesData : public wxTreeItemData
{
  private :
    int         m_Id;
    int         m_Type;

  public :
    guPLNamesData( const int id, const int type ) { m_Id = id; m_Type = type; };
    int         GetData( void ) { return m_Id; };
    void        SetData( int id ) { m_Id = id; };
    int         GetType( void ) { return m_Type; };
    void        SetType( int type ) { m_Type = type; };
};


BEGIN_EVENT_TABLE( guPLNamesTreeCtrl, wxTreeCtrl )
    EVT_TREE_BEGIN_DRAG( wxID_ANY, guPLNamesTreeCtrl::OnBeginDrag )
END_EVENT_TABLE()

// -------------------------------------------------------------------------------- //
guPLNamesTreeCtrl::guPLNamesTreeCtrl( wxWindow * parent, guDbLibrary * db, guPlayListPanel * playlistpanel ) :
    wxTreeCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT|wxTR_FULL_ROW_HIGHLIGHT|wxTR_MULTIPLE )
{
    m_Db = db;

    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->RegisterObject( this );

    m_PlayListPanel = playlistpanel;
    m_ImageList = new wxImageList();
    m_ImageList->Add( wxBitmap( guImage( guIMAGE_INDEX_track ) ) );
    m_ImageList->Add( wxBitmap( guImage( guIMAGE_INDEX_system_run ) ) );

    AssignImageList( m_ImageList );

    m_RootId   = AddRoot( wxT( "Playlists" ), -1, -1, NULL );
    m_StaticId = AppendItem( m_RootId, _( "Static playlists" ), 0, 0, NULL );
    m_DynamicId = AppendItem( m_RootId, _( "Dynamic playlists" ), 1, 1, NULL );

    SetIndent( 5 );

    SetDropTarget( new guPLNamesDropTarget( this ) );

    Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guPLNamesTreeCtrl::OnContextMenu ), NULL, this );
    Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( guPLNamesTreeCtrl::OnKeyDown ), NULL, this );

    Connect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guPLNamesTreeCtrl::OnConfigUpdated ), NULL, this );

    CreateAcceleratorTable();

    ReloadItems();
}

// -------------------------------------------------------------------------------- //
guPLNamesTreeCtrl::~guPLNamesTreeCtrl()
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->UnRegisterObject( this );

    Disconnect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guPLNamesTreeCtrl::OnContextMenu ), NULL, this );
    Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( guPLNamesTreeCtrl::OnKeyDown ), NULL, this );

    Disconnect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guPLNamesTreeCtrl::OnConfigUpdated ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnConfigUpdated( wxCommandEvent &event )
{
    int Flags = event.GetInt();
    if( Flags & guPREFERENCE_PAGE_FLAG_ACCELERATORS )
    {
        CreateAcceleratorTable();
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::CreateAcceleratorTable( void )
{
    wxAcceleratorTable AccelTable;
    wxArrayInt AliasAccelCmds;
    wxArrayInt RealAccelCmds;

    AliasAccelCmds.Add( ID_PLAYER_PLAYLIST_SEARCH );

    RealAccelCmds.Add( ID_PLAYLIST_SEARCH );

    if( guAccelDoAcceleratorTable( AliasAccelCmds, RealAccelCmds, AccelTable ) )
    {
        SetAcceleratorTable( AccelTable );
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::ReloadItems( void )
{
    int index;
    int count;

    DeleteChildren( m_StaticId );
    DeleteChildren( m_DynamicId );

    guListItems m_StaticItems;
    m_Db->GetPlayLists( &m_StaticItems, guPLAYLIST_TYPE_STATIC, &m_TextSearchFilter );
    if( ( count = m_StaticItems.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            AppendItem( m_StaticId, m_StaticItems[ index ].m_Name, -1, -1,
                                new guPLNamesData( m_StaticItems[ index ].m_Id, guPLAYLIST_TYPE_STATIC ) );
        }
    }

    guListItems m_DynamicItems;
    m_Db->GetPlayLists( &m_DynamicItems, guPLAYLIST_TYPE_DYNAMIC, &m_TextSearchFilter );
    if( ( count = m_DynamicItems.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            AppendItem( m_DynamicId, m_DynamicItems[ index ].m_Name, -1, -1,
                                new guPLNamesData( m_DynamicItems[ index ].m_Id, guPLAYLIST_TYPE_DYNAMIC ) );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnContextMenu( wxTreeEvent &event )
{
    wxMenu Menu;
    wxMenuItem * MenuItem;

    wxPoint Point = event.GetPoint();
    wxArrayTreeItemIds SelectedItems;
    int SelectCount = GetSelections( SelectedItems );

    wxTreeItemId ItemId = event.GetItem();
    guPLNamesData * ItemData = NULL;

    if( ItemId.IsOk() )
    {
        ItemData = ( guPLNamesData * ) GetItemData( ItemId );

        if( ItemData )
        {
            MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_PLAY, _( "Play" ), _( "Play current selected songs" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_player_tiny_light_play ) );
            Menu.Append( MenuItem );

            MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_ENQUEUE_AFTER_ALL,
                                wxString( _( "Enqueue" ) ) +  guAccelGetCommandKeyCodeString( ID_SONG_ENQUEUE_AFTER_ALL ),
                                _( "Add current selected songs to the playlist" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_add ) );
            Menu.Append( MenuItem );

            wxMenu * EnqueueMenu = new wxMenu();

            MenuItem = new wxMenuItem( EnqueueMenu, ID_PLAYLIST_ENQUEUE_AFTER_TRACK,
                                    wxString( _( "Current Track" ) ) +  guAccelGetCommandKeyCodeString( ID_SONG_ENQUEUE_AFTER_TRACK ),
                                    _( "Add current selected tracks to playlist after the current track" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
            EnqueueMenu->Append( MenuItem );

            MenuItem = new wxMenuItem( EnqueueMenu, ID_PLAYLIST_ENQUEUE_AFTER_ALBUM,
                                    wxString( _( "Current Album" ) ) +  guAccelGetCommandKeyCodeString( ID_SONG_ENQUEUE_AFTER_ALBUM ),
                                    _( "Add current selected tracks to playlist after the current album" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
            EnqueueMenu->Append( MenuItem );

            MenuItem = new wxMenuItem( EnqueueMenu, ID_PLAYLIST_ENQUEUE_AFTER_ARTIST,
                                    wxString( _( "Current Artist" ) ) +  guAccelGetCommandKeyCodeString( ID_SONG_ENQUEUE_AFTER_ARTIST ),
                                    _( "Add current selected tracks to playlist after the current artist" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
            EnqueueMenu->Append( MenuItem );

            Menu.Append( wxID_ANY, _( "Enqueue After" ), EnqueueMenu );

            Menu.AppendSeparator();
        }
    }

    MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_NEWPLAYLIST, _( "New Dynamic Playlist" ), _( "Create a new dynamic playlist" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_new ) );
    Menu.Append( MenuItem );

    Menu.AppendSeparator();

    MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_IMPORT, _( "Import" ), _( "Import a playlist" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_new ) );
    Menu.Append( MenuItem );

    if( ItemData )
    {
        MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_EXPORT, _( "Export" ), _( "Export the playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_save ) );
        Menu.Append( MenuItem );

        Menu.AppendSeparator();

        if( SelectCount == 1 )
        {
            if( ItemData->GetType() == guPLAYLIST_TYPE_DYNAMIC )
            {
                MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_EDIT, _( "Edit" ), _( "Edit the selected playlist" ) );
                MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit ) );
                Menu.Append( MenuItem );

                MenuItem = new wxMenuItem( &Menu, ID_SONG_SAVETOPLAYLIST, _( "Save to Playlist" ), _( "Save the selected playlist as a Static Playlist" ) );
                MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_save ) );
                Menu.Append( MenuItem );
            }

            MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_RENAME, _( "Rename" ), _( "Change the name of the selected playlist" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit ) );
            Menu.Append( MenuItem );
        }

        MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_DELETE, _( "Delete" ), _( "Delete the selected playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit_delete ) );
        Menu.Append( MenuItem );

        Menu.AppendSeparator();

        MenuItem = new wxMenuItem( &Menu, ID_MAINFRAME_SET_ALLOW_PLAYLIST, _( "Set as Alow Filter" ), _( "Set this playlist as the allow filter" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_accept ) );
        Menu.Append( MenuItem );

        MenuItem = new wxMenuItem( &Menu, ID_MAINFRAME_SET_DENY_PLAYLIST, _( "Set as Deny Filter" ), _( "Delete the selected playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_filter ) );
        Menu.Append( MenuItem );

        Menu.AppendSeparator();

        guMainFrame * MainFrame = ( guMainFrame * ) wxTheApp->GetTopWindow();
        MainFrame->CreateCopyToMenu( &Menu );
    }

    PopupMenu( &Menu, Point );

    event.Skip();
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnBeginDrag( wxTreeEvent &event )
{
    int Index;
    int Count;
    wxArrayTreeItemIds SelectedItems;
    if( ( Count = GetSelections( SelectedItems ) ) )
    {
        wxFileDataObject Files;

        for( Index = 0; Index < Count; Index++ )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) GetItemData( SelectedItems[ Index ] );
            if( ItemData )
            {

                if( ItemData->GetType() == guPLAYLIST_TYPE_STATIC )
                {
                    wxString PlayListPath = m_Db->GetPlayListPath( ItemData->GetData() );
                    if( !PlayListPath.IsEmpty() )
                    {
                        // If the playlist was added from a file
                        Files.AddFile( PlayListPath );
                    }
                    else
                    {
                        //m_Db->GetPlayListFiles( ItemData->GetData(), &Files );
                        int Index;
                        int Count;
                        guTrackArray Tracks;
                        m_Db->GetPlayListSongs( ItemData->GetData(), guPLAYLIST_TYPE_STATIC, &Tracks, NULL, NULL );
                        //m_PLTracksListBox->GetAllSongs( &Tracks );
                        if( ( Count = Tracks.Count() ) )
                        {
                            guPlayListFile PlayListFile;

                            for( Index = 0; Index < Count; Index++ )
                            {
                                PlayListFile.AddItem( Tracks[ Index ].m_FileName,
                                    Tracks[ Index ].m_ArtistName + wxT( " - " ) + Tracks[ Index ].m_SongName );
                            }

                            wxString PlayListName = m_Db->GetPlayListName( ItemData->GetData() );
                            PlayListFile.SetName( PlayListName );

                            PlayListPath = wxGetHomeDir() + wxT( "/.guayadeque/PlayLists/") + PlayListName + wxT( ".m3u" );
                            wxFileName::Mkdir( wxPathOnly( PlayListPath ), 0777, wxPATH_MKDIR_FULL );
                            PlayListFile.Save( PlayListPath );
                            m_Db->SetPlayListPath( ItemData->GetData(), PlayListPath );

                            Files.AddFile( PlayListPath );
                        }
                    }
                }
                else
                {
                    guTrackArray Tracks;
                    m_Db->GetPlayListSongs( ItemData->GetData(), ItemData->GetType(), &Tracks, NULL, NULL );
                    int index;
                    int count = Tracks.Count();
                    for( index = 0; index < count; index++ )
                    {
                        Files.AddFile( Tracks[ index ].m_FileName );
                    }
                }
            }
        }
        wxDropSource source( Files, this );

        wxDragResult Result = source.DoDragDrop();
        if( Result )
        {
        }

        // If we left any hightligted item remove it
        if( m_DragOverItem.IsOk() )
        {
            SetItemDropHighlight( m_DragOverItem, false );
            m_DragOverItem = wxTreeItemId();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnDragOver( const wxCoord x, const wxCoord y )
{
    int HitFlags;
    wxTreeItemId TreeItemId = HitTest( wxPoint( x, y ), HitFlags );

    if( TreeItemId.IsOk() )
    {
        if( TreeItemId != m_DragOverItem )
        {
            if( m_DragOverItem.IsOk() )
            {
                SetItemDropHighlight( m_DragOverItem, false );
            }
            guPLNamesData * ItemData = ( guPLNamesData * ) GetItemData( TreeItemId );
            if( ItemData && ItemData->GetType() == guPLAYLIST_TYPE_STATIC )
            {
                SetItemDropHighlight( TreeItemId, true );
                m_DragOverItem = TreeItemId;
            }
        }
    }
    else
    {
        if( m_DragOverItem.IsOk() )
        {
            SetItemDropHighlight( m_DragOverItem, false );
            m_DragOverItem = wxTreeItemId();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnDropFile( const wxString &filename )
{
    if( guIsValidAudioFile( filename ) )
    {
        if( wxFileExists( filename ) )
        {
            guTrack Track;
            if( m_Db->FindTrackFile( filename, &Track ) )
            {
                m_DropIds.Add( Track.m_SongId );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnDropEnd( void )
{
    if( m_DragOverItem.IsOk() )
    {
        SetItemDropHighlight( m_DragOverItem, false );
        if( m_DropIds.Count() )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) GetItemData( m_DragOverItem );
            if( ItemData && ItemData->GetType() == guPLAYLIST_TYPE_STATIC )
            {
                m_Db->AppendStaticPlayList( ItemData->GetData(), m_DropIds );
                m_Db->UpdateStaticPlayListFile( ItemData->GetData() );
            }

            SelectItem( m_StaticId );
            SelectItem( m_DragOverItem );
        }
        m_DragOverItem = wxTreeItemId();
    }
    m_DropIds.Clear();
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnKeyDown( wxKeyEvent &event )
{
    if( event.GetKeyCode() == WXK_DELETE )
    {
        wxCommandEvent CmdEvent( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_DELETE );
        wxPostEvent( this, CmdEvent );
        return;
    }
    event.Skip();
}


// -------------------------------------------------------------------------------- //
// guPLNamesDropFilesThread
// -------------------------------------------------------------------------------- //
guPLNamesDropFilesThread::guPLNamesDropFilesThread( guPLNamesDropTarget * plnamesdroptarget,
                             guPLNamesTreeCtrl * plnamestreectrl, const wxArrayString &files ) :
    wxThread()
{
    m_PLNamesTreeCtrl = plnamestreectrl;
    m_Files = files;
    m_PLNamesDropTarget = plnamesdroptarget;

    if( Create() == wxTHREAD_NO_ERROR )
    {
        SetPriority( WXTHREAD_DEFAULT_PRIORITY );
        Run();
    }
}

// -------------------------------------------------------------------------------- //
guPLNamesDropFilesThread::~guPLNamesDropFilesThread()
{
//    printf( "guPLNamesDropFilesThread Object destroyed\n" );
    if( !TestDestroy() )
    {
        m_PLNamesDropTarget->ClearPlayListFilesThread();
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesDropFilesThread::AddDropFiles( const wxString &DirName )
{
    wxDir Dir;
    wxString FileName;
    wxString SavedDir( wxGetCwd() );

    //printf( "Entering Dir : " ); printf( ( char * ) DirName.char_str() );  ; printf( "\n" );
    if( wxDirExists( DirName ) )
    {
        //wxMessageBox( DirName, wxT( "DirName" ) );
        Dir.Open( DirName );
        wxSetWorkingDirectory( DirName );
        if( Dir.IsOpened() )
        {
            if( Dir.GetFirst( &FileName, wxEmptyString, wxDIR_FILES | wxDIR_DIRS ) )
            {
                do {
                    if( ( FileName[ 0 ] != '.' ) )
                    {
                        if( Dir.Exists( FileName ) )
                        {
                            AddDropFiles( DirName + wxT( "/" ) + FileName );
                        }
                        else
                        {
                            m_PLNamesTreeCtrl->OnDropFile( DirName + wxT( "/" ) + FileName );
                        }
                    }
                } while( Dir.GetNext( &FileName ) && !TestDestroy() );
            }
        }
    }
    else
    {
        m_PLNamesTreeCtrl->OnDropFile( DirName );
    }
    wxSetWorkingDirectory( SavedDir );
}

// -------------------------------------------------------------------------------- //
guPLNamesDropFilesThread::ExitCode guPLNamesDropFilesThread::Entry()
{
    int index;
    int Count = m_Files.Count();
    for( index = 0; index < Count; ++index )
    {
        if( TestDestroy() )
            break;
        AddDropFiles( m_Files[ index ] );
    }

    if( !TestDestroy() )
    {
        //
        m_PLNamesTreeCtrl->OnDropEnd();
    }

    return 0;
}

// -------------------------------------------------------------------------------- //
// guPLNamesDropTarget
// -------------------------------------------------------------------------------- //
guPLNamesDropTarget::guPLNamesDropTarget( guPLNamesTreeCtrl * plnamestreectrl )
{
    m_PLNamesTreeCtrl = plnamestreectrl;
    m_PLNamesDropFilesThread = NULL;
}

// -------------------------------------------------------------------------------- //
guPLNamesDropTarget::~guPLNamesDropTarget()
{
//    printf( "guPLNamesDropTarget Object destroyed\n" );
}

// -------------------------------------------------------------------------------- //
bool guPLNamesDropTarget::OnDropFiles( wxCoord x, wxCoord y, const wxArrayString &files )
{
    if( m_PLNamesDropFilesThread )
    {
        m_PLNamesDropFilesThread->Pause();
        m_PLNamesDropFilesThread->Delete();
    }

    m_PLNamesDropFilesThread = new guPLNamesDropFilesThread( this, m_PLNamesTreeCtrl, files );

    if( !m_PLNamesDropFilesThread )
    {
        guLogError( wxT( "Could not create the add files thread." ) );
    }

    return true;
}

// -------------------------------------------------------------------------------- //
wxDragResult guPLNamesDropTarget::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
    //printf( "guPLNamesDropTarget::OnDragOver... %d - %d\n", x, y );
    m_PLNamesTreeCtrl->OnDragOver( x, y );
    return wxDragCopy;
}




// -------------------------------------------------------------------------------- //
// guPlayListPanel
// -------------------------------------------------------------------------------- //
guPlayListPanel::guPlayListPanel( wxWindow * parent, guMediaViewer * mediaviewer ) :
            guAuiManagerPanel( parent )
{
    m_MediaViewer = mediaviewer;
    m_Db = mediaviewer->GetDb();
    m_PlayerPanel = mediaviewer->GetPlayerPanel();
    m_ConfigPath = mediaviewer->ConfigPath() + wxT( "/playlists" );
    m_ExportLastFolder = wxGetHomeDir();

    guConfig * Config = ( guConfig * ) guConfig::Get();

    m_VisiblePanels = Config->ReadNum( wxT( "VisiblePanels" ), guPANEL_PLAYLIST_VISIBLE_DEFAULT, m_ConfigPath );

    InitPanelData();

    CreateControls();
}

// -------------------------------------------------------------------------------- //
guPlayListPanel::~guPlayListPanel()
{
    // Save the Splitter positions into the main config
    guConfig * Config = ( guConfig * ) guConfig::Get();

    Config->WriteNum( wxT( "VisiblePanels" ), m_VisiblePanels, m_ConfigPath );
    Config->WriteStr( wxT( "LastLayout" ), m_AuiManager.SavePerspective(), m_ConfigPath );

	Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( guPlayListPanel::OnPLNamesSelected ), NULL, this );
	Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( guPlayListPanel::OnPLNamesActivated ), NULL, this );
    Disconnect( ID_PLAYLIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesPlay ), NULL, this );
    Disconnect( ID_PLAYLIST_ENQUEUE_AFTER_ALL, ID_PLAYLIST_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEnqueue ), NULL, this );
    Disconnect( ID_PLAYLIST_NEWPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesNewPlaylist ), NULL, this );
    Disconnect( ID_PLAYLIST_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEditPlaylist ), NULL, this );
    Disconnect( ID_PLAYLIST_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesRenamePlaylist ), NULL, this );
    Disconnect( ID_PLAYLIST_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesDeletePlaylist ), NULL, this );
    m_NamesTreeCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesCopyTo ), NULL, this );

    Disconnect( ID_PLAYLIST_IMPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesImport ), NULL, this );
    Disconnect( ID_PLAYLIST_EXPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesExport ), NULL, this );

    Disconnect( ID_MAINFRAME_SET_ALLOW_PLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnSetAllowDenyFilter ), NULL, this );

    m_PLTracksListBox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guPlayListPanel::OnPLTracksActivated ), NULL, this );
    Disconnect( ID_SONG_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteClicked ), NULL, this );
    Disconnect( ID_SONG_DELETE_LIBRARY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteLibrary ), NULL, this );
    Disconnect( ID_SONG_DELETE_DRIVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteDrive ), NULL, this );
    Disconnect( ID_SONG_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksPlayClicked ), NULL, this );
    Disconnect( ID_SONG_ENQUEUE_AFTER_ALL, ID_SONG_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksQueueClicked ), NULL, this );
    Disconnect( ID_SONG_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditLabelsClicked ), NULL, this );
    Disconnect( ID_SONG_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditTracksClicked ), NULL, this );
    m_PLTracksListBox->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksCopyToClicked ), NULL, this );
    Disconnect( ID_SONG_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSavePlayListClicked ), NULL, this );
    Disconnect( ID_SONG_SET_RATING_0, ID_SONG_SET_RATING_5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSetRating ), NULL, this );
    Disconnect( ID_SONG_SET_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSetField ), NULL, this );
    Disconnect( ID_SONG_EDIT_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditField ), NULL, this );

    Disconnect( ID_SONG_BROWSE_GENRE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectGenre ), NULL, this );
    Disconnect( ID_SONG_BROWSE_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectArtist ), NULL, this );
    Disconnect( ID_SONG_BROWSE_ALBUMARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectAlbumArtist ), NULL, this );
    Disconnect( ID_SONG_BROWSE_ALBUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectAlbum ), NULL, this );

    Disconnect( ID_PLAYLIST_SEARCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnGoToSearch ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::CreateControls( void )
{
    guConfig * Config = ( guConfig * ) guConfig::Get();

    wxPanel * NamesPanel;
	NamesPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * NameSizer;
	NameSizer = new wxBoxSizer( wxVERTICAL );

	m_NamesTreeCtrl = new guPLNamesTreeCtrl( NamesPanel, m_Db, this );
	m_NamesTreeCtrl->ExpandAll();
	NameSizer->Add( m_NamesTreeCtrl, 1, wxEXPAND, 5 );

	NamesPanel->SetSizer( NameSizer );
	NamesPanel->Layout();
	NameSizer->Fit( NamesPanel );

    m_AuiManager.AddPane( NamesPanel,
            wxAuiPaneInfo().Name( wxT( "PlayListNames" ) ).Caption( _( "Playlists" ) ).
            MinSize( 50, 50 ).CloseButton( false ).
            Dockable( true ).Left() );


    wxPanel *  DetailsPanel;
	DetailsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * DetailsSizer;
	DetailsSizer = new wxBoxSizer( wxVERTICAL );

	m_PLTracksListBox = new guPLSoListBox( DetailsPanel, m_MediaViewer, wxT( "playlist" ), guLISTVIEW_COLUMN_SELECT );
	DetailsSizer->Add( m_PLTracksListBox, 1, wxEXPAND, 5 );

	DetailsPanel->SetSizer( DetailsSizer );
	DetailsPanel->Layout();
	DetailsSizer->Fit( DetailsPanel );

    m_AuiManager.AddPane( DetailsPanel, wxAuiPaneInfo().Name( wxT( "PlayListTracks" ) ).Caption( wxT( "PlayList" ) ).
            MinSize( 50, 50 ).
            CenterPane() );

    wxString PlayListLayout = Config->ReadStr( wxT( "LastLayout" ), wxEmptyString, m_ConfigPath );
    if( Config->GetIgnoreLayouts() || PlayListLayout.IsEmpty() )
    {
        m_VisiblePanels = guPANEL_PLAYLIST_VISIBLE_DEFAULT;
        //m_AuiManager.Update();
        PlayListLayout = wxT( "layout2|name=PlayListNames;caption=" ) + wxString( _( "Playlists" ) );
        PlayListLayout += wxT( ";state=2044;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=180;besth=350;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        PlayListLayout += wxT( "name=PlayListTracks;caption=Playlists;state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        PlayListLayout += wxT( "dock_size(1,2,0)=47|dock_size(4,0,0)=186|dock_size(5,0,0)=52|" );
    }

    m_AuiManager.LoadPerspective( PlayListLayout, true );


	Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( guPlayListPanel::OnPLNamesSelected ), NULL, this );
	Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( guPlayListPanel::OnPLNamesActivated ), NULL, this );
    Connect( ID_PLAYLIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesPlay ), NULL, this );
    Connect( ID_PLAYLIST_ENQUEUE_AFTER_ALL, ID_PLAYLIST_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEnqueue ), NULL, this );
    Connect( ID_PLAYLIST_NEWPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesNewPlaylist ), NULL, this );
    Connect( ID_PLAYLIST_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEditPlaylist ), NULL, this );
    Connect( ID_PLAYLIST_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesRenamePlaylist ), NULL, this );
    Connect( ID_PLAYLIST_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesDeletePlaylist ), NULL, this );
    m_NamesTreeCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesCopyTo ), NULL, this );

    Connect( ID_PLAYLIST_IMPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesImport ), NULL, this );
    Connect( ID_PLAYLIST_EXPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesExport ), NULL, this );

    Connect( ID_MAINFRAME_SET_ALLOW_PLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnSetAllowDenyFilter ), NULL, this );
    Connect( ID_MAINFRAME_SET_DENY_PLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnSetAllowDenyFilter ), NULL, this );

    m_PLTracksListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guPlayListPanel::OnPLTracksActivated ), NULL, this );
    Connect( ID_SONG_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteClicked ), NULL, this );
    Connect( ID_SONG_DELETE_LIBRARY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteLibrary ), NULL, this );
    Connect( ID_SONG_DELETE_DRIVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteDrive ), NULL, this );
    Connect( ID_SONG_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksPlayClicked ), NULL, this );
    Connect( ID_SONG_ENQUEUE_AFTER_ALL, ID_SONG_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksQueueClicked ), NULL, this );
    Connect( ID_SONG_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditLabelsClicked ), NULL, this );
    Connect( ID_SONG_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditTracksClicked ), NULL, this );
    m_PLTracksListBox->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksCopyToClicked ), NULL, this );
    Connect( ID_SONG_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSavePlayListClicked ), NULL, this );
    Connect( ID_SONG_SET_RATING_0, ID_SONG_SET_RATING_5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSetRating ), NULL, this );
    Connect( ID_SONG_SET_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSetField ), NULL, this );
    Connect( ID_SONG_EDIT_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditField ), NULL, this );

    Connect( ID_SONG_BROWSE_GENRE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectGenre ), NULL, this );
    Connect( ID_SONG_BROWSE_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectArtist ), NULL, this );
    Connect( ID_SONG_BROWSE_ALBUMARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectAlbumArtist ), NULL, this );
    Connect( ID_SONG_BROWSE_ALBUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectAlbum ), NULL, this );

    Connect( ID_PLAYLIST_SEARCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnGoToSearch ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::InitPanelData()
{
}

// -------------------------------------------------------------------------------- //
bool guPlayListPanel::DoTextSearch( const wxString &textsearch )
{
    if( m_LastSearchString != textsearch )
    {
        m_LastSearchString = textsearch; //m_InputTextCtrl->GetLineText( 0 );
        if( !m_LastSearchString.IsEmpty() )
        {
            if( m_LastSearchString.Length() > 1 )
            {
                m_NamesTreeCtrl->m_TextSearchFilter = guSplitWords( m_LastSearchString );
                m_NamesTreeCtrl->ExpandAll();
                m_NamesTreeCtrl->ReloadItems();
            }
            return true;
        }
        else
        {
            m_NamesTreeCtrl->m_TextSearchFilter.Clear();
            m_NamesTreeCtrl->ExpandAll();
            m_NamesTreeCtrl->ReloadItems();
        }
    }
    return false;
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesSelected( wxTreeEvent& event )
{
    wxArrayTreeItemIds Selections;
    if( m_NamesTreeCtrl->GetSelections( Selections ) )
    {
        wxArrayInt Ids;
        wxArrayInt Types;
        int Index;
        int Count = Selections.Count();
        for( Index = 0; Index < Count; Index++ )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( Selections[ Index ] );
            if( ItemData )
            {
                Ids.Add( ItemData->GetData() );
                Types.Add( ItemData->GetType() );
            }
        }
        m_PLTracksListBox->SetPlayList( Ids, Types );
    }
    else
    {
        m_PLTracksListBox->SetPlayList( -1, -1 );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesActivated( wxTreeEvent& event )
{
    guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( event.GetItem() );
    if( ItemData )
    {
        guTrackArray Tracks;
        m_PLTracksListBox->GetAllSongs( &Tracks );
        if( Tracks.Count() )
        {
            NormalizeTracks( &Tracks );
            guConfig * Config = ( guConfig * ) guConfig::Get();
            if( Config )
            {
                if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
                {
                    m_PlayerPanel->AddToPlayList( Tracks );
                }
                else
                {
                    m_PlayerPanel->SetPlayList( Tracks );
                }
            }
        }
    }
    event.Skip();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesPlay( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guTrackArray Tracks;
        m_PLTracksListBox->GetAllSongs( &Tracks );
        NormalizeTracks( &Tracks );
        m_PlayerPanel->SetPlayList( Tracks );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesEnqueue( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guTrackArray Tracks;
        m_PLTracksListBox->GetAllSongs( &Tracks );
        NormalizeTracks( &Tracks );
        m_PlayerPanel->AddToPlayList( Tracks, true, event.GetId() - ID_PLAYLIST_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesNewPlaylist( wxCommandEvent &event )
{
    guDynPlayList DynPlayList;
    guDynPlayListEditor * PlayListEditor = new guDynPlayListEditor( this, &DynPlayList );
    if( PlayListEditor->ShowModal() == wxID_OK )
    {
        PlayListEditor->FillPlayListEditData();

        m_Db->CreateDynamicPlayList( DynPlayList.m_Name, &DynPlayList );

        SendPlayListUpdatedEvent();
    }
    PlayListEditor->Destroy();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesEditPlaylist( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
        guDynPlayList DynPlayList;
        m_Db->GetDynamicPlayList( ItemData->GetData(), &DynPlayList );
        guDynPlayListEditor * PlayListEditor = new guDynPlayListEditor( this, &DynPlayList );
        if( PlayListEditor->ShowModal() == wxID_OK )
        {
            PlayListEditor->FillPlayListEditData();
            m_Db->UpdateDynamicPlayList( ItemData->GetData(), &DynPlayList );
            m_NamesTreeCtrl->ReloadItems();
        }
        PlayListEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesRenamePlaylist( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "Playlist Name: " ),
          _( "Enter the new playlist name" ), m_NamesTreeCtrl->GetItemText( ItemId ) );
        if( EntryDialog->ShowModal() == wxID_OK )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
            wxASSERT( ItemData );
            m_Db->SetPlayListName( ItemData->GetData(), EntryDialog->GetValue() );
            if( ItemData->GetType() == guPLAYLIST_TYPE_STATIC )
                m_Db->UpdateStaticPlayListFile( ItemData->GetData() );
            //m_NamesTreeCtrl->SetItemText( ItemId, EntryDialog->GetValue() );
            SendPlayListUpdatedEvent();
        }
        EntryDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::DeleteCurrentPlayList( void )
{
    wxArrayTreeItemIds SelectedItems;
    int Index;
    int Count = m_NamesTreeCtrl->GetSelections( SelectedItems );
    if( Count )
    {
        for( Index = 0; Index < Count; Index++ )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( SelectedItems[ Index ] );
            if( ItemData )
            {
                m_Db->DeletePlayList( ItemData->GetData() );
                if( ItemData->GetType() == guPLAYLIST_TYPE_STATIC )
                    m_Db->UpdateStaticPlayListFile( ItemData->GetData() );
            }
        }
        SendPlayListUpdatedEvent();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesDeletePlaylist( wxCommandEvent &event )
{
    if( wxMessageBox( _( "Are you sure to delete the selected Playlist?" ),
                      _( "Confirm" ),
                      wxICON_QUESTION|wxYES_NO|wxNO_DEFAULT, this ) == wxYES )
    {
        DeleteCurrentPlayList();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesCopyTo( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        int Index = event.GetId() - ID_COPYTO_BASE;
        if( Index >= guCOPYTO_DEVICE_BASE )
        {
            Index -= guCOPYTO_DEVICE_BASE;
            event.SetInt( Index );

            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
            if( ItemData )
            {
                wxString PlayListPath = m_Db->GetPlayListPath( ItemData->GetData() );
                if( !PlayListPath.IsEmpty() )
                {
                    event.SetId( ID_MAINFRAME_COPYTODEVICE_PLAYLIST );
                    event.SetClientData( new wxString( PlayListPath ) );
                    wxPostEvent( wxTheApp->GetTopWindow(), event );
                }
                else
                {
                    int Index;
                    int Count;
                    guTrackArray Tracks;
                    m_PLTracksListBox->GetAllSongs( &Tracks );
                    if( ( Count = Tracks.Count() ) )
                    {
                        guPlayListFile PlayListFile;

                        NormalizeTracks( &Tracks );

                        for( Index = 0; Index < Count; Index++ )
                        {
                            PlayListFile.AddItem( Tracks[ Index ].m_FileName,
                                Tracks[ Index ].m_ArtistName + wxT( " - " ) + Tracks[ Index ].m_SongName );
                        }

                        wxString PlayListName = m_NamesTreeCtrl->GetItemText( ItemId );
                        PlayListFile.SetName( PlayListName );

                        PlayListPath = wxGetHomeDir() + wxT( "/.guayadeque/PlayLists/") + PlayListName + wxT( ".m3u" );
                        wxFileName::Mkdir( wxPathOnly( PlayListPath ), 0777, wxPATH_MKDIR_FULL );
                        PlayListFile.Save( PlayListPath );
                        m_Db->SetPlayListPath( ItemData->GetData(), PlayListPath );

                        event.SetId( ID_MAINFRAME_COPYTODEVICE_PLAYLIST );
                        event.SetClientData( new wxString( PlayListPath ) );
                        wxPostEvent( wxTheApp->GetTopWindow(), event );
                    }
                }
            }
        }
        else
        {
            event.SetId( ID_MAINFRAME_COPYTO );

            guTrackArray * Tracks = new guTrackArray();
            m_PLTracksListBox->GetAllSongs( Tracks );
            NormalizeTracks( Tracks );

            event.SetInt( Index );
            event.SetClientData( ( void * ) Tracks );
            wxPostEvent( wxTheApp->GetTopWindow(), event );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesImport( wxCommandEvent &event )
{
    int Index;
    int Count;

    wxFileDialog * FileDialog = new wxFileDialog( this,
        _( "Select the playlist file" ),
        wxGetHomeDir(),
        wxEmptyString,
        wxT( "*.m3u;*.pls;*.asx;*.xspf" ),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( FileDialog )
    {
        if( FileDialog->ShowModal() == wxID_OK )
        {
            guPlayListFile PlayListFile( FileDialog->GetPath() );
            if( ( Count = PlayListFile.Count() ) )
            {
                if( PlayListFile.GetName().IsEmpty() )
                {
                    wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "Playlist Name: " ),
                      _( "Enter the new playlist name" ), FileDialog->GetPath().AfterLast( wxT( '/' ) ).BeforeLast( wxT( '.' ) ) );
                    if( EntryDialog->ShowModal() == wxID_OK )
                    {
                        PlayListFile.SetName( EntryDialog->GetValue() );
                    }
                    delete EntryDialog;
                }

                //
                if( PlayListFile.GetName().IsEmpty() )
                {
                    PlayListFile.SetName( _( "New Playlist" ) );
                }

                wxArrayInt Songs;
                for( Index = 0; Index < Count; Index++ )
                {
                    wxURI Uri( PlayListFile.GetItem( Index ).m_Location );
                    wxString FileName = Uri.BuildUnescapedURI();
                    if( FileName.StartsWith( wxT( "file:" ) ) )
                        FileName = FileName.Mid( 5 );
                    //guLogMessage( wxT( "Trying to add file '%s'" ), FileName.c_str() );
                    int SongId = m_Db->FindTrackFile( FileName, NULL );
                    if( SongId )
                    {
                        Songs.Add( SongId );
                        //guLogMessage( wxT( "Found it!" ) );
                    }
                    //else
                    //    guLogMessage( wxT( "Not Found it!" ) );
                }

                if( Songs.Count() )
                {
                    int PLId = m_Db->CreateStaticPlayList( PlayListFile.GetName(), Songs );
                    m_Db->UpdateStaticPlayListFile( PLId );
                }

                //m_NamesTreeCtrl->ReloadItems();
                SendPlayListUpdatedEvent();
            }
        }
        FileDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesExport( wxCommandEvent &event )
{
    int Index;
    int Count;

    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        wxString PlayListName = m_NamesTreeCtrl->GetItemText( ItemId );

        wxFileDialog * FileDialog = new wxFileDialog( this,
            _( "Select the playlist file" ),
            m_ExportLastFolder,
            PlayListName + wxT( ".m3u" ),
            wxT( "*.m3u;*.pls;*.asx;*.xspf" ),
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( FileDialog )
        {
            if( FileDialog->ShowModal() == wxID_OK )
            {
                guPlayListFile PlayListFile;

                PlayListFile.SetName( PlayListName );

                guTrackArray Tracks;

                m_PLTracksListBox->GetAllSongs( &Tracks );
                if( ( Count = Tracks.Count() ) )
                {
                    NormalizeTracks( &Tracks );
                    for( Index = 0; Index < Count; Index++ )
                    {
                        PlayListFile.AddItem( Tracks[ Index ].m_FileName,
                            Tracks[ Index ].m_ArtistName + wxT( " - " ) + Tracks[ Index ].m_SongName );
                    }

                    PlayListFile.Save( FileDialog->GetPath() );
                }
            }
            m_ExportLastFolder = wxPathOnly( FileDialog->GetPath() );
            FileDialog->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksActivated( wxListEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( Tracks.Count() )
    {
        NormalizeTracks( &Tracks );
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Tracks );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Tracks );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksDeleteClicked( wxCommandEvent &event )
{
    wxArrayInt DelTracks;

    m_PLTracksListBox->GetPlayListSetIds( &DelTracks );

    if( DelTracks.Count() )
    {
        wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
        if( ItemId.IsOk() )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
            if( ItemData && ItemData->GetType() == guPLAYLIST_TYPE_STATIC )
            {
                m_Db->DelPlaylistSetIds( ItemData->GetData(), DelTracks );
                m_Db->UpdateStaticPlayListFile( ItemData->GetData() );
                m_PLTracksListBox->ReloadItems();
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksPlayClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        m_PLTracksListBox->GetAllSongs( &Tracks );
    NormalizeTracks( &Tracks );
    m_PlayerPanel->SetPlayList( Tracks );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksQueueClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        m_PLTracksListBox->GetAllSongs( &Tracks );
    NormalizeTracks( &Tracks );
    m_PlayerPanel->AddToPlayList( Tracks, true, event.GetId() - ID_SONG_ENQUEUE_AFTER_ALL );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksEditLabelsClicked( wxCommandEvent &event )
{
    guListItems Tracks;
    m_PLTracksListBox->GetSelectedItems( &Tracks, false );
    if( Tracks.Count() )
    {
        guArrayListItems LabelSets = m_Db->GetSongsLabels( m_PLTracksListBox->GetSelectedItems( false ) );

        guLabelEditor * LabelEditor = new guLabelEditor( this, m_Db, _( "Tracks Labels Editor" ), false, &Tracks, &LabelSets );
        if( LabelEditor )
        {
            if( LabelEditor->ShowModal() == wxID_OK )
            {
                // Update the labels in the files
                m_Db->UpdateSongsLabels( LabelSets );
            }
            LabelEditor->Destroy();
            m_PLTracksListBox->ReloadItems( false );
        }
    }

}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;
    guImagePtrArray Images;
    wxArrayString Lyrics;
    wxArrayInt ChangedFlags;

    guTrackEditor * TrackEditor = new guTrackEditor( this, m_Db, &Tracks, &Images, &Lyrics, &ChangedFlags );
    if( TrackEditor )
    {
        if( TrackEditor->ShowModal() == wxID_OK )
        {
            m_MediaViewer->UpdateTracks( Tracks, Images, Lyrics, ChangedFlags );

            m_PLTracksListBox->ReloadItems();

            // Update the track in database, playlist, etc
//            ( ( guMainFrame * ) wxTheApp->GetTopWindow() )->UpdatedTracks( guUPDATED_TRACKS_PLAYLISTS, &Tracks );
        }
        guImagePtrArrayClean( &Images );
        TrackEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_PLTracksListBox->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );

    if( ( count = Tracks.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            NewSongs.Add( Tracks[ index ].m_SongId );
        }
    }
    else
    {
        m_PLTracksListBox->GetAllSongs( &Tracks );
        count = Tracks.Count();
        for( index = 0; index < count; index++ )
        {
            NewSongs.Add( Tracks[ index ].m_SongId );
        }
    }

    if( NewSongs.Count() );
    {
        guListItems PlayLists;
        m_Db->GetPlayLists( &PlayLists, guPLAYLIST_TYPE_STATIC );

        guPlayListAppend * PlayListAppendDlg = new guPlayListAppend( wxTheApp->GetTopWindow(), m_Db, &NewSongs, &PlayLists );

        if( PlayListAppendDlg->ShowModal() == wxID_OK )
        {
            int Selected = PlayListAppendDlg->GetSelectedPlayList();
            if( Selected == wxNOT_FOUND )
            {
                wxString PLName = PlayListAppendDlg->GetPlaylistName();
                if( PLName.IsEmpty() )
                {
                    PLName = _( "UnNamed" );
                }
                int PLId = m_Db->CreateStaticPlayList( PLName, NewSongs );
                m_Db->UpdateStaticPlayListFile( PLId );
            }
            else
            {
                int PLId = PlayLists[ Selected ].m_Id;
                wxArrayInt OldSongs;
                m_Db->GetPlayListSongIds( PLId, &OldSongs );
                if( PlayListAppendDlg->GetSelectedPosition() == 0 ) // BEGIN
                {
                    m_Db->UpdateStaticPlayList( PLId, NewSongs );
                    m_Db->AppendStaticPlayList( PLId, OldSongs );
                }
                else                                                // END
                {
                    m_Db->AppendStaticPlayList( PLId, NewSongs );
                }
                m_Db->UpdateStaticPlayListFile( PLId );
            }
            SendPlayListUpdatedEvent();
        }
        PlayListAppendDlg->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSetRating( wxCommandEvent &event )
{
    int Rating = event.GetId() - ID_SONG_SET_RATING_0;
    //guLogMessage( wxT( "OnSongSetRating( %i )" ), Rating );

    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );

    m_MediaViewer->SetTracksRating( Tracks, Rating );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSetField( wxCommandEvent &event )
{
    int ColumnId = m_PLTracksListBox->GetColumnId( m_PLTracksListBox->GetLastColumnClicked() );
    //guLogMessage( wxT( "guPlayListPanel::OnSongSetField %i" ), ColumnId );

    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );

    wxVariant NewData = m_PLTracksListBox->GetLastDataClicked();

    //guLogMessage( wxT( "Setting Data to : %s" ), NewData.GetString().c_str() );

    // This should be done in a thread for huge selections of tracks...
    wxArrayInt ChangedFlags;
    int Index;
    int Count = Tracks.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        ChangedFlags.Add( guTRACK_CHANGED_DATA_TAGS );
        guTrack * Track = &Tracks[ Index ];
        switch( ColumnId )
        {
            case guSONGS_COLUMN_NUMBER :
                Track->m_Number = NewData.GetLong();
                break;

            case guSONGS_COLUMN_TITLE :
                Track->m_SongName = NewData.GetString();
                break;

            case guSONGS_COLUMN_ARTIST :
                Track->m_ArtistName = NewData.GetString();
                break;

            case guSONGS_COLUMN_ALBUMARTIST :
                Track->m_AlbumArtist = NewData.GetString();
                break;

            case guSONGS_COLUMN_ALBUM :
                Track->m_AlbumName = NewData.GetString();
                break;

            case guSONGS_COLUMN_GENRE :
                Track->m_GenreName = NewData.GetString();
                break;

            case guSONGS_COLUMN_COMPOSER :
                Track->m_Composer = NewData.GetString();
                break;

            case guSONGS_COLUMN_DISK :
                Track->m_Disk = NewData.GetString();
                break;

            case guSONGS_COLUMN_YEAR :
                Track->m_Year = NewData.GetLong();
                break;

        }
    }

    m_Db->UpdateSongs( &Tracks, ChangedFlags );

    m_MediaViewer->UpdatedTracks( guUPDATED_TRACKS_NONE, &Tracks );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksEditField( wxCommandEvent &event )
{
    int ColumnId = m_PLTracksListBox->GetColumnId( m_PLTracksListBox->GetLastColumnClicked() );
    //guLogMessage( wxT( "guLibPanel::OnSongSetField %i" ), ColumnId );

    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );

    wxString Label = m_PLTracksListBox->GetColumnNames()[ ColumnId ];
    wxVariant DefValue = m_PLTracksListBox->GetLastDataClicked();

    wxArrayString Items;

    int Index;
    int Count = Tracks.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        wxVariant Value;
        guTrack * Track = &Tracks[ Index ];

        switch( ColumnId )
        {
            case guSONGS_COLUMN_NUMBER :
                Value = ( long ) Track->m_Number;
                break;

            case guSONGS_COLUMN_TITLE :
                Value = Track->m_SongName;
                break;

            case guSONGS_COLUMN_ARTIST :
                Value = Track->m_ArtistName;
                break;

            case guSONGS_COLUMN_ALBUMARTIST :
                Value = Track->m_AlbumArtist;
                break;

            case guSONGS_COLUMN_ALBUM :
                Value = Track->m_AlbumName;
                break;

            case guSONGS_COLUMN_GENRE :
                Value = Track->m_GenreName;
                break;

            case guSONGS_COLUMN_COMPOSER :
                Value = Track->m_Composer;
                break;

            case guSONGS_COLUMN_DISK :
                Value = Track->m_Disk;
                break;

            case guSONGS_COLUMN_YEAR :
                Value = ( long ) Track->m_Year;
                break;
        }
        if( Items.Index( Value.GetString() ) == wxNOT_FOUND )
            Items.Add( Value.GetString() );
    }

    guEditWithOptions * FieldEditor = new guEditWithOptions( this, _( "Field Editor" ), Label, DefValue.GetString(), Items );

    if( FieldEditor )
    {
        if( FieldEditor->ShowModal() == wxID_OK )
        {
            DefValue = FieldEditor->GetData();

            //guLogMessage( wxT( "Setting Data to : %s" ), DefValue.GetString().c_str() );

            // This should be done in a thread for huge selections of tracks...
            wxArrayInt ChangedFlags;
            int Index;
            int Count = Tracks.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                ChangedFlags.Add( guTRACK_CHANGED_DATA_TAGS );
                guTrack * Track = &Tracks[ Index ];
                switch( ColumnId )
                {
                    case guSONGS_COLUMN_NUMBER :
                        Track->m_Number = DefValue.GetLong();
                        break;

                    case guSONGS_COLUMN_TITLE :
                        Track->m_SongName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_ARTIST :
                        Track->m_ArtistName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_ALBUMARTIST :
                        Track->m_AlbumArtist = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_ALBUM :
                        Track->m_AlbumName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_GENRE :
                        Track->m_GenreName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_COMPOSER :
                        Track->m_Composer = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_DISK :
                        Track->m_Disk = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_YEAR :
                        Track->m_Year = DefValue.GetLong();
                        break;
                }
            }

            m_Db->UpdateSongs( &Tracks, ChangedFlags );

            m_MediaViewer->UpdatedTracks( guUPDATED_TRACKS_NONE, &Tracks );
        }
        FieldEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::PlayListUpdated( void )
{
    //guLogMessage( wxT( "guPLayListPanel::PlayListUpdated" ) );
    m_NamesTreeCtrl->ReloadItems();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectGenre( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Genres = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Genres->Index( Tracks[ index ].m_GenreId ) == wxNOT_FOUND )
        {
            Genres->Add( Tracks[ index ].m_GenreId );
        }
    }

    event.SetId( ID_GENRE_SETSELECTION );
    event.SetClientData( ( void * ) Genres );
    wxPostEvent( m_MediaViewer, event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectArtist( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Artists = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Artists->Index( Tracks[ index ].m_ArtistId ) == wxNOT_FOUND )
        {
            Artists->Add( Tracks[ index ].m_ArtistId );
        }
    }
    event.SetId( ID_ARTIST_SETSELECTION );
    event.SetClientData( ( void * ) Artists );
    wxPostEvent( m_MediaViewer, event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectAlbumArtist( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Ids = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Ids->Index( Tracks[ index ].m_AlbumArtistId ) == wxNOT_FOUND )
        {
            Ids->Add( Tracks[ index ].m_AlbumArtistId );
        }
    }
    event.SetId( ID_ALBUMARTIST_SETSELECTION );
    event.SetClientData( ( void * ) Ids );
    wxPostEvent( m_MediaViewer, event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectAlbum( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Albums = new wxArrayInt();

    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Albums->Index( Tracks[ index ].m_AlbumId ) == wxNOT_FOUND )
        {
            Albums->Add( Tracks[ index ].m_AlbumId );
        }
    }
    event.SetId( ID_ALBUM_SETSELECTION );
    event.SetClientData( ( void * ) Albums );
    wxPostEvent( m_MediaViewer, event );
}

// -------------------------------------------------------------------------------- //
bool guPlayListPanel::GetPlayListCounters( wxLongLong * count, wxLongLong * len, wxLongLong * size )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
        if( ItemData )
        {
//            m_Db->GetPlayListCounters( ItemData->GetData(), ItemData->GetType(), count, len, size );
            m_PLTracksListBox->GetCounters( count, len, size );
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksDeleteLibrary( wxCommandEvent &event )
{
    if( m_PLTracksListBox->GetSelectedCount() )
    {
        if( wxMessageBox( wxT( "Are you sure to remove the selected tracks from your library?" ),
            wxT( "Remove tracks from library" ), wxICON_QUESTION|wxYES|wxNO|wxNO_DEFAULT ) == wxYES )
        {
            guTrackArray Tracks;
            m_PLTracksListBox->GetSelectedSongs( &Tracks );
            //
            m_Db->DeleteLibraryTracks( &Tracks, true );

            m_PLTracksListBox->ReloadItems();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksDeleteDrive( wxCommandEvent &event )
{
    if( m_PLTracksListBox->GetSelectedCount() )
    {
        if( wxMessageBox( wxT( "Are you sure to delete the selected tracks from your drive?\nThis will permanently erase the selected tracks." ),
            wxT( "Remove tracks from drive" ), wxICON_QUESTION|wxYES|wxNO|wxNO_DEFAULT ) == wxYES )
        {
            guTrackArray Tracks;
            m_PLTracksListBox->GetSelectedSongs( &Tracks );
            //
            m_Db->DeleteLibraryTracks( &Tracks, false );
            //
            int Index;
            int Count = Tracks.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                if( !wxRemoveFile( Tracks[ Index ].m_FileName ) )
                {
                    guLogMessage( wxT( "Error deleting '%s'" ), Tracks[ Index ].m_FileName.c_str() );
                }
            }

            m_PLTracksListBox->ReloadItems();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::SendPlayListUpdatedEvent( void )
{
////    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_UPDATED );
////    wxPostEvent( m_MediaViewer, evt );
//    m_NamesTreeCtrl->ReloadItems();
//
//    m_MediaViewer->PlayListUpdated();
    m_MediaViewer->UpdatePlaylists();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnGoToSearch( wxCommandEvent &event )
{
    m_MediaViewer->GoToSearch();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnSetAllowDenyFilter( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
        if( ItemData )
        {
            event.SetInt( ItemData->GetData() );

            wxPostEvent( wxTheApp->GetTopWindow(), event );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::NormalizeTracks( guTrackArray * tracks, const bool isdrag )
{
    if( m_MediaViewer )
        m_MediaViewer->NormalizeTracks( tracks, isdrag );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::UpdatePlaylists( void )
{
    m_NamesTreeCtrl->ReloadItems();
    m_MediaViewer->PlayListUpdated();
}

// -------------------------------------------------------------------------------- //
