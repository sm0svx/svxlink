/**
@file	 Voter.h
@brief   This file contains a class that implement a receiver voter
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-18

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2012 Tobias Blomberg / SM0SVX

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


#ifndef VOTER_INCLUDED
#define VOTER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <CppStdCompat.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"
#include "Macho.hpp"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
  class AudioSelector;
  class Pty;
};


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
@brief	An Rx class that implement a receiver voter
@author Tobias Blomberg
@date   2005-04-18

This class implements a receiver voter. A voter is a device that choose the
best receiver from a pool of receivers tuned to the same frequency. This
make it possible to cover a larger geographical area with a radio system.
*/
class Voter : public Rx
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg   The Config object to read configuration data from
     * @param 	name  The name of the receiver configuration section
     */
    Voter(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~Voter(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
    
    /**
     * @brief   Set the verbosity level of the receiver
     * @param   verbose Set to \em false to keep the rx from printing things
     */
    void setVerbose(bool verbose) { m_verbose = verbose; }

    /**
     * @brief 	Set the mute state for this receiver
     * @param 	mute_state The mute state to set for this receiver
     */
    void setMuteState(MuteState new_mute_state);
    
    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	thresh The detection threshold in dB SNR
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    bool addToneDetector(float fq, int bw, float thresh, int required_duration);
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    float signalStrength(void) const;
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    int sqlRxId(void) const;
    
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    void reset(void);
    
  protected:
    
  private:
    static CONSTEXPR float    DEFAULT_HYSTERESIS             = 1.5f;
    static CONSTEXPR unsigned DEFAULT_VOTING_DEALAY          = 0;
    static CONSTEXPR unsigned DEFAULT_SQL_CLOSE_REVOTE_DELAY = 500;
    static CONSTEXPR unsigned DEFAULT_REVOTE_INTERVAL        = 1000;
    static CONSTEXPR unsigned DEFAULT_RX_SWITCH_DELAY        = 500;
    
    static CONSTEXPR unsigned MAX_VOTING_DELAY               = 5000;
    static CONSTEXPR unsigned MAX_BUFFER_LENGTH              = MAX_VOTING_DELAY;
    static CONSTEXPR float    MAX_HYSTERESIS                 = 2.0f;
    static CONSTEXPR unsigned MAX_SQL_CLOSE_REVOTE_DELAY     = 3000;
    static CONSTEXPR unsigned MIN_REVOTE_INTERVAL            = 100;
    static CONSTEXPR unsigned MAX_REVOTE_INTERVAL            = 60000;
    static CONSTEXPR unsigned MAX_RX_SWITCH_DELAY            = 3000;
	void commandHandler(const void *buf, size_t count);
	void setRxStatus(char *name, bool status);
    class SatRx;

    TOPSTATE(Top)
    {
      typedef std::list<sigc::slot<void> > SlotList;
      
	// Top state variables (visible to all substates)
      struct Box {
	Box(void)
	  : voting_delay(DEFAULT_VOTING_DEALAY), hysteresis(DEFAULT_HYSTERESIS),
	    sql_close_revote_delay(DEFAULT_SQL_CLOSE_REVOTE_DELAY),
	    rx_switch_delay(DEFAULT_RX_SWITCH_DELAY),
	    revote_interval(DEFAULT_REVOTE_INTERVAL), voter(0), best_srx(0),
	    mute_state(MUTE_ALL), task_timer(0), event_timer(0)
	{
	  event_timer.setEnable(false);
	}
	
	unsigned	voting_delay;
	float		hysteresis;
	unsigned	sql_close_revote_delay;
	unsigned	rx_switch_delay;
	unsigned	revote_interval;
	Voter		*voter;
	SatRx		*best_srx;
        Rx::MuteState   mute_state;
	Async::Timer	*task_timer;
	SlotList	task_list;
	Async::Timer	event_timer;
      };

      STATE(Top)

      void setVotingDelay(unsigned delay_ms)
      {
        box().voting_delay = delay_ms;
      }
      unsigned votingDelay(void) { return box().voting_delay; }
      void setHysteresis(float hysteresis) { box().hysteresis = hysteresis; }
      float hysteresis(void) { return box().hysteresis; }
      void setSqlCloseRevoteDelay(unsigned delay_ms)
      {
	box().sql_close_revote_delay = delay_ms;
      }
      unsigned sqlCloseRevoteDelay(void)
      {
        return box().sql_close_revote_delay;
      }
      void setRxSwitchDelay(unsigned delay_ms)
      {
        box().rx_switch_delay = delay_ms;
      }
      unsigned rxSwitchDelay(void) { return box().rx_switch_delay; }
      void setRevoteInterval(unsigned interval_ms)
      {
	box().revote_interval = interval_ms;
      }
      unsigned revoteInterval(void) { return box().revote_interval; }
      
	// Machine's event protocol
      virtual void timerExpired(void) { }
      virtual void setMuteState(Rx::MuteState new_mute_state);
      virtual void reset(void);
      virtual void satSquelchOpen(SatRx *srx, bool is_open);
      virtual void satSignalLevelUpdated(SatRx *srx, float siglev);
      virtual float signalStrength(void) { return -100.0; }
      virtual int sqlRxId(void) { return 0; }
      virtual SatRx *activeSrx(void) { return 0; }
      
      protected:
	Voter &voter(void) { return *box().voter; }
	SatRx *bestSrx(void) { return box().best_srx; }
	bool muteState(void) { return box().mute_state; }
	void runTask(sigc::slot<void> task);
	void taskTimerExpired(Async::Timer *t);
	void startTimer(unsigned time_ms);
	void stopTimer(void);
	
      private:
	void entry() {}
	void init(Voter *voter);
	void exit(void);
	
	void eventTimerExpired(Async::Timer *t);
    };

    SUBSTATE(Muted, Top)
    {
      STATE(Muted)
      
      virtual void setMuteState(Rx::MuteState new_mute_state);
      
      private:
	void entry(void);
	
	void doUnmute(void);
    };

    SUBSTATE(Idle, Top)
    {
      STATE(Idle)

      virtual void satSquelchOpen(SatRx *srx, bool is_open);

      private:
	void entry(void);
    };

    SUBSTATE(VotingDelay, Top)
    {
      STATE(VotingDelay)

      virtual void timerExpired(void);
      virtual void satSquelchOpen(SatRx *srx, bool is_open);

      private:
	void entry(void);
	void exit(void);
	
    };

    SUBSTATE(ActiveRxSelected, Top) {
      struct Box
      {
	Box(void) : active_srx(0) {}
	SatRx *active_srx;
      };
      
      STATE(ActiveRxSelected)

      virtual void setMuteState(Rx::MuteState new_mute_state);
      virtual int sqlRxId(void);
      virtual SatRx *activeSrx(void) { return box().active_srx; }

      protected:
	virtual void changeActiveSrx(SatRx *srx);
	
      private:
	void init(SatRx *srx);
	void exit(void);
    };

    SUBSTATE(SquelchOpen, ActiveRxSelected) {
      STATE(SquelchOpen)

      virtual void satSquelchOpen(SatRx *srx, bool is_open);
      virtual float signalStrength(void);

      protected:
	virtual void changeActiveSrx(SatRx *srx);
	
      private:
	void entry(void);
	void init(void);
	void exit(void);
    };

    SUBSTATE(SqlCloseWait, ActiveRxSelected)
    {
      STATE(SqlCloseWait)

      virtual void timerExpired(void);
      virtual void satSquelchOpen(SatRx *srx, bool is_open);
      
      private:
	void entry(void);
	void exit(void);
    };

    SUBSTATE(Receiving, SquelchOpen)
    {
      STATE(Receiving)
      
      virtual void timerExpired(void);

      private:
	void entry(void);
	void exit(void);
    };

    SUBSTATE(SwitchActiveRx, SquelchOpen)
    {
      struct Box
      {
	Box(void) : switch_to_srx(0) {}
	SatRx *switch_to_srx;
      };
      
      STATE(SwitchActiveRx)

      virtual void setMuteState(Rx::MuteState new_mute_state);

      protected:
	virtual void timerExpired(void);

      private:
	void entry(void);
	void init(SatRx *srx);
	void exit(void);
    };
    
    typedef std::list<Macho::IEvent<Top>*> EventQueue;
    
    Async::Config     	  &cfg;
    std::list<SatRx *>	  rxs;
    bool	      	  m_verbose;
    Async::AudioSelector  *selector;
    Macho::Machine<Top>   sm;
    bool		  is_processing_event;
    EventQueue		  event_queue;
    
    void dispatchEvent(Macho::IEvent<Top> *event);
    void satSquelchOpen(bool is_open, SatRx *rx);
    void satSignalLevelUpdated(float siglev, SatRx *srx);
    void muteAllBut(SatRx *srx, Rx::MuteState mute_state);
    void muteAll(Rx::MuteState mute_state) { muteAllBut(0, mute_state); }
    void unmuteAll(void);
    void resetAll(void);
    void printSquelchState(void);
    void setRxStatus(void);
    SatRx *findBestRx(void) const;
	Async::Pty *voter_pty;

};  /* class Voter */


//} /* namespace */

#endif /* VOTER_INCLUDED */



/*
 * This file has not been truncated
 */
