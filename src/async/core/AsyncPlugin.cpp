/**
@file   AsyncPlugin.cpp
@brief  A base class for making a class into a dynamic loadable plugin
@author Tobias Blomberg / SM0SVX
@date   2022-08-23

\verbatim
Async - A library for programming event driven applications
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

#include <dlfcn.h>
#include <link.h>

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

#include "AsyncPlugin.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Static class variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

namespace {


/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/



}; /* End of anonymous namespace */

/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Plugin* Plugin::load(const std::string& path)
{
  //std::cout << "### Loading plugin \"" << path << "\"" << std::endl;

  void *handle = nullptr;
  handle = dlopen(path.c_str(), RTLD_NOW);
  if (handle == nullptr)
  {
    std::cerr << "*** ERROR: Failed to load plugin "
              << path << ": " << dlerror() << std::endl;
    return nullptr;
  }

  struct link_map *link_map;
  if (dlinfo(handle, RTLD_DI_LINKMAP, &link_map) == -1)
  {
    std::cerr << "*** ERROR: Could not read information for plugin "
              << path << ": " << dlerror() << std::endl;
    dlclose(handle);
    return nullptr;
  }
  //std::cout << "### Found plugin " << link_map->l_name << std::endl;

  ConstructFunc construct = (ConstructFunc)dlsym(handle, "construct");
  if (construct == nullptr)
  {
    std::cerr << "*** ERROR: Could not find construct function for plugin "
              << path << ": " << dlerror() << std::endl;
    dlclose(handle);
    return nullptr;
  }

  Plugin *plugin = construct();
  if (plugin == nullptr)
  {
    std::cerr << "*** ERROR: Construction failed for plugin "
              << path << std::endl;
    dlclose(handle);
    return nullptr;
  }

  plugin->setHandle(handle);
  plugin->m_plugin_path = link_map->l_name;

  return plugin;
} /* Plugin::load */


void Plugin::unload(Plugin* p)
{
  if (p != nullptr)
  {
    void* handle = p->pluginHandle();
    delete p;
    dlclose(handle);
  }
} /* Plugin::unload */


Plugin::Plugin(void)
{
  //std::cout << "### Plugin::Plugin" << std::endl;
} /* Plugin::Plugin */


Plugin::~Plugin(void)
{
  //std::cout << "### Plugin::~Plugin" << std::endl;
  m_handle = nullptr;
} /* Plugin::~Plugin */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
