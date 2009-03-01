/**
@file	 Settings.h
@brief   Handle application settings.
@author  Tobias Blomberg
@date	 2003-03-30

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
#include <qcombobox.h>
#include <qlistbox.h>
#include <qstringlist.h>
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
#include "images/033.xpm"
#include "images/035.xpm"
#include "images/036.xpm"
#include "images/038.xpm"
#include "images/045.xpm"
#include "images/092.xpm"
#include "images/096.xpm"
#include "images/120.xpm"


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

#define CONF_APRSBEACON_ENABLED       CONF_APP_NAME "/APRSenabled"
#define CONF_LOCATION_AS_APRSCOMMENT  CONF_APP_NAME "/APRSlocationasaprscomment"
#define CONF_APRSBEACON_INTERVAL      CONF_APP_NAME "/APRSbeaconinterval"
#define CONF_APRS_COMMENT             CONF_APP_NAME "/APRScomment"
#define CONF_LATITUDE                 CONF_APP_NAME "/APRSlatitude"
#define CONF_LONGITUDE                CONF_APP_NAME "/APRSlongitude"
#define CONF_APRSSERVER               CONF_APP_NAME "/APRSserver"
#define CONF_APRSPORT                 CONF_APP_NAME "/APRSport"
#define CONF_EAST_WEST                CONF_APP_NAME "/APRSEastWest"
#define CONF_NORTH_SOUTH              CONF_APP_NAME "/APRSNorthSouth"
#define CONF_LOC_AS_APRS_COMMENT      CONF_APP_NAME "/APRSlocascomment"
#define CONF_APRS_ICON                CONF_APP_NAME "/APRSicon"

#define CONF_ROGER_BEEP_ENABLED       CONF_APP_NAME "/RogerBeepEnabled"
#define CONF_ROGER_BEEP_FQ            CONF_APP_NAME "/RogerBeepFq"
#define CONF_ROGER_BEEP_LENGTH        CONF_APP_NAME "/RogerBeepLength"
#define CONF_ROGER_BEEP_LEVEL         CONF_APP_NAME "/RogerBeepLevel"

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

#define CONF_AUDIO_DEVICE_DEFAULT 	"/dev/dsp"
#define CONF_USE_FULL_DUPLEX_DEFAULT    false
#define CONF_CONNECT_SOUND_DEFAULT 	"/usr/share/qtel/sounds/connect.raw"

#define CONF_VOX_ENABLED_DEFAULT      	true
#define CONF_VOX_THRESHOLD_DEFAULT    	-30
#define CONF_VOX_DELAY_DEFAULT      	1000
#define CONF_APRS_ENABLED_DEFAULT       false
#define CONF_LATITUDE_DEFAULT           "5125.97"
#define CONF_LONGITUDE_DEFAULT          "01214.75"
#define CONF_APRS_ICON_DEFAULT          1
#define CONF_APRSPORT_DEFAULT           14580
#define CONF_APRSSERVER_DEFAULT         "www.db0erf.de"
#define CONF_NORTH_SOUTH_DEFAULT        "N"
#define CONF_EAST_WEST_DEFAULT          "E"
#define CONF_ROGER_BEEP_FQ_DEFAULT      700
#define CONF_ROGER_BEEP_LENGTH_DEFAULT  300
#define CONF_ROGER_BEEP_LEVEL_DEFAULT   -12


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

} /* Settings::Settings */


Settings::~Settings(void)
{

} /* Settings::~Settings */


void Settings::showDialog(void)
{
  if (dialog != 0)
  {
    return;
  }

  SettingsDialog settings_dialog;
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
  settings_dialog.use_roger_beep->setChecked(m_roger_beep_enabled);
  settings_dialog.beep_length->setValue(m_roger_beep_length);
  settings_dialog.beep_fq->setValue(m_roger_beep_fq);
  settings_dialog.beep_level->setValue(m_roger_beep_level);

  settings_dialog.aprsbeacon_interval->setValue(m_aprsbeacon_interval);
  settings_dialog.aprsbeacon_enabled->setChecked(m_aprsbeacon_enabled);
  settings_dialog.loc_as_aprs_comment->setChecked(m_loc_as_aprs_comment);
  settings_dialog.aprs_comment->setText(m_aprs_comment);
  settings_dialog.latitude->setText(m_latitude);
  settings_dialog.longitude->setText(m_longitude);
  settings_dialog.north_south->setCurrentText(m_north_south);
  settings_dialog.east_west->setCurrentText(m_east_west);
  settings_dialog.aprs_icon->setCurrentItem(m_aprs_icon);

 // settings_dialog.aprs_icon->changeItem( QPixmap( "/usr/share/icons/033.xpm" ), 0 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_033_xpm ), 0 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_035_xpm ), 1 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_036_xpm ), 2 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_038_xpm ), 3 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_045_xpm ), 4 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_096_xpm ), 5 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_092_xpm ), 6 );
  settings_dialog.aprs_icon->changeItem( QPixmap( pic_120_xpm ), 7 );

  bool cfg_done = false;
  QString lat, lon;

  while (!cfg_done)
  {
    if (settings_dialog.exec() == QDialog::Accepted)
    {
        // checkout, if all APRS config has been done, don't start aprs without
        // reasonable values
      if (settings_dialog.aprsbeacon_enabled->isChecked())
      {
         if ((settings_dialog.latitude->text().length() != 7) ||
            (settings_dialog.longitude->text().length() != 8) ||
            settings_dialog.aprsserver->text().isEmpty())
         {
            QMessageBox::warning(&settings_dialog,
            SettingsDialog::trUtf8("Qtel: Missing or wrong APRS settings"),
            SettingsDialog::trUtf8("Qtel: If you want to enable the APRS "
                "function, please use valid values for latitude/longitude!\n"
                "APRS has been disabled!"));
      	    settings_dialog.aprsbeacon_enabled->setChecked(false);
         }

         if ((settings_dialog.latitude->text() == CONF_LATITUDE_DEFAULT) ||
           (settings_dialog.longitude->text() == CONF_LONGITUDE_DEFAULT))
         {
            QMessageBox::warning(&settings_dialog,
            SettingsDialog::trUtf8("Qtel: Possibly wrong APRS settings!"),
            SettingsDialog::trUtf8("Qtel: You didn't change the settings for "
                "latitude and/or longitude!\n"
                "If really true, you should leave immediatelly!"));
         }
      }

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

        m_roger_beep_enabled = settings_dialog.use_roger_beep->isChecked();
        m_roger_beep_level = settings_dialog.beep_level->value();
        m_roger_beep_fq = settings_dialog.beep_fq->value();
        m_roger_beep_length= settings_dialog.beep_length->value();
    
        m_directory_server = settings_dialog.directory_server->text();
        m_list_refresh_time = settings_dialog.list_refresh_time->value();
        m_start_as_busy = settings_dialog.start_as_busy_checkbox->isChecked();
    
        m_audio_device = settings_dialog.audio_device->text();
        m_use_full_duplex = settings_dialog.use_full_duplex->isChecked();
        m_connect_sound = settings_dialog.connect_sound->text();
        m_aprsbeacon_enabled = settings_dialog.aprsbeacon_enabled->isChecked();
        m_aprsbeacon_interval = settings_dialog.aprsbeacon_interval->value();
        m_aprs_comment = settings_dialog.aprs_comment->text();
        m_latitude = settings_dialog.latitude->text();
        m_longitude = settings_dialog.longitude->text();
        m_loc_as_aprs_comment = settings_dialog.loc_as_aprs_comment->isChecked();
        m_aprsserver = settings_dialog.aprsserver->text();
        m_aprsport = settings_dialog.aprsport->value();
        m_north_south = settings_dialog.north_south->text(settings_dialog.north_south->currentItem());
        m_east_west = settings_dialog.east_west->text(settings_dialog.east_west->currentItem());
    //   m_aprs_icon = settings_dialog.aprs_icon->text(settings_dialog.aprs_icon->currentItem());
        m_aprs_icon = settings_dialog.aprs_icon->currentItem();

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
        qsettings.writeEntry(CONF_ROGER_BEEP_ENABLED, m_roger_beep_enabled);
        qsettings.writeEntry(CONF_ROGER_BEEP_FQ, m_roger_beep_fq);
        qsettings.writeEntry(CONF_ROGER_BEEP_LENGTH, m_roger_beep_length);
        qsettings.writeEntry(CONF_ROGER_BEEP_LEVEL, m_roger_beep_level);

        qsettings.writeEntry(CONF_APRSBEACON_ENABLED, m_aprsbeacon_enabled);
        qsettings.writeEntry(CONF_APRSBEACON_INTERVAL, m_aprsbeacon_interval);
        qsettings.writeEntry(CONF_LOC_AS_APRS_COMMENT, m_loc_as_aprs_comment);
        qsettings.writeEntry(CONF_APRS_COMMENT, m_aprs_comment);
        qsettings.writeEntry(CONF_LATITUDE, m_latitude);
        qsettings.writeEntry(CONF_LONGITUDE, m_longitude);
        qsettings.writeEntry(CONF_LOCATION_AS_APRSCOMMENT, m_location_as_aprs_comment);
        qsettings.writeEntry(CONF_APRSSERVER, m_aprsserver);
        qsettings.writeEntry(CONF_APRSPORT, m_aprsport);
        qsettings.writeEntry(CONF_NORTH_SOUTH, m_north_south);
        qsettings.writeEntry(CONF_EAST_WEST, m_east_west);
        qsettings.writeEntry(CONF_APRS_ICON, m_aprs_icon);

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
  m_roger_beep_enabled = qsettings.readBoolEntry(CONF_ROGER_BEEP_ENABLED);
  m_roger_beep_fq = qsettings.readNumEntry(CONF_ROGER_BEEP_FQ,
                                           CONF_ROGER_BEEP_FQ_DEFAULT);
  m_roger_beep_length= qsettings.readNumEntry(CONF_ROGER_BEEP_LENGTH,
                                              CONF_ROGER_BEEP_LENGTH_DEFAULT);
  m_roger_beep_level = qsettings.readNumEntry(CONF_ROGER_BEEP_LEVEL,
                                              CONF_ROGER_BEEP_LEVEL_DEFAULT);

  m_aprsbeacon_enabled = qsettings.readBoolEntry(CONF_APRSBEACON_ENABLED,
      CONF_APRS_ENABLED_DEFAULT);
  m_aprsbeacon_interval = qsettings.readNumEntry(CONF_APRSBEACON_INTERVAL);
  m_aprs_comment = qsettings.readEntry(CONF_APRS_COMMENT);
  m_latitude = qsettings.readEntry(CONF_LATITUDE,CONF_LATITUDE_DEFAULT);
  m_longitude = qsettings.readEntry(CONF_LONGITUDE,CONF_LONGITUDE_DEFAULT);
  m_location_as_aprs_comment = qsettings.readBoolEntry(CONF_LOCATION_AS_APRSCOMMENT);
  m_aprsserver = qsettings.readEntry(CONF_APRSSERVER,CONF_APRSSERVER_DEFAULT);
  m_aprsport= qsettings.readNumEntry(CONF_APRSPORT,CONF_APRSPORT_DEFAULT);
  m_north_south = qsettings.readEntry(CONF_NORTH_SOUTH,CONF_NORTH_SOUTH_DEFAULT);
  m_east_west = qsettings.readEntry(CONF_EAST_WEST,CONF_EAST_WEST_DEFAULT);
  m_aprs_icon = qsettings.readNumEntry(CONF_APRS_ICON,CONF_APRS_ICON_DEFAULT);

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

