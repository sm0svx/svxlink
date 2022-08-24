/**
@file	 ComDialog.h
@brief   Implements the station communication dialog.
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

#ifndef COMDIALOG_INCLUDED
#define COMDIALOG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <string>
#include <QEvent>


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

#include "ui_ComDialogBase.h"
#include "Vox.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class DnsLookup;
  class AudioFifo;
  class AudioValve;
  class AudioSplitter;
};

class QString;
class QTextCodec;


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

class ComDialog : public QDialog, private Ui::ComDialogBase,
		  public sigc::trackable
{
  Q_OBJECT
      
  public:
    ComDialog(EchoLink::Directory& dir, const QString& call,
	      const QString& remote_name);
    ComDialog(EchoLink::Directory& dir, const QString& remote_host);
    virtual ~ComDialog(void);
    
    void acceptConnection(void);
    void setRemoteParams(const QString& priv);
    
  protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    
  private:
    QString   	      	    callsign;
    EchoLink::Qso    	    *con;
    EchoLink::Directory     &dir;
    bool      	      	    accept_connection;
    Async::AudioIO  	    *mic_audio_io;
    Async::AudioIO  	    *spkr_audio_io;
    bool      	      	    audio_full_duplex;
    bool      	      	    is_transmitting;
    QColor    	      	    orig_background_color;
    bool      	      	    ctrl_pressed;
    Async::AudioFifo        *rem_audio_fifo;
    Async::AudioValve       *rem_audio_valve;
    Async::AudioValve       *ptt_valve;
    Async::AudioSplitter    *tx_audio_splitter;
    Vox      	      	    *vox;
    Async::DnsLookup 	    *dns;
    QTextCodec		    *chat_codec;
  
    ComDialog(const ComDialog&);
    ComDialog& operator=(const ComDialog&);
    void init(const QString& remote_name="?");
    void updateStationData(const EchoLink::StationData *station);
    void createConnection(const EchoLink::StationData *station);
    void onStationListUpdated(void);
    bool openAudioDevice(Async::AudioIO::Mode mode);
    void dnsResultsReady(Async::DnsLookup &);
    bool isChatText(const QString& msg);

  private slots:
    void connectToStation();
    void disconnectFromStation();
    void setIsTransmitting(bool transmit);
    void pttButtonPressedReleased();
    void pttButtonToggleStateChanged(bool checked);
    void sendChatMsg();
    void infoMsgReceived(const std::string& msg);
    void chatMsgReceived(const std::string& msg);
    void stateChange(EchoLink::Qso::State state);
    
    void isReceiving(bool is_receiving);
    void meterLevelChanged(int level_db);
    void voxStateChanged(Vox::State state);
    void checkTransmit(void);

    
};  /* class ComDialog */


//} /* namespace */

#endif /* COMDIALOG_INCLUDED */



/*
 * This file has not been truncated
 */

