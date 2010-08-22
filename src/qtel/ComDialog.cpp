/**
@file	 ComDialog.cpp
@brief   Implements the station communication dialog.
@author  Tobias Blomberg
@date	 2003-03-09

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>

#include <qstring.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qtextbrowser.h>
#include <qevent.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qtextcodec.h>
#include <qregexp.h>
#undef emit


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <AsyncDnsLookup.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkQso.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioSplitter.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "MyMessageBox.h"
#include "images/link.xpm"
#include "images/repeater.xpm"
#include "images/conference.xpm"
#include "images/online_icon.xpm"
#include "Settings.h"
#include "ComDialog.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace SigC;
using namespace Async;
using namespace EchoLink;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ComDialog::ComDialog(AudioIO *audio_io, Directory& dir, const QString& callsign,
    const QString& remote_name)
  : callsign(callsign), con(0), dir(dir), accept_connection(false),
    audio_io(audio_io), audio_full_duplex(false), is_transmitting(false),
    ctrl_pressed(false), rem_audio_fifo(0), ptt_valve(0), tx_audio_splitter(0),
    vox(0), dns(0)
    
{
  init(remote_name);

  const StationData *station = dir.findCall(callsign.latin1());
  updateStationData(station);
  if (station != 0)
  {
    createConnection(station);
  }
  else
  {
    dir.getCalls();
  }
} /* ComDialog::ComDialog */


ComDialog::ComDialog(AudioIO *audio_io, Directory& dir,
      	      	     const QString& remote_host)
  : callsign(remote_host), con(0), dir(dir), accept_connection(false),
    audio_io(audio_io), audio_full_duplex(false), is_transmitting(false),
    ctrl_pressed(false), rem_audio_fifo(0), ptt_valve(0), tx_audio_splitter(0),
    vox(0), dns(0), chat_codec(0)
    
{
  init();
  dns = new DnsLookup(remote_host.latin1());
  dns->resultsReady.connect(slot(*this, &ComDialog::dnsResultsReady));
} /* ComDialog::ComDialog */


ComDialog::~ComDialog(void)
{
  Settings::instance()->setVoxParams(vox->enabled(), vox->threshold(),
      	      	      	      	     vox->delay());
				     
  delete vox;
  delete dns;
  delete ptt_valve;
  delete tx_audio_splitter;
  delete rem_audio_valve;
  delete rem_audio_fifo;
  audio_io->close();
  delete con;
} /* ComDialog::~ComDialog */


void ComDialog::acceptConnection(void)
{
  accept_connection = true;
  if (con != 0)
  {
    con->accept();
  }
} /* ComDialog::accept */


void ComDialog::setRemoteParams(const QString& priv)
{
  if (con != 0)
  {
    con->setRemoteParams(priv);
  }
} /* ComDialog::setRemoteParams */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void ComDialog::init(const QString& remote_name)
{
  chat_codec = QTextCodec::codecForName(Settings::instance()->chatEncoding());
  
  if (callsign.find("-L") != -1)
  {
    setIcon(QPixmap(const_cast<const char **>(link_xpm)));
  }
  else if (callsign.find("-R") != -1)
  {
    setIcon(QPixmap(const_cast<const char **>(repeater_xpm)));
  }
  else if (callsign.find("*") == 0)
  {
    setIcon(QPixmap(const_cast<const char **>(conference_xpm)));
  }
  else
  {
    setIcon(QPixmap(const_cast<const char **>(online_icon)));
  }
  
  setWFlags(getWFlags() | Qt::WDestructiveClose);
  
  setCaption(QString("EchoLink QSO: ") + callsign);
  call->setText(callsign);
  name_label->setText(remote_name);
  
  rem_audio_fifo = new AudioFifo(8000);
  rem_audio_fifo->setOverwrite(true);
  rem_audio_fifo->setPrebufSamples(1280);
  
  rem_audio_valve = new AudioValve;
  rem_audio_valve->setOpen(false);
  rem_audio_fifo->registerSink(rem_audio_valve);

  rem_audio_valve->registerSink(audio_io);

  tx_audio_splitter = new AudioSplitter;
  audio_io->registerSink(tx_audio_splitter);
  
  Settings *settings = Settings::instance();  

  vox = new Vox;
  vox->setEnabled(settings->voxEnabled());
  vox->setThreshold(settings->voxThreshold());
  vox->setDelay(settings->voxDelay());
  connect(vox, SIGNAL(levelChanged(int)),
      	  this, SLOT(meterLevelChanged(int)));
  connect(vox, SIGNAL(stateChanged(Vox::State)),
      	  this, SLOT(voxStateChanged(Vox::State)));
  connect(vox_enabled_checkbox, SIGNAL(toggled(bool)),
      	  vox, SLOT(setEnabled(bool)));
  connect(vox_threshold, SIGNAL(sliderMoved(int)),
      	  vox, SLOT(setThreshold(int)));
  connect(vox_delay, SIGNAL(valueChanged(int)),
      	  vox, SLOT(setDelay(int)));
  tx_audio_splitter->addSink(vox);

  vox_enabled_checkbox->setChecked(vox->enabled());
  vox_threshold->setValue(vox->threshold());
  vox_delay->setValue(vox->delay());
     
  ptt_valve = new AudioValve;
  tx_audio_splitter->addSink(ptt_valve);
  
  if (settings->useFullDuplex() && audio_io->isFullDuplexCapable())
  {
    audio_full_duplex = true;
    if (!openAudioDevice(AudioIO::MODE_RDWR))
    {
      return;
    }
    rem_audio_valve->setOpen(true);
    
    vox_enabled_checkbox->setEnabled(true);
  }
  else
  {
    vox_enabled_checkbox->setEnabled(false);  
  }
    
  QObject::connect(
      reinterpret_cast<QObject *>(connect_button), SIGNAL(clicked()),
      this, SLOT(connectToStation()));
  QObject::connect(
      reinterpret_cast<QObject *>(disconnect_button), SIGNAL(clicked()),
      this, SLOT(disconnectFromStation()));
  QObject::connect(
      reinterpret_cast<QObject *>(ptt_button), SIGNAL(pressed()),
      this, SLOT(pttButtonPressedReleased()));
  QObject::connect(
      reinterpret_cast<QObject *>(ptt_button), SIGNAL(released()),
      this, SLOT(pttButtonPressedReleased()));
  QObject::connect(
      reinterpret_cast<QObject *>(ptt_button), SIGNAL(stateChanged(int)),
      this, SLOT(pttButtonToggleStateChanged(int)));
  QObject::connect(
      reinterpret_cast<QObject *>(chat_outgoing), SIGNAL(returnPressed()),
      this, SLOT(sendChatMsg()));
  
  ptt_button->installEventFilter(this);
  
  dir.stationListUpdated.connect(slot(*this, &ComDialog::onStationListUpdated));
  
  orig_background_color = rx_indicator->paletteBackgroundColor(); 
  
} /* ComDialog::init */


bool ComDialog::eventFilter(QObject *watched, QEvent *e)
{
  if (e->type() == QEvent::KeyPress )
  {
    QKeyEvent * ke = (QKeyEvent*)e;
    if (!ke->text().isEmpty() && (ke->key() != Key_Space) &&
	(ke->key() != Key_Tab) && (ke->key() != Key_Escape))
    {
      chat_outgoing->setFocus();
      chat_outgoing->insert(ke->text());
      return TRUE;
    }
    
    if (ke->key() == Key_Control)
    {
      ptt_button->setToggleButton(true);
      ctrl_pressed = true;
    }
  } 
  else if (e->type() == QEvent::KeyRelease)
  {
    QKeyEvent * ke = (QKeyEvent*)e;
    if (ke->key() == Key_Control)
    {
      if (!is_transmitting)
      {
      	ptt_button->setToggleButton(false);
      }
      ctrl_pressed = false;
    }
  }
  
  return FALSE;
}


void ComDialog::updateStationData(const StationData *station)
{
  if (station != 0)
  {
    description->setText(station->description().c_str());
    status->setText(station->statusStr().c_str());
    ip_address->setText(station->ipStr().c_str());
    time->setText(station->time().c_str());
  }
  else
  {
    description->setText("?");
    status->setText("?");
    ip_address->setText("?");
    time->setText("?");  
  }
} /* ComDialog::updateStationData */


void ComDialog::createConnection(const StationData *station)
{
  Settings *settings = Settings::instance();  
  con = new Qso(station->ip(), settings->callsign().latin1(),
      settings->name().latin1(), settings->info().latin1());
  if (!con->initOk())
  {
    MyMessageBox *mb = new MyMessageBox(trUtf8("Qtel Error"),
	trUtf8("Could not create connection to station") + " " + callsign);
    mb->show();
    connect(mb, SIGNAL(closed()), this, SLOT(close()));
    delete con;
    con = 0;
    return;
  }
  
  con->infoMsgReceived.connect(slot(*this, &ComDialog::infoMsgReceived));
  con->chatMsgReceived.connect(slot(*this, &ComDialog::chatMsgReceived));
  con->stateChange.connect(slot(*this, &ComDialog::stateChange));
  con->isReceiving.connect(slot(*this, &ComDialog::isReceiving));
  con->registerSink(rem_audio_fifo);
  ptt_valve->registerSink(con);
  
  connect_button->setEnabled(TRUE);
  connect_button->setFocus();
  
  if (accept_connection)
  {
    con->accept();
  }
  
  setIsTransmitting(false);
  
} /* ComDialog::createConnection */


void ComDialog::onStationListUpdated(void)
{
  const StationData *station = dir.findCall(callsign.latin1());
  //station = 0;
  updateStationData(station);
  if (con == 0)
  {
    if (station != 0)
    {
      createConnection(station);
    }
    else
    {
      MyMessageBox *mb = new MyMessageBox(trUtf8("Qtel Error"),
	  trUtf8("Station data not found in directory server.\nCan't create "
	  "connection to") + " " + callsign);
      mb->show();
      connect(mb, SIGNAL(closed()), this, SLOT(close()));
    }
  }
} /* ComDialog::onStationListUpdated */


bool ComDialog::openAudioDevice(AudioIO::Mode mode)
{
  bool open_ok = audio_io->open(mode);
  if (!open_ok)
  {
    MyMessageBox *mb = new MyMessageBox(trUtf8("Qtel Error"),
	trUtf8("Could not open audio device"));
    mb->show();
    connect(mb, SIGNAL(closed()), this, SLOT(close()));
  }
  
  return open_ok;
  
} /* ComDialog::openAudioDevice */


void ComDialog::dnsResultsReady(DnsLookup &)
{
  if (dns->addresses().empty())
  {
    MyMessageBox *mb = new MyMessageBox(trUtf8("Qtel Error"),
	trUtf8("Could not create connection to remote host") + " \"" +
	dns->label().c_str() + "\"");
    mb->show();
    connect(mb, SIGNAL(closed()), this, SLOT(close()));
    delete con;
    con = 0;
    return;
  }
  
  StationData *station = new StationData;
  station->setIp(dns->addresses()[0]);
  updateStationData(station);
  createConnection(station);  
  
  delete dns;
  dns = 0;
  
} /* ComDialog::dnsResultsReady */


void ComDialog::connectToStation()
{
  con->connect();
} /* ComDialog::connectToStation */


void ComDialog::disconnectFromStation()
{
  con->disconnect();
} /* ComDialog::disconnectFromStation */


void ComDialog::setIsTransmitting(bool transmit)
{
  if (transmit)
  {
    if (!audio_full_duplex)
    {
      rem_audio_valve->setOpen(false);
      audio_io->close();
      if (!openAudioDevice(AudioIO::MODE_RD))
      {
	disconnect();
	return;
      }
    }
    is_transmitting = true;
    tx_indicator->setPaletteBackgroundColor("red");
    ptt_valve->setOpen(true);
  }
  else
  {
    ptt_valve->setOpen(false);
    if (!audio_full_duplex)
    {
      audio_io->close();
      if (!openAudioDevice(AudioIO::MODE_WR))
      {
	disconnect();
	return;
      }
      rem_audio_valve->setOpen(true);
    }
    is_transmitting = false;
    tx_indicator->setPaletteBackgroundColor(orig_background_color);
    
    if (!ctrl_pressed)
    {
      ptt_button->setToggleButton(false);
    }
  }
} /* ComDialog::setIsTransmitting */


void ComDialog::pttButtonPressedReleased(void)
{
  ptt_button->setFocus();
  
    /* Ignore press and release events if toggle mode is enabled */
  if (ptt_button->isToggleButton())
  {
    return;
  }
  
  checkTransmit();
  
} /* ComDialog::pttButtonPressedReleased */


void ComDialog::pttButtonToggleStateChanged(int state)
{
  checkTransmit();
} /* ComDialog::pttButtonToggleStateChanged */


void ComDialog::sendChatMsg()
{
  chat_incoming->append(Settings::instance()->callsign() + "> " +
      chat_outgoing->text());
  con->sendChatData(chat_codec->fromUnicode(chat_outgoing->text()).data());
  chat_outgoing->clear();
  ptt_button->setFocus();
} /* ComDialog::sendChatMsg */


void ComDialog::infoMsgReceived(const string& msg)
{
  info_incoming->append("------------ " + trUtf8("INFO") + " ------------");
  info_incoming->append(msg.c_str());
  info_incoming->append("------------------------------");
} /* ComDialog::infoMsgReceived */


void ComDialog::chatMsgReceived(const string& msg)
{
  if(isChatText(msg.c_str()))
  {
    QString txt(msg);
    txt.truncate(msg.length() - 1); // Remove NL
    chat_incoming->append(chat_codec->toUnicode(txt));
  }
  else
  {
    info_incoming->append(msg.c_str());
  }
} /* ComDialog::chatMsgReceived */


void ComDialog::stateChange(Qso::State state)
{
  switch (state)
  {
    case Qso::STATE_CONNECTED:
      connect_button->setEnabled(false);
      disconnect_button->setEnabled(true);
      ptt_button->setEnabled(true);
      chat_outgoing->setEnabled(true);
      ptt_button->setFocus();
      info_incoming->append(trUtf8("Connected to ") + call->text() + "\n");
      if (name_label->text() == "?")
      {
	name_label->setText(con->remoteName().c_str());
      }
      break;
    
    case Qso::STATE_CONNECTING:
      connect_button->setEnabled(false);
      disconnect_button->setEnabled(true);
      ptt_button->setEnabled(false);
      chat_outgoing->setEnabled(false);
      disconnect_button->setFocus();
      info_incoming->append(trUtf8("Connecting to ") + call->text() + "...\n");
      break;
      
    case Qso::STATE_BYE_RECEIVED:
      ctrl_pressed = false;
      ptt_button->setOn(false);
      connect_button->setEnabled(false);
      disconnect_button->setEnabled(false);
      ptt_button->setEnabled(false);
      chat_outgoing->setEnabled(false);
      break;
      
    case Qso::STATE_DISCONNECTED:
      ctrl_pressed = false;
      ptt_button->setOn(false);
      connect_button->setEnabled(true);
      disconnect_button->setEnabled(false);
      ptt_button->setEnabled(false);
      chat_outgoing->setEnabled(false);
      connect_button->setFocus();
      info_incoming->append(trUtf8("Disconnected") + "\n");
      break;
  }
  
  checkTransmit();
  
} /* ComDialog::stateChange */


void ComDialog::isReceiving(bool is_receiving)
{
  rx_indicator->setPaletteBackgroundColor(
      is_receiving ? QColor("green") : orig_background_color);
} /* ComDialog::isReceiving */


void ComDialog::meterLevelChanged(int level_db)
{
  vox_meter->setProgress(vox_meter->totalSteps() + level_db);
} /* ComDialog::meterLevelChanged */


void ComDialog::voxStateChanged(Vox::State state)
{
  switch (state)
  {
    case Vox::IDLE:
      vox_indicator->setPaletteBackgroundColor(orig_background_color);
      break;
    case Vox::ACTIVE:
      vox_indicator->setPaletteBackgroundColor("red");
      break;
    case Vox::HANG:
      vox_indicator->setPaletteBackgroundColor("yellow");
      break;
  }
  checkTransmit();
} /* ComDialog::voxStateChanged */


void ComDialog::checkTransmit(void)
{
  setIsTransmitting(
      	// We must be connected to transmit.
      ((con != 0) && (con->currentState() == Qso::STATE_CONNECTED)) &&
      
      	// Either the PTT or the VOX must be active to transmit
      ((ptt_button->isOn()) || ptt_button->isDown() ||
       (vox->state() != Vox::IDLE))
  );
} /* ComDialog::checkTransmit */


bool ComDialog::isChatText(const QString& msg)
{
    // Will match callsigns with a mix of letters and digits with a minimum of
    // three characters and a maximum of seven characters, excluding the
    // optional "-L" or "-R". To be classified as a chat message a ">" must
    // directly follow the callsign.
  QRegExp rexp("^[A-Z0-9]{3,7}(?:-[RL])?>");
  return (rexp.search(msg) == 0);
  
  #if 0
  string::const_iterator it;
  it=msg.begin();
  if(!(((*it >= 'A') && (*it <= 'Z')) || ((*it >= '0') && (*it <= '9')))) return FALSE;
  it++;
  if(!(((*it >= 'A') && (*it <= 'Z')) || ((*it >= '0') && (*it <= '9')))) return FALSE;
  it++;
  if(!(((*it >= 'A') && (*it <= 'Z')) || ((*it >= '0') && (*it <= '9')))) return FALSE;
  it++;
  if(((*it >= 'A') && (*it <= 'Z')) || ((*it >= '0') && (*it <= '9')))
  {
    it++;
    if(((*it >= 'A') && (*it <= 'Z')) || ((*it >= '0') && (*it <= '9')))
    {
      it++;
      if((*it >= 'A') && (*it <= 'Z'))
      {
        it++;
      }
    }
  }
  if(*it == '>') return TRUE;
  if(*it != '-') return FALSE;
  it++;
  if((*it != 'L') && (*it != 'R')) return FALSE;
  it++;
  if(*it == '>') return TRUE;
  return FALSE;
  #endif
}


/*
 * This file has not been truncated
 */

