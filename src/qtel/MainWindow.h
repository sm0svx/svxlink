/**
@file	 MainWindow.h
@brief   Implementation class for the main window
@author  Tobias Blomberg
@date	 2003-03-09

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003  Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



#ifndef MAINWINDOW_INCLUDED
#define MAINWINDOW_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>

#include <qlabel.h>
#include <qmap.h>
#include <qpopupmenu.h>
#undef emit


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <EchoLinkDirectory.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "MainWindowBase.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class IncomingConnection;
class QPopupMenu;
class MsgHandler;


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Macro:   
 * Purpose: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 * Bugs:    
 *----------------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------------
 * Type:    
 * Purpose: 
 * Members: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 *----------------------------------------------------------------------------
 */


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Class:     MainWindow
 * Purpose:   MainWindow class
 * Inherits:  
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */   
class MainWindow : public MainWindowBase, public SigC::Object
{
  Q_OBJECT
  
  public:
    /*
     *------------------------------------------------------------------------
     * Method:	MainWindow
     * Purpose: Constructor
     * Input: 	
     * Output:	None
     * Author:	
     * Created: 
     * Remarks: 
     * Bugs:  	
     *------------------------------------------------------------------------
     */
    MainWindow(EchoLink::Directory& dir);
    virtual ~MainWindow(void);
    
    
  protected:
    
  private:    
    typedef QMap<QListViewItem *, QString> SelectMap;
  
    EchoLink::Directory &     	  dir;
    QTimer *  	      	      	  refresh_call_list_timer;
    bool      	      	      	  is_busy;
    QLabel *  	      	      	  status_indicator;
    std::string       	      	  old_server_msg;
    QPopupMenu *      	      	  station_view_popup;
    int       	      	      	  station_view_popup_add;
    int       	      	      	  station_view_popup_add_named;
    int       	      	      	  station_view_popup_remove;
    Async::AudioIO 	      	  *audio_io;
    SelectMap 	      	      	  select_map;
    MsgHandler		      	  *msg_handler;
    Async::AudioIO 	      	  *msg_audio_io;
    EchoLink::StationData::Status prev_status;
    
    std::map<std::string, std::string> incoming_con_param;
    
    MainWindow(const MainWindow&);
    MainWindow operator=(const MainWindow&);
    
    void incomingConnection(const Async::IpAddress& remote_address,
      	const std::string& remote_call, const std::string& remote_name,
      	const std::string& remote_priv);
    void closeEvent(QCloseEvent *e);
    void serverError(const std::string& msg);
    void statusChanged(EchoLink::StationData::Status status);
    void allMsgsWritten(void);
    void allSamplesFlushed(void);
    
  private slots:
    void initialize(void);
    void explorerViewClicked(QListViewItem* item);
    void stationViewDoubleClicked(QListViewItem* item);
    void stationViewSelectionChanged(void);
    void callsignListUpdated(void);
    void refreshCallList(void);
    void updateRegistration(void);
    void setBusy(bool busy);
    void forceQuit(void);
    void incomingSelectionChanged(QListViewItem *item);
    void clearIncomingList(void);
    void acceptIncoming(void);
    void stationViewRightClicked(QListViewItem *item,
	const QPoint &point, int column);
    void addSelectedToBookmarks(void);
    void removeSelectedFromBookmarks(void);
    void addNamedStationToBookmarks(void);
    void configurationUpdated(void);
    void connectionConnectToIpActionActivated(void);
    void connectionConnectToSelectedActionActivated(void);
    
};  /* class MainWindow */


//} /* namespace */

#endif /* MAINWINDOW_INCLUDED */



/*
 * This file has not been truncated
 */

