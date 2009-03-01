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

/** @example Template_demo.cpp
An example of how to use the Template class
*/


#ifndef SETTINGS_INCLUDED
#define SETTINGS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <qstringlist.h>
#include <qsize.h>
#undef emit


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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class SettingsDialog;


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
 * Class:     Settings
 * Purpose:   Settings class
 * Inherits:
 * Input:
 * Output:
 * Author:
 * Created:
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
class Settings : public SigC::Object
{
  public:
    static Settings *instance(void);

    /*
     *------------------------------------------------------------------------
     * Method:
     * Purpose:
     * Input:
     * Output:	None
     * Author:
     * Created:
     * Remarks:
     * Bugs:
     *------------------------------------------------------------------------
     */
    ~Settings(void);

    void showDialog(void);
    void readSettings(void);
    const QString& callsign(void) const { return m_callsign; }
    const QString& password(void) const { return m_password; }
    const QString& name(void) const { return m_name; }
    const QString& location(void) const { return m_location; }
    const QString& info(void) const { return m_info; }

    // APRS configuration
    bool aprsbeacon_enabled(void) const { return m_aprsbeacon_enabled; }
    bool loc_as_aprs_comment(void) const { return m_loc_as_aprs_comment; }
    const QString& directoryServer(void) const { return m_directory_server; }
    int listRefreshTime(void) const { return m_list_refresh_time; }
    int aprsbeacon_interval(void) const { return m_aprsbeacon_interval; }
    const QString& aprs_comment(void) const { return m_aprs_comment; }
    const QString& aprsserver(void) const { return m_aprsserver; }
    const QString& latitude(void) const { return m_latitude; }
    const QString& longitude(void) const { return m_longitude; }
    const QString& north_south(void) const { return m_north_south; }
    const QString& east_west(void) const { return m_east_west; }
    const char aprs_icon(void) const
    {
        const char icon[8] = {'/','#','$','&','-','`','\\','x'};
        return icon[m_aprs_icon];
    }
    int aprsport(void) const { return m_aprsport; }

    bool startAsBusy(void) const { return m_start_as_busy; }

    const QString& audioDevice(void) const { return m_audio_device; }
    bool useFullDuplex(void) const { return m_use_full_duplex; }
    const QString& connectSound(void) const { return m_connect_sound; }
    bool rogerBeepEnabled(void) const { return m_roger_beep_enabled; }
    int rogerBeepLength(void) const { return m_roger_beep_length; }
    int rogerBeepFq(void) const { return m_roger_beep_fq; }
    int rogerBeepLevel(void) const { return m_roger_beep_level; }

    int dtmf_level(void) const { return m_dtmf_level; }
    int dtmf_length(void) const { return m_dtmf_length; }
    int dtmf_tone_space(void) const { return m_dtmf_tone_space; }

    void setBookmarks(const QStringList &bookmarks);
    const QStringList& bookmarks(void) const { return m_bookmarks; }

    QSize mainWindowSize(void) const { return m_main_window_size; }
    void setMainWindowSize(QSize size);
    QValueList<int> vSplitterSizes() const { return m_vsplitter_sizes; }
    void setVSplitterSizes(QValueList<int> sizes);
    QValueList<int> hSplitterSizes() const { return m_hsplitter_sizes; }
    void setHSplitterSizes(QValueList<int> sizes);

    void setVoxParams(bool enabled, int threshold_db, int delay_ms);
    bool voxEnabled(void) const { return m_vox_enabled; }

    bool location_as_aprs_comment(void) { return m_location_as_aprs_comment; }
    int voxThreshold(void) const { return m_vox_threshold; }
    int voxDelay(void) const { return m_vox_delay; }

    void setConnectToIp(const QString &host);
    const QString& connectToIp(void) const { return m_connect_to_ip; }

    SigC::Signal0<void> configurationUpdated;

  protected:

  private:
    static Settings * the_instance;

    SettingsDialog *  dialog;

    QString   	      m_callsign;
    QString   	      m_password;
    QString   	      m_name;
    QString   	      m_location;
    QString   	      m_info;
    QString           m_aprs_comment;
    QString           m_latitude;
    QString           m_longitude;
    QString           m_aprsserver;
    QString           m_north_south;
    QString           m_east_west;
    int               m_aprs_icon;

    QString   	      m_directory_server;
    int       	      m_list_refresh_time;
    int               m_aprsport;
    int               m_aprsbeacon_interval;
    bool      	      m_start_as_busy;

    QString   	      m_audio_device;
    bool      	      m_use_full_duplex;
    QString   	      m_connect_sound;
    bool              m_roger_beep_enabled;
    int               m_roger_beep_fq;
    int               m_roger_beep_length;
    int               m_roger_beep_level;
    int               m_dtmf_level;
    int               m_dtmf_tone_space;
    int               m_dtmf_length;

    QStringList       m_bookmarks;

    QSize     	      m_main_window_size;
    QValueList<int>   m_vsplitter_sizes;
    QValueList<int>   m_hsplitter_sizes;

    bool      	      m_vox_enabled;
    bool              m_aprsbeacon_enabled;
    bool              m_loc_as_aprs_comment;
    bool              m_location_as_aprs_comment;
    int       	      m_vox_threshold;
    int       	      m_vox_delay;

    QString   	      m_connect_to_ip;

    Settings(void);

};  /* class Settings */

//} /* namespace */

#endif /* SETTINGS_INCLUDED */



/*
 * This file has not been truncated
 */

