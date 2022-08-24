/**
@file	 ComDialog.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>

#include <sigc++/sigc++.h>

#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QEvent>
#include <QApplication>
#include <QMessageBox>
#include <QCheckBox>
#include <QSlider>
#include <QTextCodec>
#include <QRegExp>
#include <QPixmap>
#include <QKeyEvent>
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
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDecimator.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "MyMessageBox.h"
#include "Settings.h"
#include "ComDialog.h"
#include "multirate_filter_coeff.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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


ComDialog::ComDialog(Directory& dir, const QString& callsign,
		     const QString& remote_name)
  : callsign(callsign), con(0), dir(dir), accept_connection(false),
    audio_full_duplex(false), is_transmitting(false), ctrl_pressed(false),
    rem_audio_fifo(0), ptt_valve(0), tx_audio_splitter(0), vox(0), dns(0)
{
  setupUi(this);

  mic_audio_io =
      new AudioIO(Settings::instance()->micAudioDevice().toStdString(), 0);
  spkr_audio_io =
      new AudioIO(Settings::instance()->spkrAudioDevice().toStdString(), 0);

  init(remote_name);

  const StationData *station = dir.findCall(callsign.toStdString());
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


ComDialog::ComDialog(Directory& dir, const QString& remote_host)
  : callsign(remote_host), con(0), dir(dir), accept_connection(false),
    audio_full_duplex(false), is_transmitting(false), ctrl_pressed(false),
    rem_audio_fifo(0), ptt_valve(0), tx_audio_splitter(0), vox(0), dns(0),
    chat_codec(0)
{
  setupUi(this);
  
  mic_audio_io =
      new AudioIO(Settings::instance()->micAudioDevice().toStdString(), 0);
  spkr_audio_io =
      new AudioIO(Settings::instance()->spkrAudioDevice().toStdString(), 0);
  
  init();
  dns = new DnsLookup(remote_host.toStdString());
  dns->resultsReady.connect(mem_fun(*this, &ComDialog::dnsResultsReady));
} /* ComDialog::ComDialog */


ComDialog::~ComDialog(void)
{
  delete con;
  if (vox != 0)
  {
    Settings::instance()->setVoxParams(vox->enabled(), vox->threshold(),
      	      	      	      	       vox->delay());
    delete vox;
  }
  delete dns;
  delete ptt_valve;
  delete tx_audio_splitter;
  delete rem_audio_valve;
  delete rem_audio_fifo;
  delete mic_audio_io;
  delete spkr_audio_io;
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
    con->setRemoteParams(priv.toStdString());
  }
} /* ComDialog::setRemoteParams */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void ComDialog::keyPressEvent(QKeyEvent *ke)
{
  if (!ctrl_pressed && !ke->text().isEmpty() &&
      (ke->key() != Qt::Key_Space) && (ke->key() != Qt::Key_Tab) &&
      (ke->key() != Qt::Key_Escape) && chat_outgoing->isEnabled())
  {
    chat_outgoing->setFocus();
  }
  
  if (ke->key() == Qt::Key_Control)
  {
    ptt_button->setCheckable(true);
    ctrl_pressed = true;
  }

  ke->ignore();
  QDialog::keyPressEvent(ke);
} /* ComDialog::keyPressEvent */


void ComDialog::keyReleaseEvent(QKeyEvent *ke)
{
  if (ke->key() == Qt::Key_Control)
  {
    if (!is_transmitting)
    {
      ptt_button->setCheckable(false);
    }
    ctrl_pressed = false;
  }
  ke->ignore();
  QDialog::keyReleaseEvent(ke);
} /* ComDialog::keyReleaseEvent */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void ComDialog::init(const QString& remote_name)
{
  chat_codec = QTextCodec::codecForName(Settings::instance()->chatEncoding().toUtf8());
  
  if (callsign.contains(QRegExp("-L$")))
  {
    setWindowIcon(QPixmap(":/icons/images/link.xpm"));
  }
  else if (callsign.contains(QRegExp("-R$")))
  {
    setWindowIcon(QPixmap(":/icons/images/repeater.xpm"));
  }
  else if (callsign.indexOf("*") == 0)
  {
    setWindowIcon(QPixmap(":/icons/images/conference.xpm"));
  }
  else
  {
    setWindowIcon(QPixmap(":/icons/images/online_icon.xpm"));
  }
  
  setAttribute(Qt::WA_DeleteOnClose);
  
  setWindowTitle(QString("EchoLink QSO: ") + callsign);
  call->setText(callsign);
  name_label->setText(remote_name);
  
  AudioSource *prev_src = 0;
  rem_audio_fifo = new AudioFifo(INTERNAL_SAMPLE_RATE);
  rem_audio_fifo->setOverwrite(true);
  rem_audio_fifo->setPrebufSamples(1280 * INTERNAL_SAMPLE_RATE / 8000);
  prev_src = rem_audio_fifo;
  
  rem_audio_valve = new AudioValve;
  rem_audio_valve->setOpen(false);
  prev_src->registerSink(rem_audio_valve);
  prev_src = rem_audio_valve;

#if (INTERNAL_SAMPLE_RATE == 8000)  
  if (spkr_audio_io->sampleRate() > 8000)
#endif
  {
      // Interpolate sample rate to 16kHz
    AudioInterpolator *i1 = new AudioInterpolator(2, coeff_16_8,
                                                  coeff_16_8_taps);
    prev_src->registerSink(i1, true);
    prev_src = i1;
  }

  if (spkr_audio_io->sampleRate() > 16000)
  {
      // Interpolate sample rate to 48kHz
#if (INTERNAL_SAMPLE_RATE == 8000)
    AudioInterpolator *i2 = new AudioInterpolator(3, coeff_48_16_int,
                                                  coeff_48_16_int_taps);
#else
    AudioInterpolator *i2 = new AudioInterpolator(3, coeff_48_16,
                                                  coeff_48_16_taps);
#endif
    prev_src->registerSink(i2, true);
    prev_src = i2;
  }
  
  prev_src->registerSink(spkr_audio_io);
  prev_src = 0;

    // Mic audio audio pipe starts here
  prev_src = mic_audio_io;

    // We need a buffer before the Decimators
  AudioFifo *mic_fifo = new AudioFifo(2048);
  prev_src->registerSink(mic_fifo, true);
  prev_src = mic_fifo;

    // If the sound card sample rate is higher than 16kHz (48kHz assumed),
    // decimate it down to 16kHz
  if (mic_audio_io->sampleRate() > 16000)
  {
    AudioDecimator *d1 = new AudioDecimator(3, coeff_48_16_wide,
					    coeff_48_16_wide_taps);
    prev_src->registerSink(d1, true);
    prev_src = d1;
  }

#if (INTERNAL_SAMPLE_RATE < 16000)
    // If the sound card sample rate is higher than 8kHz (16 or 48kHz assumed)
    // decimate it down to 8kHz.
  if (mic_audio_io->sampleRate() > 8000)
  {
    AudioDecimator *d2 = new AudioDecimator(2, coeff_16_8, coeff_16_8_taps);
    prev_src->registerSink(d2, true);
    prev_src = d2;
  }
#endif

  tx_audio_splitter = new AudioSplitter;
  prev_src->registerSink(tx_audio_splitter);
  prev_src = 0;
  
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
  
#if INTERNAL_SAMPLE_RATE == 16000
  AudioDecimator *down_sampler = new AudioDecimator(
          2, coeff_16_8, coeff_16_8_taps);
  tx_audio_splitter->addSink(down_sampler, true);
  ptt_valve = new AudioValve;
  down_sampler->registerSink(ptt_valve);
#else
  ptt_valve = new AudioValve;
  tx_audio_splitter->addSink(ptt_valve);
#endif
  
  if (settings->useFullDuplex())
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
      reinterpret_cast<QObject *>(ptt_button), SIGNAL(toggled(bool)),
      this, SLOT(pttButtonToggleStateChanged(bool)));
  QObject::connect(
      reinterpret_cast<QObject *>(chat_outgoing), SIGNAL(returnPressed()),
      this, SLOT(sendChatMsg()));
  
  ptt_button->installEventFilter(this);
  
  dir.stationListUpdated.connect(mem_fun
    (*this, &ComDialog::onStationListUpdated));
  
  orig_background_color = rx_indicator->palette().color(QPalette::Window);
} /* ComDialog::init */


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
  AudioSource *prev_src = ptt_valve;

  Settings *settings = Settings::instance();  
  con = new Qso(station->ip(), settings->callsign().toStdString(),
      settings->name().toStdString(), settings->info().toStdString());
  if (!con->initOk())
  {
    MyMessageBox *mb = new MyMessageBox(tr("Qtel Error"),
	tr("Could not create connection to") + " " + callsign);
    mb->show();
    connect(mb, SIGNAL(closed()), this, SLOT(close()));
    delete con;
    con = 0;
    return;
  }
  con->infoMsgReceived.connect(mem_fun(*this, &ComDialog::infoMsgReceived));
  con->chatMsgReceived.connect(mem_fun(*this, &ComDialog::chatMsgReceived));
  con->stateChange.connect(mem_fun(*this, &ComDialog::stateChange));
  con->isReceiving.connect(mem_fun(*this, &ComDialog::isReceiving));
  prev_src->registerSink(con);
  prev_src = con;
  
  prev_src->registerSink(rem_audio_fifo);
  
  connect_button->setEnabled(true);
  connect_button->setFocus();
  
  if (accept_connection)
  {
    con->accept();
  }
  
  setIsTransmitting(false);
  
} /* ComDialog::createConnection */


void ComDialog::onStationListUpdated(void)
{
  const StationData *station = dir.findCall(callsign.toStdString());
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
      MyMessageBox *mb = new MyMessageBox(tr("Qtel Error"),
	  tr("Station data not found in directory server.\nCan't create "
	  "connection to") + " " + callsign);
      mb->show();
      connect(mb, SIGNAL(closed()), this, SLOT(close()));
    }
  }
} /* ComDialog::onStationListUpdated */


bool ComDialog::openAudioDevice(AudioIO::Mode mode)
{
  bool mic_open_ok = true;
  bool spkr_open_ok = true;
  
  if ((mode == AudioIO::MODE_RD) || (mode == AudioIO::MODE_RDWR))
  {
    mic_open_ok = mic_audio_io->open(AudioIO::MODE_RD);
    if (!mic_open_ok)
    {
      MyMessageBox *mb = new MyMessageBox(tr("Qtel Error"),
	  tr("Could not open mic audio device"));
      mb->show();
      connect(mb, SIGNAL(closed()), this, SLOT(close()));
    }
  }
  
  if ((mode == AudioIO::MODE_WR) || (mode == AudioIO::MODE_RDWR))
  {
    spkr_open_ok = spkr_audio_io->open(AudioIO::MODE_WR);
    if (!spkr_open_ok)
    {
      MyMessageBox *mb = new MyMessageBox(tr("Qtel Error"),
	  tr("Could not open speaker audio device"));
      mb->show();
      connect(mb, SIGNAL(closed()), this, SLOT(close()));
    }
  }
  
  return mic_open_ok && spkr_open_ok;
  
} /* ComDialog::openAudioDevice */


void ComDialog::dnsResultsReady(DnsLookup &)
{
  if (dns->addresses().empty())
  {
    MyMessageBox *mb = new MyMessageBox(tr("Qtel Error"),
	tr("Could not create connection to remote host") + " \"" +
	dns->label().c_str() + "\"");
    mb->show();
    connect(mb, SIGNAL(closed()), this, SLOT(close()));
    delete con;
    con = 0;
    return;
  }
  
  StationData station;
  station.setIp(dns->addresses()[0]);
  updateStationData(&station);
  createConnection(&station);  
  
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
      mic_audio_io->close();
      spkr_audio_io->close();
      if (!openAudioDevice(AudioIO::MODE_RD))
      {
	disconnect();
	return;
      }
    }
    is_transmitting = true;
    QPalette palette = tx_indicator->palette();
    palette.setColor(QPalette::Window, Qt::red);
    tx_indicator->setPalette(palette);
    ptt_valve->setOpen(true);
  }
  else
  {
    ptt_valve->setOpen(false);
    if (!audio_full_duplex)
    {
      mic_audio_io->close();
      spkr_audio_io->close();
      if (!openAudioDevice(AudioIO::MODE_WR))
      {
	disconnect();
	return;
      }
      rem_audio_valve->setOpen(true);
    }
    is_transmitting = false;
    QPalette palette = tx_indicator->palette();
    palette.setColor(QPalette::Window, orig_background_color);
    tx_indicator->setPalette(palette);
    
    if (!ctrl_pressed)
    {
      ptt_button->setCheckable(false);
    }
  }
} /* ComDialog::setIsTransmitting */


void ComDialog::pttButtonPressedReleased(void)
{
  ptt_button->setFocus();
  
    /* Ignore press and release events if toggle mode is enabled */
  if (ptt_button->isCheckable())
  {
    return;
  }
  
  checkTransmit();
  
} /* ComDialog::pttButtonPressedReleased */


void ComDialog::pttButtonToggleStateChanged(bool checked)
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
  info_incoming->append("------------ " + tr("INFO") + " ------------");
  info_incoming->append(msg.c_str());
  info_incoming->append("------------------------------");
} /* ComDialog::infoMsgReceived */


void ComDialog::chatMsgReceived(const string& msg)
{
  if(isChatText(msg.c_str()))
  {
    string txt(msg.substr(0, msg.size()-1));
    chat_incoming->append(chat_codec->toUnicode(txt.c_str()));
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
      info_incoming->append(tr("Connected to ") + call->text() + "\n");
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
      info_incoming->append(tr("Connecting to ") + call->text() + "...\n");
      break;
      
    case Qso::STATE_BYE_RECEIVED:
      ctrl_pressed = false;
      ptt_button->setChecked(false);
      connect_button->setEnabled(false);
      disconnect_button->setEnabled(false);
      ptt_button->setEnabled(false);
      chat_outgoing->setEnabled(false);
      break;
      
    case Qso::STATE_DISCONNECTED:
      ctrl_pressed = false;
      ptt_button->setChecked(false);
      connect_button->setEnabled(true);
      disconnect_button->setEnabled(false);
      ptt_button->setEnabled(false);
      chat_outgoing->setEnabled(false);
      connect_button->setFocus();
      info_incoming->append(tr("Disconnected") + "\n");
      break;
  }
  
  checkTransmit();
  
} /* ComDialog::stateChange */


void ComDialog::isReceiving(bool is_receiving)
{
  QPalette palette = rx_indicator->palette();
  if (is_receiving)
  {
    palette.setColor(QPalette::Window, QColor(Qt::green));
  }
  else
  {
    palette.setColor(QPalette::Window, orig_background_color);
  }
  rx_indicator->setPalette(palette);
} /* ComDialog::isReceiving */


void ComDialog::meterLevelChanged(int level_db)
{
  vox_meter->setValue(vox_meter->maximum() + level_db);
} /* ComDialog::meterLevelChanged */


void ComDialog::voxStateChanged(Vox::State state)
{
  QPalette palette = vox_indicator->palette();
  switch (state)
  {
    case Vox::IDLE:
      palette.setColor(QPalette::Window, orig_background_color);
      break;
    case Vox::ACTIVE:
      palette.setColor(QPalette::Window, Qt::red);
      break;
    case Vox::HANG:
      palette.setColor(QPalette::Window, Qt::yellow);
      break;
  }
  vox_indicator->setPalette(palette);
  checkTransmit();
} /* ComDialog::voxStateChanged */


void ComDialog::checkTransmit(void)
{
  setIsTransmitting(
      	// We must be connected to transmit.
      ((con != 0) && (con->currentState() == Qso::STATE_CONNECTED)) &&
      
      	// Either the PTT or the VOX must be active to transmit
      ((ptt_button->isChecked()) || ptt_button->isDown() ||
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
  return msg.contains(rexp);
}



/*
 * This file has not been truncated
 */

