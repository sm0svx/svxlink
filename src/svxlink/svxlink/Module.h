/**
@file	 Module.h
@brief   This file contains the base class for implementing a SvxLink module.
@author  Tobias Blomberg / SM0SVX
@date	 2005-02-18

This file contains a class for implementing a SvxLink modules. The module
should inherit the Module class and implement the abstract methods.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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


#ifndef MODULE_INCLUDED
#define MODULE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <dlfcn.h>
#include <sigc++/sigc++.h>

#include <string>
#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>



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
};

class Logic;


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
@brief	Base class for implementing a SvxLink module
@author Tobias Blomberg
@date   2005-02-18

This is a base class for implementing a SvxLink module. The module main class
should inherit this class and implement the abstract methods. All communication
to the SvxLink core is done through this class.

\include Template_demo.cpp
*/
class Module : public sigc::trackable, public Async::AudioSink,
      	       public Async::AudioSource
{
  public:
    /**
     * @brief 	The type for the module initialization function
     * @param 	dl_handle The plugin handle
     * @param 	logic 	  The logic core this module belongs to
     * @param 	cfg_name  The name of the configuration file section for this
     *	      	      	  module
     * @return	Must return a newly constructed Module object
     *
     * This is the type of the initialization function that must exist in
     * each module. Its sole purpose is to create a new Module object and
     * return it to the logic core. It must be declared as a pure C function
     * for the logic core module loading core to find it. Below is a standard
     * implementation of such a function.
     *
     * \verbatim
     * extern "C" {
     *   Module *module_init(void *dl_handle, Logic *logic,
     *                       const char *cfg_name)
     *   {
     *     return new ModuleXxx(dl_handle, logic, cfg_name);
     *   }
     * } // extern "C"
     * \endverbatim
     */
    typedef Module* (*InitFunc)(void *dl_handle, Logic *logic,
      	      	      	        const char *cfg_name);
    
    /**
     * @brief 	Default constuctor
     */
    Module(void *dl_handle, Logic *logic, const std::string& cfg_name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~Module(void);
    
    /**
     * @brief 	Initialize the module
     * @return	Return \em true if the initialization was successful
     *	      	or else \em false.
     *
     * This function is called when it is time to initialize the module.
     * As little as possible of the initialization should be done in the
     * constructor. This function will be called once directly after the
     * module has been loaded.
     *
     * This function can but need not be reimplemented by the module. If it
     * is reimplemented, the base class function must be called and its
     * return value checked.
     */
    virtual bool initialize(void);

    /**
     * @brief	Retrieve the version that the module was compiled for
     * @return	Returns a version string representing the version of the
     *		SvxLink core that this module was compiled for.
     */
    virtual const char *compiledForVersion(void) const = 0;

    /**
     * @brief 	Retrieve the name of the config file section for this module
     * @return	Returns the name of the config file section for this module
     */
    const std::string cfgName(void) const { return m_cfg_name; }

    /**
     * @brief 	Retrieve the plugin handle for this module
     * @return	Returns the plugin handle for this module
     */
    void *pluginHandle(void) const { return m_dl_handle; }

    /**
     * @brief 	Retrieve the module id for this module
     * @return	Returns the module id for this module
     */
    int id(void) const { return m_id; }

    /**
     * @brief 	Retrieve the associated logic object
     * @return	Returns the associated logic object
     *
     * Each instance of a module, when loaded, will be associated with a
     * logic core. This function will retrieve the pointer to the object
     * that represents that logic core.
     */
    Logic *logic(void) const { return m_logic; }

    /**
     * @brief 	Activate the module
     *
     * By calling this function, the module will be activated. If the module
     * need to be notified when activation occurrs, the activateInit virtual
     * function should be reimplemented.
     * Note: This function should NOT be used by the module to activate itself.
     * Use the "activateMe" function to do that.
     */
    void activate(void);

    /**
     * @brief 	Deactivate the module
     *
     * By calling this function, the module will be deactivated. If the module
     * need to be notified when deactivation occurrs, the deactivateCleanup
     * virtual function should be reimplemented.
     * Note: This function should NOT be used by the module to deactivate
     * itself. Use the "deactivateMe" function to do that.
     */
    void deactivate(void);

    /**
     * @brief 	Check if the module is active
     * @return	Return \em true if the module is active or else \em false
     *
     * This function is used to check if the module has been activated.
     */
    bool isActive(void) const { return m_is_active; }

    /**
     * @brief 	Check if the module is transmitting
     * @return	Return \em true if the module is transmitting or else \em false
     *
     * This function is used to check if the module has requested that the
     * the transmitter should be on or off.
     */
    //bool isTransmitting(void) const { return m_is_transmitting; }

    /**
     * @brief 	Retrieve the config file object
     * @return	Returns a reference to the config file object
     *
     * This function is used to retrieve a reference to the configuration file
     * object. This reference can be used to read configuration information
     * from the configuration file.
     */
    Async::Config &cfg(void) const;

    /**
     * @brief 	Retrieve the name of the assoicated logic
     * @return	Returns the name of the associated logic
     *
     * Each instance of a module, when loaded, will be associated with a
     * logic core. This function will retrieve the name of the logic core
     * associated with this module.
     */
    const std::string& logicName(void) const;

    /**
     * @brief 	Play the module help message to the transceiver
     *
     * Call this function to order playback of this modules help message to
     * the transceiver frontend.
     */
    void playHelpMsg(void);
    
    /**
     * @brief 	Retrieve the name of the module
     * @return	Returns the name of the module
     */
    virtual const std::string& name(void) const { return m_name; }
    
    /**
     * @brief 	Internal function for module activation
     *
     * This function is called from the "activate" function. It should not be
     * called directly. It can be reimplemented by a module to get a
     * notification when the module is activated.
     */
    virtual void activateInit(void) {}
    
    /**
     * @brief 	Internal function for module deactivation
     *
     * This function is called from the "deactivate" function. It should not be
     * called directly. It can be reimplemented by a module to get a
     * notification when the module is deactivated.
     * This function will only be called if this module is active.
     */
    virtual void deactivateCleanup(void) {}
    
    /**
     * @brief 	Tell the module that a DTMF digit has been received
     * @param 	digit 	  The received digit
     * @param 	duration  The duration of the tone in milliseconds
     * @return	Return \em true if handled or else \em false
     *
     * This function is called by the logic core when a DTMF digit has been
     * detected on the receiver. Modules more often have use for the
     * "dtmfCmdReceived" function. If the return value from this function
     * is \em true, the DTMF digit is considered handled and it is ignored
     * by the logic core.
     * This function will only be called if this module is active.
     */
    virtual bool dtmfDigitReceived(char digit, int duration) { return false; }
    
    /**
     * @brief 	Tell the module that a DTMF command has been received
     * @param 	cmd The received command
     *
     * This function is called by the logic core when a DTMF command has been
     * detected on the receiver. A DTMF command is just a sequence of digits.
     * A-D, *, # is filtered out and has special meanings to the logic core.
     * This function will only be called if this module is active.
     */
    virtual void dtmfCmdReceived(const std::string& cmd) {}
    
    /**
     * @brief 	Tell the module that a DTMF command has been received when idle
     * @param 	cmd The received command
     *
     * This function is called by the logic core when a DTMF command has been
     * detected on the receiver when the module is idle, that is it has not
     * been activated. A command is sent to a non-active module if a command
     * is received that start with the module ID but have more digits than just
     * the module ID. The digits following the module ID is the actual command
     * sent to this function.
     * A DTMF command is just a sequence of digits. A-D, *, # is filtered out
     * and has special meanings to the logic core.
     */
    virtual void dtmfCmdReceivedWhenIdle(const std::string &cmd);

    /**
     * @brief 	Tell the module that the squelch has opened/closed
     * @param 	is_open \em True when the squelch is open or else \em false
     *
     * This function is called by the logic core when the squelch opens
     * or closes.
     * This function will only be called if this module is active.
     */
    virtual void squelchOpen(bool is_open) {}
    
    /**
     * @brief 	Tell the module that all announcement messages has been played
     *
     * This function is called when the logic core has finished playing back
     * messages initiated with one of the playFile, playMsg, playNumber,
     * spellWord or playSilence functions. Note that this function also may
     * be called even if it wasn't this module that initiated the message
     * playing.
     * This function will only be called if this module is active.
     */
    virtual void allMsgsWritten(void) {}
    
    /**
     * @brief 	Order the logic core to process an event
     * @param 	event The name of the event to process
     *
     * This function is called by the module when it wishes to process an
     * event. An event almost always generate sounds to be played over the
     * transmitter. Exactly what to do is specified in a handler script.
     * The handler script is pointed out in the configuration file under
     * XxxLogic/EVENT_HANDLER.
     */
    void processEvent(const std::string& event);
    
    /**
     * @brief 	Order the logic to set a variable in the event handler
     * @param 	name The name of the variable
     * @param 	value The new value of the variable
     *
     * This function is called by the module when it wishes to set a
     * variable in the event handling system. These variables can be read
     * by the event scripts to make announcements more dynamic.
     */
    void setEventVariable(const std::string& name, const std::string& value);
    
    /**
     * @brief 	Order the logic core to play an audio file over the transmitter
     * @param 	path The full path to an audio file
     *
     * This function is called by the module when it wishes to play an audio
     * file over the transmitter. The file must be in raw 16 bit signed word
     * format.
     * The module must be active for this function to do anything.
     */
    void playFile(const std::string& path);
    
    void sendDtmf(const std::string& digits);
    
    
  protected:
    /**
     * @brief 	Called by the module to activate itself
     * @return	Returns \em true if the activation went ok or else \em false is
     *	      	returned
     *
     * This function can be used by the module to activate itself. Be sure to
     * check the return value to make sure that the activation process went ok.
     */
    bool activateMe(void);
    
    /**
     * @brief 	Called by the module to deactivate itself
     *
     * This function should be used by the module to deactivate itself.
     */
    void deactivateMe(void);
    
    /**
     * @brief 	Find a module based on module id
     * @param 	id The id of the module to find
     * @return	Returns a pointer to the module object or 0 if not found
     *
     * This function can be used to find a module based on its module id
     * number.
     */
    Module *findModule(int id);
    
    /**
     * @brief 	Retrieve a list of all loaded modules
     * @return	Returns a list of all loaded modules
     *
     * A module can use this function to retrieve a list of all modules that is
     * loaded into the same logic core as this module.
     */
    std::list<Module*> moduleList(void);
    
    /**
     * @brief 	Tell the logic core if the module is idle or not
     * @param	is_idle \em True if the module is idle or else \em false
     *
     * A module must use this function to tell the logic core if it is idle
     * or not. When the module has been idle for a number of seconds that
     * is specified in the configuration file, it will be deactivated.
     */
    void setIdle(bool is_idle);
    
    /**
     * @brief 	Check if the logic core is idle or not
     * @returns Returns \em true if the logic core is idle or
     *	      	\em false if it's not.
     */
    bool logicIsIdle(void) const;

    /**
     * @brief 	Notify the module that the logic core idle state has changed
     * @param 	is_idle Set to \em true if the logic core is idle or else
     *	      	\em false.
     *
     * This function is called by the logic core when the idle state changes.
     */
    virtual void logicIdleStateChanged(bool is_idle);
    
    /**
     * @brief 	Check if the squelch is open or not
     * @returns Returns \em true if the squelch is open or
     *	      	\em false if it's not.
     */
    bool squelchIsOpen(void);

    /**
     * @brief   Check if the logic core is playing back an announcement
     * @returns Returns \em true if the playback is active or
     *          \em false if it's not.
     */
    bool isWritingMessage(void);

    /**
     * @brief   Called when a configuration variable is updated
     * @param   section The name of the configuration section
     * @param   tag     The name of the configuration variable
     */
    virtual void cfgUpdated(const std::string& section,
                            const std::string& tag);

  private:
    void      	      *m_dl_handle;
    Logic     	      *m_logic;
    int       	      m_id;
    std::string       m_name;
    bool      	      m_is_transmitting;
    sigc::connection  m_logic_idle_con;
    bool      	      m_is_active;
    std::string	      m_cfg_name;
    Async::Timer      *m_tmo_timer;
    bool              m_mute_linking;
    
    void moduleTimeout(Async::Timer *t);

};  /* class Module */


//} /* namespace */

#endif /* MODULE_INCLUDED */



/*
 * This file has not been truncated
 */

