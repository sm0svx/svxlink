/**
@file	 Settings.cpp
@brief   Handle application settings.
@author  Tobias Blomberg
@date	 2003-03-30

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <sigc++/sigc++.h>

#include <QSettings>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QStringList>
#include <QComboBox>
#include <QList>
#include <QNetworkInterface>
#undef emit

#include <stdlib.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <config.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SettingsDialog.h"
#include "Settings.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define CONF_CALLSIGN 	      	      "Callsign"
#define CONF_PASSWORD 	      	      "Password"
#define CONF_NAME     	      	      "Name"
#define CONF_LOCATION 	      	      "Location"
#define CONF_INFO     	      	      "Info"

#define CONF_DIRECTORY_SERVER 	      "DirectoryServers"
#define CONF_LIST_REFRESH_TIME        "ListRefreshTime"
#define CONF_START_AS_BUSY            "StartAsBusy"
#define CONF_BIND_ADDRESS             "BindAddress"
#define CONF_PROXY_ENABLED            "ProxyEnabled"
#define CONF_PROXY_SERVER             "ProxyServer"
#define CONF_PROXY_PORT               "ProxyPort"
#define CONF_PROXY_PASSWORD           "ProxyPassword"

#define CONF_MIC_AUDIO_DEVICE         "MicAudioDevice"
#define CONF_SPKR_AUDIO_DEVICE        "SpkrAudioDevice"
#define CONF_USE_FULL_DUPLEX          "UseFullDuplex"
#define CONF_CONNECT_SOUND            "ConnectSound"
#define CONF_CARD_SAMPLE_RATE         "CardSampleRate"

#define CONF_CHAT_ENCODING	      "ChatEncoding"

#define CONF_BOOKMARKS 	      	      "Bookmarks"
#define CONF_MAIN_WINDOW_SIZE         "MainWindowSize"
#define CONF_VSPLITTER_SIZES  	      "VSplitterSizes"
#define CONF_HSPLITTER_SIZES  	      "HSplitterSizes"
#define CONF_STN_VIEW_COL_SIZES	      "StationViewColumnSizes"
#define CONF_INCM_VIEW_COL_SIZES      "IncomingViewColumnSizes"

#define CONF_VOX_ENABLED      	      "VoxEnabled"
#define CONF_VOX_THRESHOLD            "VoxThreshold"
#define CONF_VOX_DELAY      	      "VoxDelay"

#define CONF_CONNECT_TO_IP            "ConnectToIp"

#define CONF_INFO_DEFAULT     	      "Running Qtel, an EchoLink client for " \
      	      	      	      	      "Linux"

#define CONF_DIRECTORY_SERVER_DEFAULT 	"server1.echolink.org"
#define CONF_LIST_REFRESH_TIME_DEFAULT	5
#define CONF_START_AS_BUSY_DEFAULT    	false
#define CONF_PROXY_ENABLED_DEFAULT 	false
#define CONF_PROXY_SERVER_DEFAULT 	""
#define CONF_PROXY_PORT_DEFAULT 	8100
#define CONF_PROXY_PASSWORD_DEFAULT 	""

#define CONF_AUDIO_DEVICE_DEFAULT 	"alsa:default"
#define CONF_USE_FULL_DUPLEX_DEFAULT    false
#define CONF_CONNECT_SOUND_DEFAULT 	SHARE_INSTALL_PREFIX \
                                        "/qtel/sounds/connect.raw"
#define CONF_CARD_SAMPLE_RATE_DEFAULT   48000

#define CONF_CHAT_ENCODING_DEFAULT	"ISO8859-1"

#define CONF_VOX_ENABLED_DEFAULT      	false
#define CONF_VOX_THRESHOLD_DEFAULT    	-30
#define CONF_VOX_DELAY_DEFAULT      	1000



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

Settings *Settings::the_instance = 0;



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
Settings *Settings::instance(void)
{
  if (the_instance == 0)
  {
    the_instance = new Settings;
  }
  
  return the_instance;
  
} /* Settings::instance */


Settings::Settings(void)
  : dialog(0),
    m_list_refresh_time(CONF_LIST_REFRESH_TIME_DEFAULT),
    m_start_as_busy(CONF_START_AS_BUSY_DEFAULT),
    m_proxy_enabled(CONF_PROXY_ENABLED_DEFAULT),
    m_proxy_port(CONF_PROXY_PORT_DEFAULT),
    m_use_full_duplex(CONF_USE_FULL_DUPLEX_DEFAULT),
    m_card_sample_rate(CONF_CARD_SAMPLE_RATE_DEFAULT),
    m_chat_encoding(0),
    m_vox_enabled(CONF_VOX_ENABLED_DEFAULT),
    m_vox_threshold(CONF_VOX_THRESHOLD_DEFAULT),
    m_vox_delay(CONF_VOX_DELAY_DEFAULT)
{
    // The default encoding should be first in the list
  encodings.push_back(Encoding("ISO8859-1", SettingsDialog::tr("Western")));
  encodings.push_back(Encoding("ISO8859-2", SettingsDialog::tr("Central European")));
  encodings.push_back(Encoding("ISO8859-3", SettingsDialog::tr("Central European")));
  encodings.push_back(Encoding("ISO8859-4", SettingsDialog::tr("Baltic")));
  encodings.push_back(Encoding("ISO8859-5", SettingsDialog::tr("Cyrillic")));
  encodings.push_back(Encoding("ISO8859-6", SettingsDialog::tr("Arabic")));
  encodings.push_back(Encoding("ISO8859-7", SettingsDialog::tr("Greek")));
  encodings.push_back(Encoding("ISO8859-8",
			    SettingsDialog::tr("Hebrew, visually ordered")));
  encodings.push_back(Encoding("ISO8859-8-i",
			    SettingsDialog::tr("Hebrew, logically ordered")));
  encodings.push_back(Encoding("ISO8859-9", SettingsDialog::tr("Turkish")));
  encodings.push_back(Encoding("ISO8859-10"));
  encodings.push_back(Encoding("ISO8859-13"));
  encodings.push_back(Encoding("ISO8859-14"));
  encodings.push_back(Encoding("ISO8859-15", SettingsDialog::tr("Western")));
  encodings.push_back(Encoding("utf8", SettingsDialog::tr("Unicode, 8-bit")));
  encodings.push_back(Encoding("Big5", SettingsDialog::tr("Chinese")));
  encodings.push_back(Encoding("Big5-HKSCS", SettingsDialog::tr("Chinese")));
  encodings.push_back(Encoding("eucJP", SettingsDialog::tr("Japanese")));
  encodings.push_back(Encoding("eucKR", SettingsDialog::tr("Korean")));
  encodings.push_back(Encoding("GB2312", SettingsDialog::tr("Chinese")));
  encodings.push_back(Encoding("GBK", SettingsDialog::tr("Chinese")));
  encodings.push_back(Encoding("GB18030", SettingsDialog::tr("Chinese")));
  encodings.push_back(Encoding("JIS7", SettingsDialog::tr("Japanese")));
  encodings.push_back(Encoding("Shift-JIS", SettingsDialog::tr("Japanese")));
  encodings.push_back(Encoding("TSCII", SettingsDialog::tr("Tamil")));
  encodings.push_back(Encoding("KOI8-R", SettingsDialog::tr("Russian")));
  encodings.push_back(Encoding("KOI8-U", SettingsDialog::tr("Ukrainian")));
  encodings.push_back(Encoding("IBM850"));
  encodings.push_back(Encoding("IBM866"));
  encodings.push_back(Encoding("CP874"));
  encodings.push_back(Encoding("CP1250", SettingsDialog::tr("Central European")));
  encodings.push_back(Encoding("CP1251", SettingsDialog::tr("Cyrillic")));
  encodings.push_back(Encoding("CP1252", SettingsDialog::tr("Western")));
  encodings.push_back(Encoding("CP1253", SettingsDialog::tr("Greek")));
  encodings.push_back(Encoding("CP1254", SettingsDialog::tr("Turkish")));
  encodings.push_back(Encoding("CP1255", SettingsDialog::tr("Hebrew")));
  encodings.push_back(Encoding("CP1255", SettingsDialog::tr("Hebrew")));
  encodings.push_back(Encoding("CP1256", SettingsDialog::tr("Arabic")));
  encodings.push_back(Encoding("CP1257", SettingsDialog::tr("Baltic")));
  encodings.push_back(Encoding("CP1258"));
  encodings.push_back(Encoding("TIS-620", SettingsDialog::tr("Thai")));
} /* Settings::Settings */


Settings::~Settings(void)
{
  encodings.clear();
  the_instance = 0;
} /* Settings::~Settings */


void Settings::showDialog(void)
{
  if (dialog != 0)
  {
    return;
  }
  
  SettingsDialog settings_dialog;

  foreach (const Encoding &enc, encodings)
  {
    QString str(enc.name);
    if (!enc.language.isEmpty())
    {
      str += " -- " + enc.language;
    }
    settings_dialog.chat_encoding->insertItem(10000, str);
  }

    // Set up bind address combobox
  foreach (const QNetworkInterface &netif, QNetworkInterface::allInterfaces())
  {
    const QString &hname = netif.name();
    foreach (const QNetworkAddressEntry &netaddr, netif.addressEntries())
    {
      const QHostAddress &addr = netaddr.ip();
      if (addr.protocol() == QAbstractSocket::IPv4Protocol)
      {
        QString item_text(addr.toString() + " (" + hname + ")");
        settings_dialog.bind_address->addItem(item_text, addr.toString());
      }
    }
  }
  int cur_index = settings_dialog.bind_address->findData(m_bind_address);
  if (cur_index > 0)
  {
    settings_dialog.bind_address->setCurrentIndex(cur_index);
  }
  
  settings_dialog.my_callsign->setText(m_callsign);
  settings_dialog.my_password->setText(m_password);
  settings_dialog.my_vpassword->setText(m_password);
  settings_dialog.my_name->setText(m_name);
  settings_dialog.my_location->setText(m_location);
  settings_dialog.my_info->setText(m_info);
  
  settings_dialog.directory_servers->setText(m_directory_servers);
  settings_dialog.list_refresh_time->setValue(m_list_refresh_time);
  settings_dialog.start_as_busy_checkbox->setChecked(m_start_as_busy);
  settings_dialog.proxy_enabled->setChecked(m_proxy_enabled);
  settings_dialog.proxy_server->setText(m_proxy_server);
  settings_dialog.proxy_port->setValue(m_proxy_port);
  settings_dialog.proxy_password->setText(m_proxy_password);

  settings_dialog.mic_audio_device->setText(m_mic_audio_device);
  settings_dialog.spkr_audio_device->setText(m_spkr_audio_device);
  settings_dialog.use_full_duplex->setChecked(m_use_full_duplex);
  settings_dialog.connect_sound->setText(m_connect_sound);
  QString card_sample_rate_str = QString::number(m_card_sample_rate);
  int card_sample_rate_idx =
          settings_dialog.card_sample_rate->findText(card_sample_rate_str);
  if (card_sample_rate_idx != -1)
  {
    settings_dialog.card_sample_rate->setCurrentIndex(card_sample_rate_idx);
  }

  settings_dialog.chat_encoding->setCurrentIndex(m_chat_encoding);

  bool cfg_done = false;
  while (!cfg_done)
  {
    if (settings_dialog.exec() == QDialog::Accepted)
    {
      if (settings_dialog.my_password->text() !=
	  settings_dialog.my_vpassword->text())
      {
	QMessageBox::critical(&settings_dialog,
	    SettingsDialog::tr("Qtel: Password mismatch"),
	    SettingsDialog::tr("Passwords do not match"));
      	settings_dialog.my_password->setText("");
      	settings_dialog.my_vpassword->setText("");
      }
      else if (settings_dialog.proxy_enabled->isChecked() &&
               (settings_dialog.proxy_server->text().isEmpty()))
      {
        QMessageBox::critical(&settings_dialog,
            SettingsDialog::tr("EchoLink proxy configuration problem"),
            SettingsDialog::tr(
              "EchoLink proxy enabled but no server given"));
      }
      else
      {
	m_callsign = settings_dialog.my_callsign->text().toUpper();
	m_password = settings_dialog.my_password->text().toUpper();
	m_name = settings_dialog.my_name->text();
	m_location = settings_dialog.my_location->text();
	m_info = settings_dialog.my_info->toPlainText();
	
	m_directory_servers = settings_dialog.directory_servers->text();
	m_list_refresh_time = settings_dialog.list_refresh_time->value();
	m_start_as_busy = settings_dialog.start_as_busy_checkbox->isChecked();
        int cur_index = settings_dialog.bind_address->currentIndex();
        if (cur_index > 0)
        {
	  m_bind_address = settings_dialog.bind_address->itemData(cur_index).toString();
        }
        else
        {
          m_bind_address = "";
        }
	m_proxy_enabled = settings_dialog.proxy_enabled->isChecked();
	m_proxy_server = settings_dialog.proxy_server->text();
	m_proxy_port = settings_dialog.proxy_port->value();
	m_proxy_password = settings_dialog.proxy_password->text();

	m_mic_audio_device = settings_dialog.mic_audio_device->text();
	m_spkr_audio_device = settings_dialog.spkr_audio_device->text();
	m_use_full_duplex = settings_dialog.use_full_duplex->isChecked();
	m_connect_sound = settings_dialog.connect_sound->text();
        m_card_sample_rate =
                settings_dialog.card_sample_rate->currentText().toInt();
	
	m_chat_encoding = settings_dialog.chat_encoding->currentIndex();

	QSettings qsettings;
	qsettings.setValue(CONF_CALLSIGN, m_callsign);
	qsettings.setValue(CONF_PASSWORD, m_password);
	qsettings.setValue(CONF_NAME, m_name);
	qsettings.setValue(CONF_LOCATION, m_location);
	qsettings.setValue(CONF_INFO, m_info);
	
	qsettings.setValue(CONF_DIRECTORY_SERVER, m_directory_servers);
	qsettings.setValue(CONF_LIST_REFRESH_TIME, m_list_refresh_time);
	qsettings.setValue(CONF_START_AS_BUSY, m_start_as_busy);
	qsettings.setValue(CONF_BIND_ADDRESS, m_bind_address);
	qsettings.setValue(CONF_PROXY_ENABLED, m_proxy_enabled);
	qsettings.setValue(CONF_PROXY_SERVER, m_proxy_server);
	qsettings.setValue(CONF_PROXY_PORT, m_proxy_port);
	qsettings.setValue(CONF_PROXY_PASSWORD, m_proxy_password);
      	
	qsettings.setValue(CONF_MIC_AUDIO_DEVICE, m_mic_audio_device);
	qsettings.setValue(CONF_SPKR_AUDIO_DEVICE, m_spkr_audio_device);
	qsettings.setValue(CONF_USE_FULL_DUPLEX, m_use_full_duplex);
	qsettings.setValue(CONF_CONNECT_SOUND, m_connect_sound);
	qsettings.setValue(CONF_CARD_SAMPLE_RATE, m_card_sample_rate);
      	
	qsettings.setValue(CONF_CHAT_ENCODING,
			     encodings[m_chat_encoding].name);
	
	configurationUpdated();
	
	cfg_done = true;
      }
    }
    else
    {
      cfg_done = true;
    }
  }
    
} /* Settings::showDialog */


void Settings::readSettings(void)
{
  QSettings qsettings;
  m_callsign = qsettings.value(CONF_CALLSIGN).toString().toUpper();
  m_password = qsettings.value(CONF_PASSWORD).toString().toUpper();
  m_name = qsettings.value(CONF_NAME).toString();
  m_location = qsettings.value(CONF_LOCATION).toString();
  m_info = qsettings.value(CONF_INFO, CONF_INFO_DEFAULT).toString();
  
  m_directory_servers = qsettings.value(CONF_DIRECTORY_SERVER,
      CONF_DIRECTORY_SERVER_DEFAULT).toString();
  m_list_refresh_time = qsettings.value(CONF_LIST_REFRESH_TIME,
      CONF_LIST_REFRESH_TIME_DEFAULT).toInt();
  m_start_as_busy = qsettings.value(CONF_START_AS_BUSY,
      CONF_START_AS_BUSY_DEFAULT).toBool();
  m_bind_address = qsettings.value(CONF_BIND_ADDRESS).toString();
  m_proxy_enabled = qsettings.value(CONF_PROXY_ENABLED,
      CONF_PROXY_ENABLED_DEFAULT).toBool();
  m_proxy_server = qsettings.value(CONF_PROXY_SERVER,
      CONF_PROXY_SERVER_DEFAULT).toString();
  m_proxy_port = qsettings.value(CONF_PROXY_PORT,
      CONF_PROXY_PORT_DEFAULT).toUInt();
  m_proxy_password = qsettings.value(CONF_PROXY_PASSWORD,
      CONF_PROXY_PASSWORD_DEFAULT).toString();
  
  m_mic_audio_device = qsettings.value(CONF_MIC_AUDIO_DEVICE,
      CONF_AUDIO_DEVICE_DEFAULT).toString();
  m_spkr_audio_device = qsettings.value(CONF_SPKR_AUDIO_DEVICE,
      CONF_AUDIO_DEVICE_DEFAULT).toString();
  m_use_full_duplex = qsettings.value(CONF_USE_FULL_DUPLEX,
      CONF_USE_FULL_DUPLEX_DEFAULT).toBool();
  m_connect_sound = qsettings.value(CONF_CONNECT_SOUND,
      CONF_CONNECT_SOUND_DEFAULT).toString();
  m_card_sample_rate = qsettings.value(CONF_CARD_SAMPLE_RATE,
      CONF_CARD_SAMPLE_RATE_DEFAULT).toInt();
  
  m_chat_encoding = 0;
  QString encoding_name = qsettings.value(CONF_CHAT_ENCODING,
      CONF_CHAT_ENCODING_DEFAULT).toString();
  for (QVector<Encoding>::size_type i=0; i<encodings.size(); ++i)
  {
    if (encodings[i].name == encoding_name)
    {
      m_chat_encoding = i;
      break;
    }
  }

  m_bookmarks = qsettings.value(CONF_BOOKMARKS).toStringList();
  
    // FIXME: should probably be restored through the restoreState method.
  m_main_window_size = qsettings.value(CONF_MAIN_WINDOW_SIZE, QSize()).toSize();

    // FIXME: Use a QList<int> instead.
  m_vsplitter_sizes.clear();
  QStringList str_list = qsettings.value(CONF_VSPLITTER_SIZES).toStringList();
  if (str_list.size() != 2)
  {
    foreach (QString size_str, str_list)
    {
      m_vsplitter_sizes.push_back(atoi(size_str.toStdString().c_str()));
    }
  }
  else
  {
    m_vsplitter_sizes.push_back(200);
    m_vsplitter_sizes.push_back(150);
  }
  m_hsplitter_sizes.clear();
  str_list = qsettings.value(CONF_HSPLITTER_SIZES).toStringList();
  if (str_list.size() != 2)
  {
    foreach (QString size_str, str_list)
    {
      m_hsplitter_sizes.push_back(atoi(size_str.toStdString().c_str()));
    }
  }
  else
  {
    m_hsplitter_sizes.push_back(130);
    m_hsplitter_sizes.push_back(540);
  }
  m_stn_view_col_sizes.clear();
  str_list = qsettings.value(CONF_STN_VIEW_COL_SIZES).toStringList();
  foreach (QString size_str, str_list)
  {
    m_stn_view_col_sizes.push_back(atoi(size_str.toStdString().c_str()));
  }
  m_incm_view_col_sizes.clear();
  str_list = qsettings.value(CONF_INCM_VIEW_COL_SIZES).toStringList();
  foreach (QString size_str, str_list)
  {
    m_incm_view_col_sizes.push_back(atoi(size_str.toStdString().c_str()));
  }
  
  m_vox_enabled = qsettings.value(CONF_VOX_ENABLED,
      CONF_VOX_ENABLED_DEFAULT).toBool();
  m_vox_threshold = qsettings.value(CONF_VOX_THRESHOLD,
      CONF_VOX_THRESHOLD_DEFAULT).toInt();
  m_vox_delay = qsettings.value(CONF_VOX_DELAY,
      CONF_VOX_DELAY_DEFAULT).toInt();
  
  m_connect_to_ip = qsettings.value(CONF_CONNECT_TO_IP, "").toString();

  if (m_callsign.isEmpty() || m_password.isEmpty() || m_name.isEmpty())
  {
    showDialog();
  }
  
} /* Settings::readSettings */


void Settings::setBookmarks(const QStringList &bookmarks)
{
  m_bookmarks = bookmarks;
  QSettings qsettings;
  qsettings.setValue(CONF_BOOKMARKS, m_bookmarks);
} /* Settings::setBookmarks */


void Settings::setMainWindowSize(QSize size)
{
  m_main_window_size = size;
  QSettings qsettings;
  qsettings.setValue(CONF_MAIN_WINDOW_SIZE, size);
} /* Settings::setMainWindowSize */


void Settings::setVSplitterSizes(QList<int> sizes)
{
  m_vsplitter_sizes = sizes;
  QStringList str_list;
  foreach (int size, sizes)
  {
    str_list.push_back(QString("%1").arg(size));
  }
  QSettings qsettings;
  qsettings.setValue(CONF_VSPLITTER_SIZES, str_list);
} /* Settings::setVSplitterSizes */


void Settings::setHSplitterSizes(QList<int> sizes)
{
  m_hsplitter_sizes = sizes;
  QStringList str_list;
  foreach (int size, sizes)
  {
    str_list.push_back(QString("%1").arg(size));
  }
  QSettings qsettings;
  qsettings.setValue(CONF_HSPLITTER_SIZES, str_list);
} /* Settings::setHSplitterSizes */


void Settings::setStationViewColSizes(QList<int> sizes)
{
  m_stn_view_col_sizes = sizes;
  QStringList str_list;
  foreach (int size, sizes)
  {
    str_list.push_back(QString("%1").arg(size));
  }
  QSettings qsettings;
  qsettings.setValue(CONF_STN_VIEW_COL_SIZES, str_list);
} /* Settings::setStationViewColSizes */


void Settings::setIncomingViewColSizes(QList<int> sizes)
{
  m_incm_view_col_sizes = sizes;
  QStringList str_list;
  foreach (int size, sizes)
  {
    str_list.push_back(QString("%1").arg(size));
  }
  QSettings qsettings;
  qsettings.setValue(CONF_INCM_VIEW_COL_SIZES, str_list);
} /* Settings::setStationViewColSizes */


void Settings::setVoxParams(bool enabled, int threshold_db, int delay_ms)
{
  m_vox_enabled = enabled;
  m_vox_threshold = threshold_db;
  m_vox_delay = delay_ms;
  QSettings qsettings;
  qsettings.setValue(CONF_VOX_ENABLED, m_vox_enabled);
  qsettings.setValue(CONF_VOX_THRESHOLD, m_vox_threshold);
  qsettings.setValue(CONF_VOX_DELAY, m_vox_delay);
} /* Settings::setVoxParams */


void Settings::setConnectToIp(const QString &host)
{
  m_connect_to_ip = host;
  QSettings qsettings;
  qsettings.setValue(CONF_CONNECT_TO_IP, m_connect_to_ip);
} /* Settings::setConnectToIp */




/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
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
 * This file has not been truncated
 */

