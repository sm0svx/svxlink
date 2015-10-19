/**
@file	 Settings.h
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

#ifndef SETTINGS_INCLUDED
#define SETTINGS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <QStringList>
#include <QSize>
#include <QVector>
#include <QList>
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
class Settings : public sigc::trackable
{
  public:
    static Settings *instance(void);
  
    ~Settings(void);
    
    void showDialog(void);
    void readSettings(void);
    const QString& callsign(void) const { return m_callsign; }
    const QString& password(void) const { return m_password; }
    const QString& name(void) const { return m_name; }
    const QString& location(void) const { return m_location; }
    const QString& info(void) const { return m_info; }
    
    const QString& directoryServers(void) const { return m_directory_servers; }
    int listRefreshTime(void) const { return m_list_refresh_time; }
    bool startAsBusy(void) const { return m_start_as_busy; }
    const QString& bindAddress(void) const { return m_bind_address; }
    bool proxyEnabled(void) const { return m_proxy_enabled; }
    const QString& proxyServer(void) const { return m_proxy_server; }
    uint16_t proxyPort(void) const { return m_proxy_port; }
    const QString& proxyPassword(void) const { return m_proxy_password; }
    
    const QString& micAudioDevice(void) const { return m_mic_audio_device; }
    const QString& spkrAudioDevice(void) const { return m_spkr_audio_device; }
    bool useFullDuplex(void) const { return m_use_full_duplex; }
    const QString& connectSound(void) const { return m_connect_sound; }
    int cardSampleRate(void) const { return m_card_sample_rate; }
    
    const QString& chatEncoding(void) const
    {
      return encodings[m_chat_encoding].name;
    }
    
    void setBookmarks(const QStringList &bookmarks);
    const QStringList& bookmarks(void) const { return m_bookmarks; }
    
    QSize mainWindowSize(void) const { return m_main_window_size; }
    void setMainWindowSize(QSize size);
    QList<int> vSplitterSizes() const { return m_vsplitter_sizes; }
    void setVSplitterSizes(QList<int> sizes);
    QList<int> hSplitterSizes() const { return m_hsplitter_sizes; }
    void setHSplitterSizes(QList<int> sizes);
    QList<int> stationViewColSizes() const { return m_stn_view_col_sizes; }
    void setStationViewColSizes(QList<int> sizes);
    QList<int> incomingViewColSizes() const { return m_incm_view_col_sizes; }
    void setIncomingViewColSizes(QList<int> sizes);
    
    void setVoxParams(bool enabled, int threshold_db, int delay_ms);
    bool voxEnabled(void) const { return m_vox_enabled; }
    int voxThreshold(void) const { return m_vox_threshold; }
    int voxDelay(void) const { return m_vox_delay; }
    
    void setConnectToIp(const QString &host);
    const QString& connectToIp(void) const { return m_connect_to_ip; }
    
    sigc::signal<void> configurationUpdated;
    
  protected:
    
  private:
    class Encoding
    {
      public:
	Encoding(const QString& name="", const QString& language="")
	  : name(name), language(language) {}
	QString name;
	QString language;
    };

    static Settings *       the_instance;
    
    SettingsDialog *        dialog;
    QVector<Encoding>       encodings;
    
    QString   	            m_callsign;
    QString   	            m_password;
    QString   	            m_name;
    QString   	            m_location;
    QString   	            m_info;
    
    QString   	            m_directory_servers;
    int       	            m_list_refresh_time;
    bool      	            m_start_as_busy;
    QString   	            m_bind_address;
    bool                    m_proxy_enabled;
    QString                 m_proxy_server;
    uint16_t                m_proxy_port;
    QString                 m_proxy_password;

    QString   	            m_mic_audio_device;
    QString   	            m_spkr_audio_device;
    bool      	            m_use_full_duplex;
    QString   	            m_connect_sound;
    int                     m_card_sample_rate;
    
    int		            m_chat_encoding;

    QStringList             m_bookmarks;
    
    QSize     	            m_main_window_size;
    QList<int>              m_vsplitter_sizes;
    QList<int>              m_hsplitter_sizes;
    QList<int>		    m_stn_view_col_sizes;
    QList<int>		    m_incm_view_col_sizes;
    
    bool      	            m_vox_enabled;
    int       	            m_vox_threshold;
    int       	            m_vox_delay;
    
    QString   	            m_connect_to_ip;
    
    Settings(void);
    
};  /* class Settings */


//} /* namespace */

#endif /* SETTINGS_INCLUDED */



/*
 * This file has not been truncated
 */

