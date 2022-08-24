/**
@file	 MainWindow.h
@brief   Implementation class for the main window
@author  Tobias Blomberg
@date	 2003-03-09

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <QCloseEvent>
#include <QModelIndex>
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

#include "ui_MainWindowBase.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class IncomingConnection;
class MsgHandler;
class EchoLinkDirectoryModel;

namespace EchoLink
{
  class Proxy;
};


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
class MainWindow : public QMainWindow, private Ui::MainWindowBase,
		   public sigc::trackable
{
  Q_OBJECT
  
  public:
    MainWindow(void);
    virtual ~MainWindow(void);
    
  protected:
    
  private:    
    EchoLink::Directory      	  *dir;
    QTimer *  	      	      	  refresh_call_list_timer;
    bool      	      	      	  is_busy;
    QLabel *  	      	      	  status_indicator;
    std::string       	      	  old_server_msg;
    MsgHandler		      	  *msg_handler;
    Async::AudioIO 	      	  *msg_audio_io;
    EchoLink::StationData::Status prev_status;
    EchoLinkDirectoryModel	  *bookmark_model;
    EchoLinkDirectoryModel	  *conf_model;
    EchoLinkDirectoryModel	  *link_model;
    EchoLinkDirectoryModel	  *repeater_model;
    EchoLinkDirectoryModel	  *station_model;
    EchoLink::Proxy               *proxy;
    
    QMap<QString, QString> incoming_con_param;
    
    MainWindow(const MainWindow&);
    MainWindow operator=(const MainWindow&);
    
    void incomingConnection(const Async::IpAddress& remote_address,
      	const std::string& remote_call, const std::string& remote_name,
      	const std::string& remote_priv);
    void closeEvent(QCloseEvent *e);
    void serverError(const std::string& msg);
    void statusChanged(EchoLink::StationData::Status status);
    void allMsgsWritten(void);
    void initMsgAudioIo(void);
    void setupAudioParams(void);
    void initEchoLink(void);
    void updateBookmarkModel(void);
    
  private slots:
    void stationViewSelectorCurrentItemChanged(QListWidgetItem *current,
					       QListWidgetItem *previous);
    void stationViewDoubleClicked(const QModelIndex &index);
    void stationViewSelectionChanged(const QItemSelection &current,
				     const QItemSelection &previous);
    void callsignListUpdated(void);
    void refreshCallList(void);
    void updateRegistration(void);
    void setBusy(bool busy);
    void forceQuit(void);
    void incomingSelectionChanged(void);
    void clearIncomingList(void);
    void acceptIncoming(void);
    void addSelectedToBookmarks(void);
    void removeSelectedFromBookmarks(void);
    void addNamedStationToBookmarks(void);
    void configurationUpdated(void);
    void connectionConnectToIpActionActivated(void);
    void connectionConnectToSelectedActionActivated(void);
    void settings(void);
    void helpAbout(void);

};  /* class MainWindow */


//} /* namespace */

#endif /* MAINWINDOW_INCLUDED */



/*
 * This file has not been truncated
 */

