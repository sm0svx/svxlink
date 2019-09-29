/**
@file   TGHandler.cpp
@brief  A class for handling talk groups
@author Tobias Blomberg / SM0SVX
@date   2019-07-26

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
#include <algorithm>


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

#include "TGHandler.h"


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

TGHandler::TGHandler(void)
  : m_timeout_timer(1000, Async::Timer::TYPE_PERIODIC),
    m_sql_timeout(0), m_sql_timeout_blocktime(60)
{
  m_timeout_timer.expired.connect(
      mem_fun(*this, &TGHandler::checkTalkerTimeout));
} /* TGHandler::TGHandler */


TGHandler::~TGHandler(void)
{
  for (IdMap::iterator it = m_id_map.begin(); it != m_id_map.end(); ++it)
  {
    TGInfo* tg_info = it->second;
    delete tg_info;
  }
} /* TGHandler::~TGHandler */


void TGHandler::setSqlTimeoutBlocktime(unsigned sql_timeout_blocktime)
{
  m_sql_timeout_blocktime = std::max(sql_timeout_blocktime, 1U);
} /* TGHandler::setSqlTimeoutBlocktime */


void TGHandler::switchTo(ReflectorClient *client, uint32_t tg)
{
  TGInfo *tg_info = 0;
  ClientMap::iterator client_map_it = m_client_map.find(client);
  if (client_map_it != m_client_map.end())
  {
    tg_info = client_map_it->second;
    assert(tg_info != 0);
    if (tg_info->id == tg)
    {
      return;
    }
    removeClientP(tg_info, client);
  }

  if (tg > 0)
  {
    IdMap::iterator id_map_it = m_id_map.find(tg);
    if (id_map_it != m_id_map.end())
    {
      tg_info = id_map_it->second;
    }
    else
    {
      tg_info = new TGInfo(tg);
      m_id_map[tg] = tg_info;
    }
    tg_info->clients.insert(client);
    m_client_map[client] = tg_info;
  }

  //printTGStatus();
} /* TGHandler::switchTo */


void TGHandler::removeClient(ReflectorClient* client)
{
  ClientMap::iterator client_map_it = m_client_map.find(client);
  if (client_map_it != m_client_map.end())
  {
    TGInfo* tg_info = client_map_it->second;
    if (tg_info->talker == client)
    {
      setTalkerForTG(tg_info->id, 0);
    }
    removeClientP(tg_info, client);
    //printTGStatus();
  }
} /* TGHandler::removeClient */


const TGHandler::ClientSet& TGHandler::clientsForTG(uint32_t tg) const
{
  static const TGHandler::ClientSet empty_set;
  IdMap::const_iterator id_map_it = m_id_map.find(tg);
  if (id_map_it == m_id_map.end())
  {
    return empty_set;
  }
  return id_map_it->second->clients;
} /* TGHandler::clientsForTG */


void TGHandler::setTalkerForTG(uint32_t tg, ReflectorClient* new_talker)
{
  IdMap::const_iterator id_map_it = m_id_map.find(tg);
  if (id_map_it == m_id_map.end())
  {
    return;
  }
  TGInfo* tg_info = id_map_it->second;
  ReflectorClient* old_talker = tg_info->talker;
  if (new_talker == old_talker)
  {
    gettimeofday(&tg_info->last_talker_timestamp, NULL);
    return;
  }
  tg_info->sql_timeout_cnt = (new_talker != 0) ? m_sql_timeout : 0;
  id_map_it->second->talker = new_talker;
  talkerUpdated(tg, old_talker, new_talker);
  //printTGStatus();
} /* TGHandler::setTalkerForTG */


ReflectorClient* TGHandler::talkerForTG(uint32_t tg) const
{
  IdMap::const_iterator id_map_it = m_id_map.find(tg);
  if (id_map_it == m_id_map.end())
  {
    return 0;
  }
  return id_map_it->second->talker;
} /* TGHandler::talkerForTG */


uint32_t TGHandler::TGForClient(ReflectorClient* client)
{
  ClientMap::iterator client_map_it = m_client_map.find(client);
  if (client_map_it == m_client_map.end())
  {
    return 0;
  }
  return client_map_it->second->id;
} /* TGHandler::TGForClient */


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

void TGHandler::checkTalkerTimeout(Async::Timer *t)
{
  for (IdMap::iterator it = m_id_map.begin(); it != m_id_map.end(); ++it)
  {
    TGInfo *tg_info = it->second;
    assert(tg_info != 0);
    if (tg_info->talker != 0)
    {
      struct timeval now, diff;
      gettimeofday(&now, NULL);
      timersub(&now, &tg_info->last_talker_timestamp, &diff);
      if (diff.tv_sec > TALKER_AUDIO_TIMEOUT)
      {
        cout << tg_info->talker->callsign() << ": Talker audio timeout on TG #"
             << tg_info->id << endl;
        setTalkerForTG(tg_info->id, 0);
      }

      if ((tg_info->sql_timeout_cnt > 0) && (--tg_info->sql_timeout_cnt == 0))
      {
        cout << tg_info->talker->callsign() << ": Talker audio timeout on TG #"
             << tg_info->id << endl;
        tg_info->talker->setBlock(m_sql_timeout_blocktime);
        setTalkerForTG(tg_info->id, 0);
      }
    }
  }
} /* TGHandler::checkTalkerTimeout */


void TGHandler::removeClientP(TGInfo *tg_info, ReflectorClient* client)
{
  assert(tg_info != 0);
  assert(client != 0);
  if (client == tg_info->talker)
  {
    tg_info->talker = 0;
  }
  tg_info->clients.erase(client);
  m_client_map.erase(client);
  if (tg_info->clients.empty())
  {
    m_id_map.erase(tg_info->id);
    delete tg_info;
  }
} /* TGHandler::removeClientP */


void TGHandler::printTGStatus(void)
{
  std::cout << "### ----------- BEGIN ----------------" << std::endl;
  for (IdMap::const_iterator it = m_id_map.begin();
       it != m_id_map.end(); ++it)
  {
    TGInfo *tg_info = it->second;
    std::cout << "### " << tg_info->id << ": ";
    for (ClientSet::const_iterator it = tg_info->clients.begin();
         it != tg_info->clients.end(); ++it)
    {
      ReflectorClient* client = *it;
      if (client == tg_info->talker)
      {
        std::cout << "*";
      }
      std::cout << client->callsign() << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "### ------------ END -----------------" << std::endl;
} /* TGHandler::printTGStatus */


/*
 * This file has not been truncated
 */
