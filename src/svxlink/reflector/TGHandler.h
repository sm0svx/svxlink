/**
@file   TGHandler.h
@brief  A class for handling talk groups
@author Tobias Blomberg / SM0SVX
@date   2019-07-25

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

#ifndef TG_HANDLER_INCLUDED
#define TG_HANDLER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>
#include <set>
#include <sigc++/sigc++.h>
#include <sys/time.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorClient.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



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

/**
@brief  Handle talk groups
@author Tobias Blomberg / SM0SVX
@date   2019-07-25

This class is responsible for keeping track of all talk groups that are used in
the system.
*/
class TGHandler : public sigc::trackable
{
  public:
    typedef std::set<ReflectorClient*> ClientSet;

    static TGHandler* instance(void)
    {
      static TGHandler *tg_handler = new TGHandler;
      return tg_handler;
    }

    /**
     * @brief   Default constructor
     */
    TGHandler(void);

    /**
     * @brief   Destructor
     */
    ~TGHandler(void);

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    void setConfig(const Async::Config* cfg) { m_cfg = cfg; }

    unsigned sqlTimeout(void) const { return m_sql_timeout; }
    void setSqlTimeout(unsigned sql_timeout) { m_sql_timeout = sql_timeout; }

    unsigned sqlTimeoutBlocktime(void) const
    {
      return m_sql_timeout_blocktime;
    }
    void setSqlTimeoutBlocktime(unsigned sql_timeout_blocktime);

    bool switchTo(ReflectorClient *client, uint32_t tg);

    void removeClient(ReflectorClient* client);

    const ClientSet& clientsForTG(uint32_t tg) const;

    void setTalkerForTG(uint32_t tg, ReflectorClient* client);

    ReflectorClient* talkerForTG(uint32_t tg) const;

    uint32_t TGForClient(ReflectorClient* client);

    bool allowTgSelection(ReflectorClient *client, uint32_t tg);

    bool showActivity(uint32_t tg) const;

    bool isRestricted(uint32_t tg) const;

    sigc::signal<void, uint32_t,
      ReflectorClient*, ReflectorClient*> talkerUpdated;

    sigc::signal<void, uint32_t> requestAutoQsy;

  private:
    static const time_t TALKER_AUDIO_TIMEOUT = 3; // Max three seconds gap

    struct TGInfo
    {
      uint32_t          id;
      ClientSet         clients;
      ReflectorClient*  talker;
      struct timeval    last_talker_timestamp;
      unsigned          sql_timeout_cnt;
      time_t            auto_qsy_after_s;
      time_t            auto_qsy_time;

      TGInfo(uint32_t tg)
        : id(tg), talker(0), sql_timeout_cnt(0), auto_qsy_after_s(0),
          auto_qsy_time(-1)
      {
        timerclear(&last_talker_timestamp);
      }
    };
    typedef std::map<uint32_t, TGInfo*>               IdMap;
    typedef std::map<const ReflectorClient*, TGInfo*> ClientMap;

    const Async::Config*  m_cfg;
    IdMap                 m_id_map;
    ClientMap             m_client_map;
    Async::Timer          m_timeout_timer;
    unsigned              m_sql_timeout;
    unsigned              m_sql_timeout_blocktime;

    TGHandler(const TGHandler&);
    TGHandler& operator=(const TGHandler&);
    void checkTimers(Async::Timer *t);
    void removeClientP(TGInfo *tg_info, ReflectorClient* client);
    void printTGStatus(void);
};  /* class TGHandler */


//} /* namespace */

#endif /* TG_HANDLER_INCLUDED */

/*
 * This file has not been truncated
 */
