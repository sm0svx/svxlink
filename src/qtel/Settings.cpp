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

#define CONF_BOOKMARKS 	      	      CONF_APP_NAME "/Bookmarks"
#define CONF_MAIN_WINDOW_SIZE_WIDTH   CONF_APP_NAME "/MwWidth"
#define CONF_MAIN_WINDOW_SIZE_HEIGHT  CONF_APP_NAME "/MwHeight"

#define CONF_INFO_DEFAULT     	      "Running Qtel, an EchoLink client for " \
      	      	      	      	      "Linux"
				      
#define CONF_DIRECTORY_SERVER_DEFAULT 	"server1.echolink.org"
#define CONF_LIST_REFRESH_TIME_DEFAULT	5
#define CONF_START_AS_BUSY_DEFAULT    	false



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
  
  m_bookmarks = qsettings.readListEntry(CONF_BOOKMARKS);
  
  m_main_window_size =
      QSize(qsettings.readNumEntry(CONF_MAIN_WINDOW_SIZE_WIDTH, 0),
      	    qsettings.readNumEntry(CONF_MAIN_WINDOW_SIZE_HEIGHT, 0));
  
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
} /* Settings::setBookmarks */




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

