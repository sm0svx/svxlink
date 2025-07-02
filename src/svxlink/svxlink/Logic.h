/**
@file	 Logic.h
@brief   The logic core of the SvxLink Server application
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

This is the logic core of the SvxLink Server application. This is where
everything is tied together. It is also the base class for implementing
specific logic core classes (e.g. SimplexLogic and RepeaterLogic).

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2025  Tobias Blomberg / SM0SVX

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


#ifndef LOGIC_INCLUDED
#define LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <list>
#include <map>
#include <vector>
#include <stdint.h>

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <LocationInfo.h>
#include <AsyncAtTimer.h>
#include <AsyncTimer.h>
#include <Tx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LogicBase.h"
#include "CmdParser.h"



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class AudioMixer;
  class AudioAmp;
  class AudioSelector;
  class AudioSplitter;
  class AudioValve;
  class AudioStreamStateDetector;
  class AudioRecorder;
  class AudioSource;
  class AudioSink;
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

class Rx;
class MsgHandler;
class Module;
class EventHandler;
class Command;
class QsoRecorder;
class DtmfDigitHandler;


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
@brief	This class implements the functions in common for RF logic cores
@author Tobias Blomberg
@date   2004-03-23

This class is used as the base class for all logic cores that implement "RF"
behaviour. That is, logic cores which are primarily intended to operate a radio
channel, e.g. RepeaterLogic and SimplexLogic.
*/
class Logic : public LogicBase
{
  public:

    /**
     * @brief 	Default constructor
     */
    Logic(void);

    /**
     * @brief   Initialize the logic core
     * @param   cfgobj      A previously initialized configuration object
     * @param   plugin_name The name of the logic core
     * @return  Returns \em true on success or \em false on failure
     */
    virtual bool initialize(Async::Config& cfgobj,
                            const std::string& plugin_name) override;

    virtual void processEvent(const std::string& event, const Module *module=0);
    void setEventVariable(const std::string& name, const std::string& value);
    virtual void playFile(const std::string& path);
    virtual void playSilence(int length);
    virtual void playTone(int fq, int amp, int len);
    virtual void playDtmf(const std::string& digits, int amp, int len);
    void recordStart(const std::string& filename, unsigned max_time);
    void recordStop(void);
    void injectDtmf(const std::string& digits, int len);

    virtual bool activateModule(Module *module);
    virtual void deactivateModule(Module *module);
    Module *activeModule(void) const { return active_module; }
    Module *findModule(int id);
    Module *findModule(const std::string& name);
    std::list<Module*> moduleList(void) const { return modules; }

    const std::string& callsign(void) const { return m_callsign; }

    Rx &rx(void) const { return *m_rx; }
    Tx &tx(void) const { return *m_tx; }

    void sendDtmf(const std::string& digits);

    void injectDtmfDigit(char digit, int duration_ms)
    {
      dtmfDigitDetectedP(digit, duration_ms);
    }

    void setReportEventsAsIdle(bool idle) { report_events_as_idle = idle; }

    bool isWritingMessage(void);
    virtual void setOnline(bool online);

    virtual Async::AudioSink *logicConIn(void);
    virtual Async::AudioSource *logicConOut(void);

    virtual void remoteCmdReceived(LogicBase* src_logic,
                                   const std::string& cmd);
    virtual void remoteReceivedTgUpdated(LogicBase *src_logic, uint32_t tg);


    CmdParser *cmdParser(void) { return &cmd_parser; }

    void processMacroCmd(const std::string &macro_cmd);

  protected:
    /**
     * @brief 	Destructor
     */
    virtual ~Logic(void) override;

    virtual void squelchOpen(bool is_open);
    virtual void allMsgsWritten(void);
    virtual void dtmfDigitDetected(char digit, int duration);
    virtual void audioStreamStateChange(bool is_active, bool is_idle);
    virtual bool getIdleState(void) const;
    virtual void transmitterStateChange(bool is_transmitting);
    virtual void selcallSequenceDetected(std::string sequence);
    virtual void dtmfCtrlPtyCmdReceived(const void *buf, size_t count);
    virtual void commandPtyCmdReceived(const void *buf, size_t count);

    void clearPendingSamples(void);
    void enableRgrSoundTimer(bool enable);
    void rxValveSetOpen(bool do_open);
    void rptValveSetOpen(bool do_open);
    void checkIdle(void);
    void setTxCtrlMode(Tx::TxCtrlMode mode);

  private:

    typedef enum
    {
      TX_CTCSS_ALWAYS=1, TX_CTCSS_SQL_OPEN=2, TX_CTCSS_LOGIC=4,
      TX_CTCSS_MODULE=8, TX_CTCSS_ANNOUNCEMENT=16
    } TxCtcssType;

    Rx	      	      	      	    *m_rx;
    Tx	      	      	      	    *m_tx;
    MsgHandler	      	      	    *msg_handler;
    Module    	      	      	    *active_module;
    std::list<Module*>	      	    modules;
    std::string       	      	    m_callsign;
    std::list<std::string>    	    cmd_queue;
    Async::Timer      	      	    exec_cmd_on_sql_close_timer;
    Async::Timer      	      	    rgr_sound_timer;
    float       	      	    report_ctcss;
    std::map<int, std::string>	    macros;
    EventHandler      	      	    *event_handler;
    Async::AudioSelector      	    *logic_con_out;
    Async::AudioSplitter	    *logic_con_in;
    CmdParser 	      	      	    cmd_parser;
    Async::AtTimer      	    every_minute_timer;
    Async::AtTimer      	    every_second_timer;
    Async::AudioRecorder  	    *recorder;
    Async::AudioMixer	      	    *tx_audio_mixer;
    Async::AudioAmp   	      	    *fx_gain_ctrl;
    Async::AudioSelector      	    *tx_audio_selector;
    Async::AudioSplitter      	    *rx_splitter;
    Async::AudioValve 	      	    *rx_valve;
    Async::AudioValve 	      	    *rpt_valve;
    Async::AudioSelector      	    *audio_from_module_selector;
    Async::AudioSplitter      	    *audio_to_module_splitter;
    Async::AudioSelector      	    *audio_to_module_selector;
    Async::AudioStreamStateDetector *state_det;
    int                             fx_gain_normal;
    int                             fx_gain_low;
    unsigned       	      	    long_cmd_digits;
    std::string       	      	    long_cmd_module;
    bool      	      	      	    report_events_as_idle;
    QsoRecorder                     *qso_recorder;
    uint8_t			    tx_ctcss;
    uint8_t			    tx_ctcss_mask;
    std::string                     sel5_from;
    std::string                     sel5_to;
    Tx::TxCtrlMode                  currently_set_tx_ctrl_mode;
    bool                            is_online;
    std::string                     online_cmd;
    DtmfDigitHandler                *dtmf_digit_handler;
    Async::Pty                      *state_pty;
    Async::Pty                      *dtmf_ctrl_pty;
    std::map<uint16_t, uint32_t>    m_ctcss_to_tg;
    Async::Pty                      *command_pty;
    Async::Timer                    m_ctcss_to_tg_timer;
    float                           m_ctcss_to_tg_last_fq;
    std::string                     m_macro_prefix                {"D"};

    void loadModules(void);
    void loadModule(const std::string& module_name);
    void unloadModules(void);
    void processCommandQueue(void);
    void processCommand(const std::string &cmd, bool force_core_cmd=false);
    void putCmdOnQueue(void);
    void sendRgrSound(void);
    void timeoutNextMinute(void);
	void timeoutNextSecond(void);
    void everyMinute(Async::AtTimer *t);
	void everySecond(Async::AtTimer *t);
    void dtmfDigitDetectedP(char digit, int duration);
    void cleanup(void);
    void updateTxCtcss(bool do_set, TxCtcssType type);
    void logicConInStreamStateChanged(bool is_active, bool is_idle);
    void audioFromModuleStreamStateChanged(bool is_active, bool is_idle);
    void onPublishStateEvent(const std::string &event_name,
                             const std::string &msg);
    void detectedTone(float fq);
    void cfgUpdated(const std::string& section, const std::string& tag);
    bool getConfigValue(const std::string& section, const std::string& tag,
                        std::string& value);

};  /* class Logic */


//} /* namespace */

#endif /* LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

