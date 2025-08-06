/**
@file	 AnnounceLogic.cpp
@brief   Contains a Announce logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2022-06-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AnnounceLogic.h"
#include "EventHandler.h"
#include "MsgHandler.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/
#define ANNOUNCELOGIC_VERSION "24072023"



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

extern "C" {
  LogicBase* construct(void) { return new AnnounceLogic; }
}



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

AnnounceLogic::AnnounceLogic(void)
  : m_logic_con_out(0), m_logic_con_in(0), report_events_as_idle(false),
    pre_interval(5), day_of_week(1), start_prenotify_before(20), cnt(0)
{
} /* AnnounceLogic::AnnounceLogic */



bool AnnounceLogic::initialize(Async::Config& cfgobj, const string& logic_name)
{

  // outgoing audio adapter for the announcements
  m_logic_con_out = new Async::AudioSelector;

  /* dummy, not used */
  m_logic_con_in = new Async::AudioPassthrough;

   // Init this LogicBase
  if (!LogicBase::initialize(cfgobj, logic_name))
  {
    cout << name() << ":*** ERROR initializing AnnounceLogic." << endl;
    return false;
  }

  string value;
  if (cfg().getValue(name(), "START_PRENOTIFICATION_MINUTES_BEFORE", value))
  {
    start_prenotify_before = atoi(value.c_str());
    if (start_prenotify_before > 60 || start_prenotify_before < 0)
    {
      start_prenotify_before = 60;
    }
  }

  if (cfg().getValue(name(), "PRENOTIFICATION_INTERVAL", value))
  {
    pre_interval = atoi(value.c_str());
    if (pre_interval < 1 || pre_interval > start_prenotify_before)
    {
      pre_interval = 5;
    }
  }

  if (cfg().getValue(name(), "DAY_OF_WEEK", value))
  {
    day_of_week = atoi(value.c_str());
  }
  else
  {
    cout << "+++ WARNING: " << name() << "/DAY_OF_WEEK not defined" << endl;
  }

  if (!cfg().getValue(name(), "DAYS_IN_MONTH", value))
  {
    cout << "*** WARNING: " << name() << "/DAYS_IN_MONTH not defined. "
         << "You must define at least on week, e.g. DAYS_IN_MONTH=1,3"
         << "Setting to 1." << endl;
    days.push_back("1");
  }
  else
  {
    size_t t = SvxLink::splitStr(days, value, ",");
    if (t <= 0)
    {
      cout << "*** WARNING: " << name() << "/DAYS_IN_MONTH has wrong format."
           << "Setting to 1." << endl;
      days.push_back("1");
    }
  }

  if (cfg().getValue(name(), "HOUR_OF_QST", value))
  {
    hour_of_qst = atoi(value.c_str());
    if (hour_of_qst < 0 || hour_of_qst > 23)
    {
      cout << "*** ERROR: " << name()
           << "/HOUR_OF_QST must be in the range of 0 to 23!" << endl;
      return false;
    }
  }
  else
  {
    cout << "*** ERROR: " << name() << "/HOUR_OF_QST not set, giving up."
        << endl;
    return false;
  }

  if (cfg().getValue(name(), "MINUTE_OF_QST", value))
  {
    minute_of_qst = atoi(value.c_str());
    if (minute_of_qst < 0 || minute_of_qst > 59)
    {
      cout << "*** ERROR: " << name()
           << "/MINUTE_OF_QST must be in the range of 0 to 59!" << endl;
      return false;
    }
  }
  else
  {
    cout << "*** ERROR: " << name()
         << "/MINUTE_OF_QST not set, giving up." << endl;
    return false;
  }

  string event_handler_str;
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str))
  {
    cerr << name() << ":*** ERROR: Config variable EVENT_HANDLER not set"
         << endl;
    return false;
  }
    // Create the message handler for announcements
  msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  msg_handler->allMsgsWritten.connect(mem_fun(*this, &AnnounceLogic::allMsgsWritten));

  m_logic_con_out->addSource(msg_handler);
  m_logic_con_out->enableAutoSelect(msg_handler, 0);
  m_logic_con_out->setFlushWait(msg_handler, false);

  if (!LinkManager::hasInstance())
  {
    cerr << name() << ":*** ERROR: You have to configure and link together "
         << "one more logi than just the AnounceLogic." << endl;
    return false;
  }

  event_handler = new EventHandler(event_handler_str, name());
  event_handler->playFile.connect(mem_fun(*this, &AnnounceLogic::playFile));
  event_handler->playSilence.connect(mem_fun(*this, &AnnounceLogic::playSilence));
  event_handler->playTone.connect(mem_fun(*this, &AnnounceLogic::playTone));
  event_handler->playDtmf.connect(mem_fun(*this, &AnnounceLogic::playDtmf));
  event_handler->getConfigValue.connect(
          sigc::mem_fun(*this, &AnnounceLogic::getConfigValue));
  event_handler->setVariable("logic_name", name().c_str());
  event_handler->setVariable("logic_type", type());
  event_handler->processEvent("namespace eval " + name() + " {}");

  if (!event_handler->initialize())
  {
    cout << "*** ERROR: " << name() << " initializing eventhandler in " << endl;
    return false;
  }

  every_minute_timer.setExpireOffset(100);
  every_minute_timer.expired.connect(mem_fun(*this, &AnnounceLogic::everyMinute));
  timeoutNextMinute();
  every_minute_timer.start();

  processEvent("startup");

  cout << name() << ": Version " << ANNOUNCELOGIC_VERSION << " started." << endl;

  return true;
} /* AnnounceLogic::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

AnnounceLogic::~AnnounceLogic(void)
{
  delete event_handler;  event_handler = 0;
  delete m_logic_con_in; m_logic_con_in = 0;
  delete msg_handler;    msg_handler = 0;
} /* AnnounceLogic::~AnnounceLogic */



void AnnounceLogic::allMsgsWritten(void)
{
} /* AnnounceLogic::allMsgsWritten */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void AnnounceLogic::processEvent(const string& event)
{
  msg_handler->begin();
  event_handler->processEvent(name() + "::" + event);
  msg_handler->end();
} /* AnnounceLogic::processEvent */


void AnnounceLogic::playFile(const string& path)
{
  msg_handler->playFile(path, report_events_as_idle);
} /* AnnounceLogic::playFile */


void AnnounceLogic::playSilence(int length)
{
  msg_handler->playSilence(length, report_events_as_idle);
} /* AnnounceLogic::playSilence */


void AnnounceLogic::playTone(int fq, int amp, int len)
{
  msg_handler->playTone(fq, amp, len, report_events_as_idle);
} /* AnnounceLogic::playTone */


void AnnounceLogic::playDtmf(const std::string& digits, int amp, int len)
{
  msg_handler->playDtmf(digits[0], amp, len, report_events_as_idle);
} /* AnnounceLogic::playDtmf */


void AnnounceLogic::timeoutNextMinute(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm tm;
  localtime_r(&tv.tv_sec, &tm);
  tm.tm_min += 1;
  tm.tm_sec = 0;
  every_minute_timer.setTimeout(tm);
} /* AnnounceLogic::timeoutNextMinute */


void AnnounceLogic::everyMinute(AtTimer *t)
{
  processEvent("every_minute");
  timeoutNextMinute();

  // time now
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm tm;
  localtime_r(&tv.tv_sec, &tm);

  // pre time
  struct timeval pre_not_time = tv;
  pre_not_time.tv_sec += (start_prenotify_before * 60 - cnt * pre_interval * 60);
  struct tm tn;
  localtime_r(&pre_not_time.tv_sec, &tn);

  // 1) check if qst announcement
  if  (tm.tm_hour == hour_of_qst && tm.tm_min == minute_of_qst &&
       check_week_of_month(tm))
  {
    announceQst();
    cnt = 0;
  }      // 2) check if pre notification
  else if (tn.tm_hour == hour_of_qst && tn.tm_min == minute_of_qst &&
      check_week_of_month(tn))
  {
    prenotification();
    cnt++;
  }

} /* AnnounceLogic::everyMinute */


void AnnounceLogic::prenotification(void)
{
  stringstream ss;
  ss << "announce_prenotification";
  processEvent(ss.str());
} /* AnnounceLogic::announceTimer */


void AnnounceLogic::announceQst(void)
{
  stringstream ss;
  ss << "announce_qst";
  processEvent(ss.str());
} /* AnnounceLogic::announceTimer */


bool AnnounceLogic::check_week_of_month(struct tm t)
{
  for (std::vector<std::string>::iterator it = days.begin();
        it != days.end(); it++)
  {
    int z = atoi((*it).c_str());
    if (t.tm_wday == day_of_week && 
          (t.tm_mday <= z * 7 && t.tm_mday >= (z-1) * 7))
    {
      return true;
    }
  }
  return false;
} /* AnnounceLogic::check_week_of_month */


bool AnnounceLogic::getConfigValue(const std::string& section,
                                   const std::string& tag,
                                   std::string& value)
{
  return cfg().getValue(section, tag, value, true);
} /* AnnounceLogic::getConfigValue */


/*
 * This file has not been truncated
 */

