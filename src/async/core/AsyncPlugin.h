/**
@file   AsyncPlugin.h
@brief  A base class for making a class into a dynamic loadable plugin
@author Tobias Blomberg / SM0SVX
@date   2022-08-23

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

/** @example AsyncPlugin_demo.cpp
An example of how to use the Async::Plugin class
*/

#ifndef ASYNC_PLUGIN_INCLUDED
#define ASYNC_PLUGIN_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


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
@brief  A base class for making a class into a dynamic loadable plugin
@author Tobias Blomberg / SM0SVX
@date   2022-08-23

\include AsyncPlugin_demo.cpp
*/
class Plugin
{
  public:
    /**
     * @brief   Load the plugin from the specified path
     * @param   path The file path
     */
    static Plugin* load(const std::string& path);

    /**
     * @brief   Load the plugin from the specified path returning correct type
     * @param   path The file path
     *
     * The application may use this function to load the plugin, check that it
     * is of the correct type and then return a pointer to that type.
     */
    template <class T>
    static T* load(const std::string& path)
    {
      Plugin* p = Plugin::load(path);
      if (p == nullptr)
      {
        return nullptr;
      }
      T* plugin = dynamic_cast<T*>(p);
      if (plugin == nullptr)
      {
        std::cerr << "*** ERROR: Could not load plugin \"" << path
                  << "\": Not a \"" << T::typeName() << "\" plugin"
                  << std::endl;
        delete p;
      }
      return plugin;
    }

    static void unload(Plugin* p);

    /**
     * @brief   Default constructor
     */
    Plugin(void);

    /**
     * @brief   Disallow copy construction
     */
    Plugin(const Plugin&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    Plugin& operator=(const Plugin&) = delete;

    /**
     * @brief   Retrieve the handle returned from the dlopen function
     * @return  Returns a handle from dlopen (@see man 3 dlopen)
     */
    void* pluginHandle(void) const { return m_handle; }

    /**
     * @brief   Retrieve the path used to find the plugin
     * @return  Returns the path to the plugin
     *
     * This function can be called to find out which path was used to load the
     * plugin.
     */
    const std::string& pluginPath(void) const { return m_plugin_path; }

  protected:
    /**
     * @brief   Destructor
     */
    virtual ~Plugin(void);

  private:
    typedef Plugin* (*ConstructFunc)(void);

    void*       m_handle      = nullptr;
    std::string m_plugin_path;

    void setHandle(void* handle) { m_handle = handle; }

};  /* class Plugin */


} /* namespace Async */

#endif /* ASYNC_PLUGIN_INCLUDED */

/*
 * This file has not been truncated
 */
