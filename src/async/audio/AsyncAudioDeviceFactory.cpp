/**
@file	 AsyncAudioDeviceFactory.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>


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

#include "AsyncAudioDeviceFactory.h"



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
 * Public static functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioDeviceFactory::~AudioDeviceFactory(void)
{
} /* AudioDeviceFactory::~AudioDeviceFactory */


bool AudioDeviceFactory::registerCreator(const std::string &name,
                                         CreatorFunc creator)
{
  creator_map[name] = creator;
  return true;
} /* AudioDeviceFactory::registerCreator */


AudioDevice *AudioDeviceFactory::create(const std::string &name,
                                        const std::string &dev_name)
{
  CreatorMap::iterator it = creator_map.find(name);
  if (it == creator_map.end())
  {
    return 0;
  }
  return it->second(dev_name);
} /* AudioDeviceFactory::create */


std::string AudioDeviceFactory::validDevTypes(void) const
{
  string type_list;
  CreatorMap::const_iterator it;
  for (it = creator_map.begin(); it != creator_map.end(); ++it)
  {
    if (!type_list.empty())
    {
      type_list += " ";
    }
    type_list += it->first;
  }
  return type_list;
} /* AudioDeviceFactory::validDevTypes */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

AudioDeviceFactory::AudioDeviceFactory(void)
{
} /* AudioDeviceFactory::AudioDeviceFactory */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

