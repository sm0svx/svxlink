/**
@file	 ComDialog.h
@brief   Implements the station communication dialog.
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


#ifndef COMDIALOG_INCLUDED
#define COMDIALOG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkQso.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ComDialogBase.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class QString;


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
 * Class:     ComDialog
 * Purpose:   ComDialog class
 * Inherits:  
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */   
class ComDialog : public ComDialogBase, public SigC::Object
{
  Q_OBJECT
      
  public:
    /*
     *------------------------------------------------------------------------
     * Method:	ComDialog
     * Purpose: Constructor
     * Input: 	
     * Output:	None
     * Author:	
     * Created: 
     * Remarks: 
     * Bugs:  	
     *------------------------------------------------------------------------
     */
    ComDialog(Async::AudioIO *audio_io, EchoLink::Directory& dir,
      	      const QString& call, const QString& remote_name="?");
    ~ComDialog(void);
    
    void acceptConnection(void);
    
  protected:
    
  private:
    QString   	      	  callsign;
    EchoLink::Qso *   	  con;
    EchoLink::Directory & dir;
    bool      	      	  accept_connection;
    Async::AudioIO * 	  audio_io;
    bool      	      	  audio_full_duplex;
    bool      	      	  is_transmitting;
    QColor    	      	  orig_background_color;
    bool      	      	  ctrl_pressed;
  
    ComDialog(const ComDialog&);
    ComDialog& operator=(const ComDialog&);
    bool eventFilter(QObject *watched, QEvent *e);
    void updateStationData(const EchoLink::StationData *station);
    void createConnection(const EchoLink::StationData *station);
    void onStationListUpdated(void);
    void audioReceived(short *buf, int len);
    void micAudioRead(short *buf, int len);
    bool openAudioDevice(Async::AudioIO::Mode mode);

  private slots:
    void connectToStation();
    void disconnectFromStation();
    void setIsTransmitting(bool transmit);
    void pttButtonPressedReleased();
    void pttButtonToggleStateChanged(int state);
    void sendChatMsg();
    void infoMsgReceived(const std::string& msg);
    void chatMsgReceived(const std::string& msg);
    void stateChange(EchoLink::Qso::State state);
    void isReceiving(bool is_receiving);

    
};  /* class ComDialog */


//} /* namespace */

#endif /* COMDIALOG_INCLUDED */



/*
 * This file has not been truncated
 */

