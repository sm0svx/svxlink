/**
@file   LADSPAPluginLoader.h
@brief  Helper class for loading LADSPA plugins
@author Tobias Blomberg / SM0SVX
@date   2024-09-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

#ifndef LADSPA_PLUGIN_LOADER_INCLUDED
#define LADSPA_PLUGIN_LOADER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#ifdef LADSPA_VERSION
#include <AsyncAudioLADSPAPlugin.h>
#endif


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
@brief  A class for loading a chain of LADSPA plugins
@author Tobias Blomberg / SM0SVX
@date   2024-09-09
*/
class LADSPAPluginLoader
{
  public:
    /**
     * @brief   Load plugins as specified in the given configuration section
     * @param   cfg An initialized config object
     * @param   sec The section to load the configuration from
     * @return  Return \em true if loading is successful
     */
    bool load(Async::Config& cfg, const std::string& sec)
    {
#ifdef LADSPA_VERSION
      std::vector<std::string> ladspa_plugin_cfg;
      if (cfg.getValue(sec, "LADSPA_PLUGINS", ladspa_plugin_cfg))
      {
        for (const auto& pcfg : ladspa_plugin_cfg)
        {
          std::istringstream is(pcfg);
          std::string label;
          std::getline(is, label, ':');
          Async::AudioLADSPAPlugin* plug = nullptr;
          if (label[0] == '@')
          {
            std::string subsec = label.substr(1);
            if (!cfg.getValue(subsec, "LABEL", label))
            {
              std::cerr << "*** ERROR: The " << subsec
                        << "/LABEL configuration variable is missing"
                        << std::endl;
              return false;
            }
            plug = new Async::AudioLADSPAPlugin(label);
            if (!plug->initialize())
            {
              std::cout << "*** ERROR: Could not instantiate LADSPA plugin "
                           "instance with label '" << label
                        << "' specified in configuration variable "
                        << subsec << "/LABEL" << std::endl;
              return false;
            }
            for (const auto& port_name : cfg.listSection(subsec))
            {
              if (port_name != "LABEL")
              {
                std::string lookup_name(port_name);
                std::replace(lookup_name.begin(), lookup_name.end(), '_', ' ');
                auto port_num = plug->findControlInputByName(lookup_name);
                if (port_num == Async::AudioLADSPAPlugin::npos)
                {
                  std::cerr << "*** ERROR: Could not find LADSPA port '"
                            << lookup_name
                            << "' specified in configuration section '"
                            << subsec << "'" << std::endl;
                  return false;
                }
                cfg.subscribeValue(subsec, port_name, 0,
                    [=](LADSPA_Data val)
                    {
                      plug->setControl(port_num, val);
                      //plug->print(std::string("### ") + sec + ": ");
                    });
              }
            }
          }
          else
          {
            plug = new Async::AudioLADSPAPlugin(label);
            if (!plug->initialize())
            {
              std::cerr << "*** ERROR: Could not instantiate LADSPA plugin "
                           "instance with label '" << label << "' "
                        << "specified in configuration variable "
                        << sec << "/LADSPA_PLUGINS." << std::endl;
              return false;
            }
            unsigned long portno = 0;
            LADSPA_Data val;
            while (is >> val)
            {
              while ((portno < plug->portCount()) &&
                     !(plug->portIsControl(portno) &&
                       plug->portIsInput(portno)))
              {
                ++portno;
              }
              if (portno >= plug->portCount())
              {
                std::cerr << "*** ERROR: Too many parameters specified for "
                             "LADSPA plugin '" << plug->label()
                          << "' specified in configuration variable "
                          << sec << "/LADSPA_PLUGINS." << std::endl;
                return false;
              }
              plug->setControl(portno++, val);
              char colon = 0;
              if ((is >> colon) && (colon != ':'))
              {
                std::cerr << "*** ERROR: Illegal format for " << sec
                          << "/LADSPA_PLUGINS configuration variable"
                          << std::endl;
                return false;
              }
            }
          }

          plug->print(sec + ": ");

          if (m_chain_src == nullptr)
          {
            m_chain_sink = plug;
          }
          else
          {
            m_chain_src->registerSink(plug, true);
          }
          m_chain_src = plug;
        }
      }
#endif
      return true;
    } /* load */

    Async::AudioSink* chainSink(void) { return m_chain_sink; }
    Async::AudioSource* chainSource(void) { return m_chain_src; }

  protected:

  private:
    Async::AudioSink*   m_chain_sink  = nullptr;
    Async::AudioSource* m_chain_src   = nullptr;

};  /* class LADSPAPluginLoader */


#endif /* LADSPA_PLUGIN_LOADER_INCLUDED */

/*
 * This file has not been truncated
 */
