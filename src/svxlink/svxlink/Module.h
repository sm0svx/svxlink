/**
@file	 Module.h
@brief   This file contains the base class for implementing a SvxLink module.
@author  Tobias Blomberg / SM0SVX
@date	 2005-02-18

This file contains a class for implementing a SvxLink modules. The module
should inherit the Module class and implement the abstract methods.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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


#ifndef MODULE_INCLUDED
#define MODULE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <dlfcn.h>
#include <sigc++/signal_system.h>

#include <string>
#include <list>


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
class Module : public SigC::Object
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
    bool isTransmitting(void) const { return m_is_transmitting; }

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
     * @brief 	Play the module name to the transceiver
     *
     * Call this function to order playback of this modules name to the
     * transceiver frontend.
     */
    void playModuleName(void);

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
    virtual const char *name(void) const = 0;
    
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
     * @param 	digit The received digit
     *
     * This function is called by the logic core when a DTMF digit has been
     * detected on the receiver. Modules more often have use for the
     * "dtmfCmdReceived" function.
     * This function will only be called if this module is active.
     */
    virtual void dtmfDigitReceived(char digit) {}
    
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
     * @brief 	Tell the module that the squelch has opened/closed
     * @param 	is_open \em True when the squelch is open or else \em false
     *
     * This function is called by the logic core when the squelch opens
     * or closes.
     * This function will only be called if this module is active.
     */
    virtual void squelchOpen(bool is_open) {}
    
    /**
     * @brief 	Tell the module that audio has been received from the receiver
     * @param 	samples A buffer containing the samples
     * @param 	count 	The number of samples received
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is called by the logic core when some audio has been
     * received from the radio receiver. The samples are 16 bit signed samples.
     * This function will only be called if this module is active.
     */
    virtual int audioFromRx(short *samples, int count) { return count; }
    
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
     * @brief 	Tell the module to report its state on the radio channel
     *
     * This function is called by the logic core when it wishes the module to
     * report its state on the radio channel. Typically this is done when a
     * manual identification has been triggered by the user by sending a "*".
     * For example, use one of the "playXxx" functions to announce the state.
     * This function will only be called if this module is active.
     */
    virtual void reportState(void) {}
    
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
    
    /**
     * @brief 	Order the logic core to play an audio message over the
     *	      	transmitter
     * @param 	msg The message to transmit
     *
     * Calling this function will trigger a playback of an audio file over
     * the transmitter. The file should be located under the
     * directory pointed out by the SOUNDS config variable. Under the SOUNDS
     * directory there is a subdirectory for each module where the logic core
     * will first look for the file. If no file can be found there, the logic
     * core will look under the "Default" subdirectory. If not found there,
     * a warning will be issued.
     * The module must be active for this function to do anything.
     */
    void playMsg(const std::string& msg) const;
    
    /**
     * @brief 	Order the logic core to play a number over the transmitter
     * @param 	number The number to transmit
     *
     * Calling this function will order the logic core to read back the digits
     * in "number" one by one.
     * The module must be active for this function to do anything.
     */
    void playNumber(int number) const;
    
    /**
     * @brief 	Order the logic core to spell a word over the transmitter
     * @param 	word The word to spell
     *
     * Calling this function will order the logic core to read back the letters
     * in "word" one by one. This is done using the phonetic alphabet.
     * The module must be active for this function to do anything.
     */
    void spellWord(const std::string& word) const;
    
    /**
     * @brief 	Order the logic core to play some silence over the transmitter
     * @param 	length The amount of silence specified in milliseconds
     *
     * Calling this function will order the logic core to play a specified
     * number of milliseconds of silence over the transmitter. This can be
     * used to make some room inbetween multiple audio messages.
     * The module must be active for this function to do anything.
     */
    void playSilence(int length) const;
    
  protected:
    
    /**
     * @brief 	Called by the module to send audio over the transmitter
     * @param 	samples A buffer containing samples (16 bit signed)
     * @param 	count The number of samples in the buffer
     * @return	Returns the number samples that have been handled by the
     *	      	logic core
     *
     * This function can be called by the module to transmit audio over the
     * radio channel. The transmitter must be activated/deactivated using the
     * "transmit" function.
     * The module must be active for this function to do anything.
     */
    int audioFromModule(short *samples, int count);
    
    /**
     * @brief 	Called by the module to activate/deactivate the transmitter
     * @param 	tx \em True activates the transmitter and \em false turns it off
     *
     * This function can be called by the module to tell the logic core to
     * activate/deactivate the transmitter. The transmitter will not be
     * deactivated until all audio has been transmitted. Note that this function
     * just gives a hint to the logic core if the module want the transmitter
     * to be active or not. If the logic core thinks that the transmitter should
     * not be active (e.g. TX timeout) or always active (repeater) it will
     * override this function.
     * The module must be active for this function to do anything.
     */
    void transmit(bool tx);
    
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
    

  private:
    void      	      *m_dl_handle;
    Logic     	      *m_logic;
    int       	      m_id;
    bool      	      m_is_transmitting;
    SigC::Connection  m_audio_con;
    SigC::Connection  m_squelch_con;
    bool      	      m_is_active;
    std::string	      m_cfg_name;
    Async::Timer      *m_tmo_timer;
    
    void moduleTimeout(Async::Timer *t);

};  /* class Module */


//} /* namespace */

#endif /* MODULE_INCLUDED */



/*
 * This file has not been truncated
 */

