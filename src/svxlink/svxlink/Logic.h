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

#include <sigc++/signal_system.h>


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
    
    void playMsg(const std::string& msg, const Module *module=0);
    void playNumber(int number);
    void spellWord(const std::string& word);
    void audioFromModule(short *samples, int count);
    void moduleTransmitRequest(bool do_transmit);
    bool activateModule(Module *module);
    void deactivateModule(Module *module);
    Module *findModule(int id);
    std::list<Module*> moduleList(void) const { return modules; }
    virtual void dtmfDigitDetected(char digit);
    const std::string& callsign(void) const { return m_callsign; }

    Async::Config &cfg(void) const { return m_cfg; }
    Rx &rx(void) const { return *m_rx; }
    Tx &tx(void) const { return *m_tx; }
    
  protected:    
    virtual void squelchOpen(bool is_open) {}
    //virtual int audioReceived(short *samples, int count) { return count; }
    
    virtual void transmit(bool do_transmit);
    virtual int transmitAudio(short *samples, int count);
    
    void clearPendingSamples(void);
    
  private:
    Async::Config     	&m_cfg;
    std::string       	m_name;
    Rx	      	      	*m_rx;
    Tx	      	      	*m_tx;
    MsgHandler	      	*msg_handler;
    Async::Timer      	*write_msg_flush_timer;
    Module    	      	*active_module;
    Async::SampleFifo 	*module_tx_fifo;
    std::list<Module*>	modules;
    std::string       	received_digits;
    std::string       	m_callsign;
    Async::Timer      	*cmd_tmo_timer;
    
    void allMsgsWritten(void);
    void allModuleSamplesWritten(void);
    void transmitCheck(void);
    void allTxSamplesFlushed(void);
    void loadModules(void);
    void loadModule(const std::string& module_name);
    void cmdTimeout(Async::Timer *t);


};  /* class Logic */


//} /* namespace */

#endif /* LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

