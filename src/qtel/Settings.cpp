/**
@file	 Settings.cpp
@brief   Handle application settings.
@author  Tobias Blomberg
@date	 2003-03-30

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

#include <qsettings.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qcombobox.h>
#undef emit

#include <stdlib.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



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

#define CONF_SEARCH_PATH  "/SM0SVX"
#define CONF_APP_NAME 	  "/Qtel"

#define CONF_CALLSIGN 	      	      CONF_APP_NAME "/Callsign"
#define CONF_PASSWORD 	      	      CONF_APP_NAME "/Password"
#define CONF_NAME     	      	      CONF_APP_NAME "/Name"
#define CONF_LOCATION 	      	      CONF_APP_NAME "/Location"
#define CONF_INFO     	      	      CONF_APP_NAME "/Info"

#define CONF_DIRECTORY_SERVER 	      CONF_APP_NAME "/DirectoryServer"
#define CONF_LIST_REFRESH_TIME        CONF_APP_NAME "/ListRefreshTime"
#define CONF_START_AS_BUSY            CONF_APP_NAME "/StartAsBusy"

#define CONF_AUDIO_DEVICE     	      CONF_APP_NAME "/AudioDevice"
#define CONF_USE_FULL_DUPLEX          CONF_APP_NAME "/UseFullDuplex"
#define CONF_CONNECT_SOUND            CONF_APP_NAME "/ConnectSound"

#define CONF_CHAT_ENCODING	      CONF_APP_NAME "/ChatEncoding"

#define CONF_BOOKMARKS 	      	      CONF_APP_NAME "/Bookmarks"
#define CONF_MAIN_WINDOW_SIZE_WIDTH   CONF_APP_NAME "/MwWidth"
#define CONF_MAIN_WINDOW_SIZE_HEIGHT  CONF_APP_NAME "/MwHeight"
#define CONF_VSPLITTER_SIZES  	      CONF_APP_NAME "/VSplitterSizes"
#define CONF_HSPLITTER_SIZES  	      CONF_APP_NAME "/HSplitterSizes"

#define CONF_VOX_ENABLED      	      CONF_APP_NAME "/VoxEnabled"
#define CONF_VOX_THRESHOLD            CONF_APP_NAME "/VoxThreshold"
#define CONF_VOX_DELAY      	      CONF_APP_NAME "/VoxDelay"

#define CONF_CONNECT_TO_IP            CONF_APP_NAME "/ConnectToIp"

#define CONF_INFO_DEFAULT     	      "Running Qtel, an EchoLink client for " \
      	      	      	      	      "Linux"
				      
#define CONF_DIRECTORY_SERVER_DEFAULT 	"server1.echolink.org"
#define CONF_LIST_REFRESH_TIME_DEFAULT	5
#define CONF_START_AS_BUSY_DEFAULT    	false

#define CONF_AUDIO_DEVICE_DEFAULT 	"alsa:default"
#define CONF_USE_FULL_DUPLEX_DEFAULT    false
#define CONF_CONNECT_SOUND_DEFAULT 	"/usr/share/qtel/sounds/connect.raw"

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
  : dialog(0)
{
  encodings.push_back(Encoding("Big5", SettingsDialog::trUtf8("Chinese")));
  encodings.push_back(Encoding("Big5-HKSCS", 
			       SettingsDialog::trUtf8("Chinese")));
  encodings.push_back(Encoding("eucJP", SettingsDialog::trUtf8("Japanese")));
  encodings.push_back(Encoding("eucKR", SettingsDialog::trUtf8("Korean")));
  encodings.push_back(Encoding("GB2312", SettingsDialog::trUtf8("Chinese")));
  encodings.push_back(Encoding("GBK", SettingsDialog::trUtf8("Chinese")));
  encodings.push_back(Encoding("GB18030", SettingsDialog::trUtf8("Chinese")));
  encodings.push_back(Encoding("JIS7", SettingsDialog::trUtf8("Japanese")));
  encodings.push_back(Encoding("Shift-JIS",
			       SettingsDialog::trUtf8("Japanese")));
  encodings.push_back(Encoding("TSCII", SettingsDialog::trUtf8("Tamil")));
  encodings.push_back(Encoding("utf8",
			       SettingsDialog::trUtf8("Unicode, 8-bit")));
  encodings.push_back(Encoding("KOI8-R", SettingsDialog::trUtf8("Russian")));
  encodings.push_back(Encoding("KOI8-U", SettingsDialog::trUtf8("Ukrainian")));
  encodings.push_back(Encoding("ISO8859-1", SettingsDialog::trUtf8("Western")));
  encodings.push_back(Encoding("ISO8859-2", SettingsDialog::trUtf8("Central European")));
  encodings.push_back(Encoding("ISO8859-3", SettingsDialog::trUtf8("Central European")));
  encodings.push_back(Encoding("ISO8859-4", SettingsDialog::trUtf8("Baltic")));
  encodings.push_back(Encoding("ISO8859-5",
			       SettingsDialog::trUtf8("Cyrillic")));
  encodings.push_back(Encoding("ISO8859-6", SettingsDialog::trUtf8("Arabic")));
  encodings.push_back(Encoding("ISO8859-7", SettingsDialog::trUtf8("Greek")));
  encodings.push_back(
    Encoding("ISO8859-8", SettingsDialog::trUtf8("Hebrew, visually ordered")));
  encodings.push_back(
    Encoding("ISO8859-8-i",
	     SettingsDialog::trUtf8("Hebrew, logically ordered")));
  encodings.push_back(Encoding("ISO8859-9", SettingsDialog::trUtf8("Turkish")));
  encodings.push_back(Encoding("ISO8859-10"));
  encodings.push_back(Encoding("ISO8859-13"));
  encodings.push_back(Encoding("ISO8859-14"));
  encodings.push_back(Encoding("ISO8859-15",
			       SettingsDialog::trUtf8("Western")));
  encodings.push_back(Encoding("IBM850"));
  encodings.push_back(Encoding("IBM866"));
  encodings.push_back(Encoding("CP874"));
  encodings.push_back(Encoding("CP1250",
			       SettingsDialog::trUtf8("Central European")));
  encodings.push_back(Encoding("CP1251", SettingsDialog::trUtf8("Cyrillic")));
  encodings.push_back(Encoding("CP1252", SettingsDialog::trUtf8("Western")));
  encodings.push_back(Encoding("CP1253", SettingsDialog::trUtf8("Greek")));
  encodings.push_back(Encoding("CP1254", SettingsDialog::trUtf8("Turkish")));
  encodings.push_back(Encoding("CP1255", SettingsDialog::trUtf8("Hebrew")));
  encodings.push_back(Encoding("CP1255", SettingsDialog::trUtf8("Hebrew")));
  encodings.push_back(Encoding("CP1256", SettingsDialog::trUtf8("Arabic")));
  encodings.push_back(Encoding("CP1257", SettingsDialog::trUtf8("Baltic")));
  encodings.push_back(Encoding("CP1258"));
  encodings.push_back(Encoding("TIS-620", SettingsDialog::trUtf8("Thai")));
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

  for (QValueVector<Encoding>::size_type i = 0; i < encodings.size(); ++i)
  {
    QString str(encodings[i].name);
    if (!encodings[i].language.isEmpty())
    {
      str += " -- " + encodings[i].language;
    }
    settings_dialog.chat_encoding->insertItem(str);
  }
  
  settings_dialog.my_callsign->setText(m_callsign);
  settings_dialog.my_password->setText(m_password);
  settings_dialog.my_vpassword->setText(m_password);
  settings_dialog.my_name->setText(m_name);
  settings_dialog.my_location->setText(m_location);
  settings_dialog.my_info->setText(m_info);
  
  settings_dialog.directory_server->setText(m_directory_server);
  settings_dialog.list_refresh_time->setValue(m_list_refresh_time);
  settings_dialog.start_as_busy_checkbox->setChecked(m_start_as_busy);

  settings_dialog.audio_device->setText(m_audio_device);
  settings_dialog.use_full_duplex->setChecked(m_use_full_duplex);
  settings_dialog.connect_sound->setText(m_connect_sound);

  settings_dialog.chat_encoding->setCurrentItem(m_chat_encoding);

  bool cfg_done = false;
  while (!cfg_done)
  {
    if (settings_dialog.exec() == QDialog::Accepted)
    {
      if (settings_dialog.my_password->text() !=
	  settings_dialog.my_vpassword->text())
      {
	QMessageBox::critical(&settings_dialog,
	    SettingsDialog::trUtf8("Qtel: Password mismatch"),
	    SettingsDialog::trUtf8("Passwords do not match"));
      	settings_dialog.my_password->setText("");
      	settings_dialog.my_vpassword->setText("");
      }
      else
      {
	m_callsign = settings_dialog.my_callsign->text().upper();
	m_password = settings_dialog.my_password->text().upper();
	m_name = settings_dialog.my_name->text();
	m_location = settings_dialog.my_location->text();
	m_info = settings_dialog.my_info->text();
	
	m_directory_server = settings_dialog.directory_server->text();
	m_list_refresh_time = settings_dialog.list_refresh_time->value();
	m_start_as_busy = settings_dialog.start_as_busy_checkbox->isChecked();

	m_audio_device = settings_dialog.audio_device->text();
	m_use_full_duplex = settings_dialog.use_full_duplex->isChecked();
	m_connect_sound = settings_dialog.connect_sound->text();
	
	m_chat_encoding = settings_dialog.chat_encoding->currentItem();

	QSettings qsettings;
	qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
	qsettings.writeEntry(CONF_CALLSIGN, m_callsign);
	qsettings.writeEntry(CONF_PASSWORD, m_password);
	qsettings.writeEntry(CONF_NAME, m_name);
	qsettings.writeEntry(CONF_LOCATION, m_location);
	qsettings.writeEntry(CONF_INFO, m_info);
	
	qsettings.writeEntry(CONF_DIRECTORY_SERVER, m_directory_server);
	qsettings.writeEntry(CONF_LIST_REFRESH_TIME, m_list_refresh_time);
	qsettings.writeEntry(CONF_START_AS_BUSY, m_start_as_busy);
      	
	qsettings.writeEntry(CONF_AUDIO_DEVICE, m_audio_device);
	qsettings.writeEntry(CONF_USE_FULL_DUPLEX, m_use_full_duplex);
	qsettings.writeEntry(CONF_CONNECT_SOUND, m_connect_sound);
      	
	qsettings.writeEntry(CONF_CHAT_ENCODING,
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
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  m_callsign = qsettings.readEntry(CONF_CALLSIGN).upper();
  m_password = qsettings.readEntry(CONF_PASSWORD).upper();
  m_name = qsettings.readEntry(CONF_NAME);
  m_location = qsettings.readEntry(CONF_LOCATION);
  m_info = qsettings.readEntry(CONF_INFO, CONF_INFO_DEFAULT);
  
  m_directory_server = qsettings.readEntry(CONF_DIRECTORY_SERVER,
      CONF_DIRECTORY_SERVER_DEFAULT);
  m_list_refresh_time = qsettings.readNumEntry(CONF_LIST_REFRESH_TIME,
      CONF_LIST_REFRESH_TIME_DEFAULT);
  m_start_as_busy = qsettings.readBoolEntry(CONF_START_AS_BUSY,
      CONF_START_AS_BUSY_DEFAULT);
  
  m_audio_device = qsettings.readEntry(CONF_AUDIO_DEVICE,
      CONF_AUDIO_DEVICE_DEFAULT);
  m_use_full_duplex = qsettings.readBoolEntry(CONF_USE_FULL_DUPLEX,
      CONF_USE_FULL_DUPLEX_DEFAULT);
  m_connect_sound = qsettings.readEntry(CONF_CONNECT_SOUND,
      CONF_CONNECT_SOUND_DEFAULT);
  
  m_chat_encoding = 0;
  QString encoding_name = qsettings.readEntry(CONF_CHAT_ENCODING,
      CONF_CHAT_ENCODING_DEFAULT);
  for (QValueVector<Encoding>::size_type i = 0; i < encodings.size(); ++i)
  {
    if (encodings[i].name == encoding_name)
    {
      m_chat_encoding = i;
      break;
    }
  }

  m_bookmarks = qsettings.readListEntry(CONF_BOOKMARKS);
  
  m_main_window_size =
      QSize(qsettings.readNumEntry(CONF_MAIN_WINDOW_SIZE_WIDTH, 0),
      	    qsettings.readNumEntry(CONF_MAIN_WINDOW_SIZE_HEIGHT, 0));
  QStringList::const_iterator it;
  m_vsplitter_sizes.clear();
  QStringList str_list = qsettings.readListEntry(CONF_VSPLITTER_SIZES);
  if (str_list.size() != 2)
  {
    for (it=str_list.begin(); it!=str_list.end(); ++it)
    {
      m_vsplitter_sizes.push_back(atoi((*it).latin1()));
    }
  }
  else
  {
    m_vsplitter_sizes.push_back(200);
    m_vsplitter_sizes.push_back(150);
  }
  m_hsplitter_sizes.clear();
  str_list = qsettings.readListEntry(CONF_HSPLITTER_SIZES);
  if (str_list.size() != 2)
  {
    for (it=str_list.begin(); it!=str_list.end(); ++it)
    {
      m_hsplitter_sizes.push_back(atoi((*it).latin1()));
    }
  }
  else
  {
    m_hsplitter_sizes.push_back(130);
    m_hsplitter_sizes.push_back(540);
  }
  
  m_vox_enabled = qsettings.readBoolEntry(CONF_VOX_ENABLED,
      CONF_VOX_ENABLED_DEFAULT);
  m_vox_threshold = qsettings.readNumEntry(CONF_VOX_THRESHOLD,
      CONF_VOX_THRESHOLD_DEFAULT);
  m_vox_delay = qsettings.readNumEntry(CONF_VOX_DELAY,
      CONF_VOX_DELAY_DEFAULT);
  
  m_connect_to_ip = qsettings.readEntry(CONF_CONNECT_TO_IP, "");

  if (m_callsign.isEmpty() || m_password.isEmpty() || m_name.isEmpty())
  {
    showDialog();
  }
  
} /* Settings::readSettings */


void Settings::setBookmarks(const QStringList &bookmarks)
{
  m_bookmarks = bookmarks;
  QSettings qsettings;
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  qsettings.writeEntry(CONF_BOOKMARKS, m_bookmarks);
} /* Settings::setBookmarks */


void Settings::setMainWindowSize(QSize size)
{
  m_main_window_size = size;
  QSettings qsettings;
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  qsettings.writeEntry(CONF_MAIN_WINDOW_SIZE_WIDTH, size.width());
  qsettings.writeEntry(CONF_MAIN_WINDOW_SIZE_HEIGHT, size.height());
} /* Settings::setMainWindowSize */


void Settings::setVSplitterSizes(QValueList<int> sizes)
{
  m_vsplitter_sizes = sizes;
  QStringList str_list;
  QValueList<int>::const_iterator it;
  for (it=sizes.begin(); it!=sizes.end(); ++it)
  {
    str_list.push_back(QString("%1").arg(*it));
  }
  QSettings qsettings;
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  qsettings.writeEntry(CONF_VSPLITTER_SIZES, str_list);
} /* Settings::setVSplitterSizes */


void Settings::setHSplitterSizes(QValueList<int> sizes)
{
  m_hsplitter_sizes = sizes;
  QStringList str_list;
  QValueList<int>::const_iterator it;
  for (it=sizes.begin(); it!=sizes.end(); ++it)
  {
    str_list.push_back(QString("%1").arg(*it));
  }
  QSettings qsettings;
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  qsettings.writeEntry(CONF_HSPLITTER_SIZES, str_list);
} /* Settings::setHSplitterSizes */


void Settings::setVoxParams(bool enabled, int threshold_db, int delay_ms)
{
  m_vox_enabled = enabled;
  m_vox_threshold = threshold_db;
  m_vox_delay = delay_ms;
  QSettings qsettings;
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  qsettings.writeEntry(CONF_VOX_ENABLED, m_vox_enabled);
  qsettings.writeEntry(CONF_VOX_THRESHOLD, m_vox_threshold);
  qsettings.writeEntry(CONF_VOX_DELAY, m_vox_delay);
} /* Settings::setVoxParams */


void Settings::setConnectToIp(const QString &host)
{
  m_connect_to_ip = host;
  QSettings qsettings;
  qsettings.insertSearchPath(QSettings::Windows, CONF_SEARCH_PATH);
  qsettings.writeEntry(CONF_CONNECT_TO_IP, m_connect_to_ip);
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

