/**
@file	 AsyncAudioDeviceFactory.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2009-12-26

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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

/** @example AudioDeviceFactory_demo.cpp
An example of how to use the AudioDeviceFactory class
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

#define REGISTER_AUDIO_DEVICE_TYPE(_name, _class) \
  AudioDevice *create_ ## _class(const string& dev_name) \
          { return new _class(dev_name); } \
  static bool _class ## _creator_registered = \
          AudioDeviceFactory::instance()->registerCreator(_name, \
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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2008-

A_detailed_class_description

\include AudioDeviceFactory_demo.cpp
*/
class AudioDeviceFactory
{
  public:
    typedef AudioDevice* (*CreatorFunc)(const std::string &dev_designator);
    
    static AudioDeviceFactory *instance(void)
    {
      if (_instance == 0)
      {
        _instance = new AudioDeviceFactory;
      }
      return _instance;
    }

    /**
     * @brief 	Destructor
     */
    ~AudioDeviceFactory(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool registerCreator(const std::string &name, CreatorFunc creator);

    AudioDevice *create(const std::string &name, const std::string &dev_name);

    std::string validDevTypes(void) const;
    
  protected:
    /**
     * @brief   Default constuctor
     */
    AudioDeviceFactory(void);
  
    
  private:
    typedef std::map<std::string, CreatorFunc> CreatorMap;
    
    static AudioDeviceFactory *_instance;

    CreatorMap creator_map;
    
    AudioDeviceFactory(const AudioDeviceFactory&);
    AudioDeviceFactory& operator=(const AudioDeviceFactory&);
    
};  /* class AudioDeviceFactory */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_FACTORY_INCLUDED */



/*
 * This file has not been truncated
 */

