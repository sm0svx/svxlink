/**
@file	 Logic.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

#include <sigc++/signal_system.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <SigCAudioSource.h>
#include <SigCAudioSink.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AudioSwitchMatrix.h"
#include "CmdParser.h"



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class Timer;
  class SampleFifo;
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
class Tx;
class MsgHandler;
class Module;
class EventHandler;
class Command;
class Recorder;
  

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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2004-03-23

A_detailed_class_description

\include Logic_demo.cpp
*/
class Logic : public SigC::Object
{
  public:
    static void connectLogics(const std::string& l1, const std::string& l2,
      	    int timeout=0);
    static void disconnectLogics(const std::string& l1, const std::string& l2);
    static bool logicsAreConnected(const std::string& l1,
      	    const std::string& l2);

    /**
     * @brief 	Default constuctor
     */
    Logic(Async::Config& cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~Logic(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    
    virtual bool initialize(void);
    
    const std::string& name(void) const { return m_name; }
    
    virtual void processEvent(const std::string& event, const Module *module=0);
    void setEventVariable(const std::string& name, const std::string& value);
    virtual void playFile(const std::string& path);
    virtual void playSilence(int length);
    virtual void playTone(int fq, int amp, int len);
    void recordStart(const std::string& filename);
    void recordStop(void);
    void audioFromModule(float *samples, int count);
    virtual void moduleTransmitRequest(bool do_transmit);

    virtual bool activateModule(Module *module);
    virtual void deactivateModule(Module *module);
    Module *activeModule(void) const { return active_module; }
    Module *findModule(int id);
    Module *findModule(const std::string& name);
    std::list<Module*> moduleList(void) const { return modules; }

    virtual void dtmfDigitDetected(char digit, int duration);
    const std::string& callsign(void) const { return m_callsign; }

    Async::Config &cfg(void) const { return m_cfg; }
    Rx &rx(void) const { return *m_rx; }
    Tx &tx(void) const { return *m_tx; }
    
    void disconnectAllLogics(void);
    
  protected:    
    virtual void squelchOpen(bool is_open);
    //virtual int audioReceived(float *samples, int count) { return count; }
    
    virtual void transmit(bool do_transmit);
    virtual int transmitAudio(float *samples, int count);
    virtual void allMsgsWritten(void);
    virtual void allTxSamplesFlushed(void);
    virtual void remoteLogicTransmitRequest(bool do_tx);
    
    void clearPendingSamples(void);
    void logicTransmitRequest(bool do_transmit);
    void enableRgrSoundTimer(bool enable);

  private:
    static AudioSwitchMatrix  	audio_switch_matrix;
    
    typedef enum
    {
      TX_CTCSS_NEVER, TX_CTCSS_ALWAYS, TX_CTCSS_SQL_OPEN
    } TxCtcssType;
    
    Async::Config     	      	&m_cfg;
    std::string       	      	m_name;
    Rx	      	      	      	*m_rx;
    Tx	      	      	      	*m_tx;
    MsgHandler	      	      	*msg_handler;
    Async::Timer      	      	*write_msg_flush_timer;
    Module    	      	      	*active_module;
    Async::SampleFifo 	      	*module_tx_fifo;
    std::list<Module*>	      	modules;
    std::string       	      	received_digits;
    std::string       	      	m_callsign;
    Async::Timer      	      	*cmd_tmo_timer;
    bool      	      	      	logic_transmit;
    std::list<std::string>    	cmd_queue;
    bool      	      	      	anti_flutter;
    char      	      	      	prev_digit;
    int      	      	      	exec_cmd_on_sql_close;
    Async::Timer      	      	*exec_cmd_on_sql_close_timer;
    Async::Timer      	      	*rgr_sound_timer;
    int       	      	      	rgr_sound_delay;
    float       	      	report_ctcss;
    std::map<int, std::string>	macros;
    EventHandler      	      	*event_handler;
    Async::SigCAudioSource      logic_con_out;
    Async::SigCAudioSink 	logic_con_in;
    bool      	      	      	remote_logic_tx;
    CmdParser 	      	      	cmd_parser;
    Async::Timer      	      	*every_minute_timer;
    Recorder  	      	      	*recorder;
    TxCtcssType       	      	tx_ctcss;
    
    void allModuleSamplesWritten(void);
    void transmitCheck(void);
    void loadModules(void);
    void loadModule(const std::string& module_name);
    void unloadModules(void);
    void cmdTimeout(Async::Timer *t);
    void processCommandQueue(void);
    void processMacroCmd(std::string& cmd);
    void putCmdOnQueue(Async::Timer *t=0);
    void sendRgrSound(Async::Timer *t=0);
    int remoteLogicWriteSamples(float *samples, int len);
    void remoteLogicFlushSamples(void);
    int audioReceived(float *samples, int len);
    void everyMinute(Async::Timer *t);
    
};  /* class Logic */


//} /* namespace */

#endif /* LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

