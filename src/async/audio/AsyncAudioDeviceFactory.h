/**
@file	 AsyncAudioDeviceFactory.h
@brief   A class for handling audio device types
@author  Tobias Blomberg / SM0SVX
@date	 2009-12-26

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_DEVICE_FACTORY_INCLUDED
#define ASYNC_AUDIO_DEVICE_FACTORY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>


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



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class AudioDevice;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/**
 * @brief   Register a new audio device type
 * @param   _name The name of the audio device type
 * @param   _class The name of the class that handle the new audio device type
 */
#define REGISTER_AUDIO_DEVICE_TYPE(_name, _class) \
  AudioDevice *create_ ## _class(const std::string& dev_name) \
          { return new _class(dev_name); } \
  static bool _class ## _creator_registered = \
          AudioDeviceFactory::instance().registerCreator(_name, \
                                                         create_ ## _class)



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
@brief	A factory class for audio devices
@author Tobias Blomberg / SM0SVX
@date   2009-12-26

This class is a factory class for creating audio devices. Use the
REGISTER_AUDIO_DEVICE_TYPE macro to register a new audio device type.
New audio device instances are created using the create method.
*/
class AudioDeviceFactory
{
  public:
    typedef AudioDevice* (*CreatorFunc)(const std::string &dev_designator);
    
    /**
     * @brief 	Get the factory singleton instance
     * @return  Returns the factory instance
     */
    static AudioDeviceFactory &instance(void)
    {
      static AudioDeviceFactory the_factory;
      return the_factory;
    }

    /**
     * @brief 	Destructor
     */
    ~AudioDeviceFactory(void);
  
    /**
     * @brief 	Register a new audio device type
     * @param 	name The name of the audio device type (e.g. alsa, oss etc)
     * @param	creator A function that create the AudioDevice object
     * @return	Return \em true on success or else \em false
     */
    bool registerCreator(const std::string &name, CreatorFunc creator);

    /**
     * @brief 	Create a new instance of the specified audio device type
     * @param 	name The audio device type (e.g. alsa, oss etc)
     * @param	dev_name The audio device name (e.g. plughw:0, /dev/dsp etc)
     * @return	Returns an AudioDevice object
     */
    AudioDevice *create(const std::string &name, const std::string &dev_name);

    /**
     * @brief 	List valid device types
     * @return	Returns a space separated list of valid device type names
     */
    std::string validDevTypes(void) const;
    
  protected:
    /**
     * @brief   Default constuctor
     */
    AudioDeviceFactory(void);
  
    
  private:
    typedef std::map<std::string, CreatorFunc> CreatorMap;
    
    CreatorMap creator_map;
    
    AudioDeviceFactory(const AudioDeviceFactory&);
    AudioDeviceFactory& operator=(const AudioDeviceFactory&);
    
};  /* class AudioDeviceFactory */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_FACTORY_INCLUDED */



/*
 * This file has not been truncated
 */

