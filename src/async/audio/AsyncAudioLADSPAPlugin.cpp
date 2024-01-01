/**
@file   AsyncAudioLADSPAPlugin.cpp
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2023-12-09

\verbatim
Async - A library for programming event driven applications
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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

//#define _DEFAULT_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>
#include <cmath>
#include <cstring>
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

#include "AsyncAudioLADSPAPlugin.h"


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

bool AudioLADSPAPlugin::findPluginsInDir(std::string dir)
{
  if (dir.empty())
  {
    return false;
  }
  if (dir[dir.length()-1] != '/')
  {
    dir += "/";
  }
  //std::cout << "### dir=" << dir << std::endl;
  struct dirent **namelist;
  int n = scandir(dir.c_str(), &namelist,
      [](const struct dirent* de) -> int
      {
        auto len = strlen(de->d_name);
        return (len > 3) && (strcmp(de->d_name+len-3, ".so") == 0);
      },
      alphasort);
  if (n == -1)
  {
    perror(std::string("scandir(" + dir + ")").c_str());
    return false;
  }

  while (n--)
  {
    //std::cout << "### " << namelist[n]->d_name << std::endl;
    PluginIndex i = 0;
    for (;;)
    {
      AudioLADSPAPlugin p(dir + namelist[n]->d_name, i++);
      if (p.ladspaDescriptor() == nullptr)
      {
        break;
      }
      auto inst = std::make_shared<InstanceInfo>();
      inst->m_path = p.path();
      inst->m_index = i-1;
      inst->m_unique_id = p.uniqueId();
      inst->m_label = p.label();
      labelMap()[inst->m_label] = inst;
      idMap()[inst->m_unique_id] = inst;
    }
    free(namelist[n]);
  }
  free(namelist);
  return true;
} /* AudioLADSPAPlugin::findPluginsInDir */


bool AudioLADSPAPlugin::findPlugins(void)
{
  std::string ladspa_path(LADSPA_PLUGIN_DIRS);
  const char* env_ladspa_path = getenv("LADSPA_PATH");
  if (env_ladspa_path != nullptr)
  {
    ladspa_path = env_ladspa_path;
  }
  std::string::size_type begin = 0, end = 0;
  do
  {
    end = ladspa_path.find(':', begin);
    findPluginsInDir(ladspa_path.substr(begin, end));
    begin = end + 1;
  } while (end != std::string::npos);
  return true;
} /* AudioLADSPAPlugin::findPlugins */


AudioLADSPAPlugin::AudioLADSPAPlugin(AudioLADSPAPlugin::UniqueID id)
{
  if (idMap().empty())
  {
    findPlugins();
  }
  auto instit = idMap().find(id);
  if (instit == idMap().end())
  {
    return;
  }
  auto inst_info = instit->second;
  m_path = inst_info->m_path;
  m_index = inst_info->m_index;
} /* AudioLADSPAPlugin::AudioLADSPAPlugin */


AudioLADSPAPlugin::AudioLADSPAPlugin(const std::string& label)
{
  if (labelMap().empty())
  {
    findPlugins();
  }
  auto instit = labelMap().find(label);
  if (instit == labelMap().end())
  {
    return;
  }
  auto inst_info = instit->second;
  m_path = inst_info->m_path;
  m_index = inst_info->m_index;
} /* AudioLADSPAPlugin::AudioLADSPAPlugin */


AudioLADSPAPlugin::~AudioLADSPAPlugin(void)
{
  deactivate();

  delete [] m_ctrl_buf;
  m_ctrl_buf = nullptr;

  if ((m_desc != nullptr) && (m_desc->cleanup != nullptr) &&
      (m_inst_handle != nullptr))
  {
    m_desc->cleanup(m_inst_handle);
  }
  m_inst_handle = nullptr;
  m_desc = nullptr;

  if (m_handle != nullptr)
  {
    if (dlclose(m_handle) != 0)
    {
      std::cerr << "*** ERROR: Failed to unload plugin "
                << m_path << ": " << dlerror() << std::endl;
    }
  }
  m_handle = nullptr;
} /* AudioLADSPAPlugin::~AudioLADSPAPlugin */


bool AudioLADSPAPlugin::initialize(void)
{
  if (m_path.empty())
  {
    //std::cerr << "*** ERROR: Empty LADSPA plugin path" << std::endl;
    return false;
  }

  if (ladspaDescriptor() == nullptr)
  {
    std::cerr << "*** ERROR: Could not find LADSPA instance for index "
              << m_index << " in plugin " << m_path << std::endl;
    return false;
  }

  m_inst_handle = m_desc->instantiate(m_desc, INTERNAL_SAMPLE_RATE);
  if (m_inst_handle == nullptr)
  {
    std::cerr << "*** ERROR: Could not instantiate LADSPA instance for index "
              << m_index << " in plugin " << m_path << std::endl;
    return false;
  }

  m_ctrl_buf = new LADSPA_Data[m_desc->PortCount];

  for (PortNumber i=0; i<m_desc->PortCount; ++i)
  {
    const LADSPA_PortDescriptor& port_desc = m_desc->PortDescriptors[i];

    if (!LADSPA_IS_PORT_INPUT(port_desc) && !LADSPA_IS_PORT_OUTPUT(port_desc))
    {
      std::cerr << "*** ERROR: Invalid LADSPA plugin " << m_path
                << " with index " << m_index << ". Port " << i
                << " is neither input nor output." << std::endl;
      return false;
    }
    if (!LADSPA_IS_PORT_CONTROL(port_desc) && !LADSPA_IS_PORT_AUDIO(port_desc))
    {
      std::cerr << "*** ERROR: Invalid LADSPA plugin " << m_path
                << " with index " << m_index << ". Port " << i
                << " is neither type control nor audio." << std::endl;
      return false;
    }

    if (LADSPA_IS_PORT_CONTROL(port_desc))
    {
      if (!setDefault(i))
      {
        std::cerr << "*** ERROR: Illegal default handling in LADSPA instance "
                     "for index " << m_index << " in plugin " << m_path
                  << std::endl;
        return false;
      }
      m_desc->connect_port(m_inst_handle, i, &m_ctrl_buf[i]);
    }
    if (LADSPA_IS_PORT_AUDIO(port_desc))
    {
      if (LADSPA_IS_PORT_INPUT(port_desc))
      {
        if (m_sample_input_port != NOPORT)
        {
          std::cerr << "*** ERROR: Only single audio input port LADSPA "
                       "instances supported but index " << m_index
                    << " in plugin " << m_path
                    << " has multiple audio input ports"
                    << std::endl;
          return false;
        }
        m_sample_input_port = i;
      }
      else
      {
        if (m_sample_output_port != NOPORT)
        {
          std::cerr << "*** ERROR: Only single audio output port LADSPA "
                       "instances supported but index " << m_index
                    << " in plugin " << m_path
                    << " has multiple audio output ports"
                    << std::endl;
          return false;
        }
        m_sample_output_port = i;
      }
    }
  }

  if ((m_sample_input_port == NOPORT) || (m_sample_output_port == NOPORT))
  {
    std::cerr << "*** ERROR: LADSPA instances must have exactly one input "
                 "port and one output port but index " << m_index
              << " in plugin " << m_path
              << " is missing an input or output port" << std::endl;
    return false;
  }

  activate();

  return true;

} /* AudioLADSPAPlugin::initialize */


bool AudioLADSPAPlugin::setControl(PortNumber portno, LADSPA_Data val)
{
  assert(m_desc != nullptr);
  assert(m_ctrl_buf != nullptr);
  if (portno >= m_desc->PortCount)
  {
    return false;
  }
  const LADSPA_PortDescriptor& port_desc = m_desc->PortDescriptors[portno];
  if (!LADSPA_IS_PORT_CONTROL(port_desc) || !LADSPA_IS_PORT_INPUT(port_desc))
  {
    return false;
  }

  float (*conv)(float) = [](float x) { return x; };
  const auto& port_range_hint = m_desc->PortRangeHints[portno];
  const auto& hint_desc = port_range_hint.HintDescriptor;
  LADSPA_Data mult = 1.0f;
  if (LADSPA_IS_HINT_SAMPLE_RATE(hint_desc))
  {
    mult = INTERNAL_SAMPLE_RATE;
  }
  auto lower_bound = port_range_hint.LowerBound * mult;
  auto upper_bound = port_range_hint.UpperBound * mult;
  if (LADSPA_IS_HINT_INTEGER(hint_desc))
  {
    conv = roundf;
  }
  if (LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc) && (val < lower_bound))
  {
    val = conv(lower_bound);
  }
  if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc) && (val > upper_bound))
  {
    val = conv(upper_bound);
  }

  m_ctrl_buf[portno] = val;

  return true;
} /* AudioLADSPAPlugin::setControl */


void AudioLADSPAPlugin::activate(void)
{
  assert((m_desc != nullptr) && (m_inst_handle != nullptr));
  if ((m_desc->activate != nullptr) && !m_is_active)
  {
    m_desc->activate(m_inst_handle);
  }
  m_is_active = false;
} /* AudioLADSPAPlugin::activate */


void AudioLADSPAPlugin::deactivate(void)
{
  if ((m_desc != nullptr) && (m_desc->deactivate != nullptr) &&
      (m_inst_handle != nullptr) && m_is_active)
  {
    m_desc->deactivate(m_inst_handle);
  }
  m_is_active = false;
} /* AudioLADSPAPlugin::deactivate */


bool AudioLADSPAPlugin::portIsControl(PortNumber portno) const
{
  assert(m_desc != nullptr);
  if (portno >= m_desc->PortCount)
  {
    return false;
  }
  const LADSPA_PortDescriptor& port_desc = m_desc->PortDescriptors[portno];
  return LADSPA_IS_PORT_CONTROL(port_desc);
} /* AudioLADSPAPlugin::portIsControl */


bool AudioLADSPAPlugin::portIsAudio(PortNumber portno) const
{
  assert(m_desc != nullptr);
  if (portno >= m_desc->PortCount)
  {
    return false;
  }
  const LADSPA_PortDescriptor& port_desc = m_desc->PortDescriptors[portno];
  return LADSPA_IS_PORT_AUDIO(port_desc);
} /* AudioLADSPAPlugin::portIsAudio */


bool AudioLADSPAPlugin::portIsInput(PortNumber portno) const
{
  assert(m_desc != nullptr);
  if (portno >= m_desc->PortCount)
  {
    return false;
  }
  const LADSPA_PortDescriptor& port_desc = m_desc->PortDescriptors[portno];
  return LADSPA_IS_PORT_INPUT(port_desc);
} /* AudioLADSPAPlugin::portIsInput */


bool AudioLADSPAPlugin::portIsOutput(PortNumber portno) const
{
  assert(m_desc != nullptr);
  if (portno >= m_desc->PortCount)
  {
    return false;
  }
  const LADSPA_PortDescriptor& port_desc = m_desc->PortDescriptors[portno];
  return LADSPA_IS_PORT_OUTPUT(port_desc);
} /* AudioLADSPAPlugin::portIsOutput */


void AudioLADSPAPlugin::print(const std::string& prefix)
{
  assert(m_desc != nullptr);

  std::cout << prefix << "\"" << m_desc->Name << "\""
            << " (" << m_desc->Label << ")"
            << " by \"" << m_desc->Maker << "\""
            << " (C) " << m_desc->Copyright
            << std::endl;
  std::cout << prefix << "    Path: " << m_path << std::endl;

  for (PortNumber i=0; i<m_desc->PortCount; ++i)
  {
    LADSPA_PortDescriptor port_desc = m_desc->PortDescriptors[i];

    if (LADSPA_IS_PORT_AUDIO(port_desc))
    {
      continue;
    }

    std::cout << prefix << "    "
              << (LADSPA_IS_PORT_INPUT(port_desc) ? "In " : "Out")
              << ": \"" << m_desc->PortNames[i] << "\" ";

    //if (LADSPA_IS_PORT_CONTROL(port_desc))
    //{
    //  std::cout << "control ";
    //}

    float (*conv)(float) = [](float x) { return x; };
    const auto& port_range_hint = m_desc->PortRangeHints[i];
    const auto& hint_desc = port_range_hint.HintDescriptor;
    if (LADSPA_IS_HINT_INTEGER(hint_desc))
    {
      //std::cout << "int:";
      conv = roundf;
    }
    //else if (LADSPA_IS_HINT_TOGGLED(hint_desc))
    //{
    //  std::cout << "bool:";
    //}
    //else
    //{
    //  std::cout << "float:";
    //}

    bool bounded_below = LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc);
    bool bounded_above = LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc);
    if (bounded_below || bounded_above)
    {
      LADSPA_Data samp_rate = 1.0f;
      if (LADSPA_IS_HINT_SAMPLE_RATE(hint_desc))
      {
        samp_rate = INTERNAL_SAMPLE_RATE;
      }
      std::cout << "[";
      if (bounded_below)
      {
        std::cout << conv(samp_rate * port_range_hint.LowerBound);
      }
      std::cout << ",";
      if (bounded_above)
      {
        std::cout << conv(samp_rate * port_range_hint.UpperBound);
      }
      std::cout << "]";
    }

    if (LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
    {
      std::cout << " (log)";
    }

    std::cout << " = " << m_ctrl_buf[i];

    std::cout << std::endl;
  }
} /* AudioLADSPAPlugin::print */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioLADSPAPlugin::processSamples(float *dest, const float *src,
                                       int count)
{
  assert(dest != nullptr);
  assert(src != nullptr);
  assert(m_sample_input_port != NOPORT);
  assert(m_sample_output_port != NOPORT);
  if (count <= 0)
  {
    return;
  }
  m_desc->connect_port(m_inst_handle, m_sample_input_port,
      const_cast<LADSPA_Data*>(src));
  m_desc->connect_port(m_inst_handle, m_sample_output_port, dest);
  m_desc->run(m_inst_handle, count);
} /* AudioLADSPAPlugin::processSamples */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

const LADSPA_Descriptor* AudioLADSPAPlugin::ladspaDescriptor(void)
{
  m_handle = dlopen(m_path.c_str(), RTLD_NOW);
  if (m_handle == nullptr)
  {
    std::cerr << "*** ERROR: Failed to load plugin "
              << m_path << ": " << dlerror() << std::endl;
    return nullptr;
  }

  using ConstructFunc = LADSPA_Descriptor_Function;
  ConstructFunc construct = (ConstructFunc)dlsym(m_handle, "ladspa_descriptor");
  if (construct == nullptr)
  {
    std::cerr << "*** ERROR: Could not find LADSPA descriptor function for "
                 "plugin " << m_path << ": " << dlerror() << std::endl;
    return nullptr;
  }

  m_desc = construct(m_index);
  return m_desc;
} /* AudioLADSPAPlugin::ladspaDescriptor */


bool AudioLADSPAPlugin::setDefault(PortNumber portno)
{
  assert(m_desc != nullptr);

  LADSPA_Data& def = m_ctrl_buf[portno];
  const auto& port_range_hint = m_desc->PortRangeHints[portno];
  const auto& hint_desc = port_range_hint.HintDescriptor;

  if (!LADSPA_IS_HINT_HAS_DEFAULT(hint_desc))
  {
    def = 0.0f;
    return true;
  }

  LADSPA_Data mult = 1.0f;
  if (LADSPA_IS_HINT_SAMPLE_RATE(hint_desc))
  {
    mult = INTERNAL_SAMPLE_RATE;
  }
  auto lower_bound = port_range_hint.LowerBound * mult;
  auto upper_bound = port_range_hint.UpperBound * mult;

  if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_desc))
  {
    if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc))
    {
      return false;
    }
    def = lower_bound;
  }

  if (LADSPA_IS_HINT_DEFAULT_LOW(hint_desc))
  {
    if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc) ||
        !LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc))
    {
      return false;
    }
    if (LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
    {
      def = expf(logf(lower_bound)*0.75 + logf(upper_bound)*0.25);
    }
    else
    {
      def = lower_bound*0.75 + upper_bound*0.25;
    }
  }

  if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_desc))
  {
    if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc) ||
        !LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc))
    {
      return false;
    }
    if (LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
    {
      def = expf(logf(lower_bound)*0.5 + logf(upper_bound)*0.5);
    }
    else
    {
      def = lower_bound*0.5 + upper_bound*0.5;
    }
  }

  if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_desc))
  {
    if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc) ||
        !LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc))
    {
      return false;
    }
    if (LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
    {
      def = expf(logf(lower_bound)*0.25 + logf(upper_bound)*0.75);
    }
    else
    {
      def = lower_bound*0.25 + upper_bound*0.75;
    }
  }

  if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_desc))
  {
    if (!LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc))
    {
      return false;
    }
    def = upper_bound;
  }

  if (LADSPA_IS_HINT_DEFAULT_0(hint_desc))
  {
    def = 0.0f;
  }

  if (LADSPA_IS_HINT_DEFAULT_1(hint_desc))
  {
    def = 1.0f;
  }

  if (LADSPA_IS_HINT_DEFAULT_100(hint_desc))
  {
    def = 100.0f;
  }

  if (LADSPA_IS_HINT_DEFAULT_440(hint_desc))
  {
    def = 440.0f;
  }

  if (LADSPA_IS_HINT_INTEGER(hint_desc))
  {
    def = roundf(def);
  }

  return true;
} /* AudioLADSPAPlugin::setDefault */


/*
 * This file has not been truncated
 */
