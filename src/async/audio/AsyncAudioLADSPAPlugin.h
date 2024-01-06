/**
@file   AsyncAudioLADSPAPlugin.h
@brief  A class for using a LADSPA plugin as an audio processor
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

/** @example AsyncAudioLADSPAPlugin_demo.cpp
An example of how to use the AudioLADSPAPlugin class
*/

#ifndef ASYNC_AUDIO_LADSPA_PLUGIN_INCLUDED
#define ASYNC_AUDIO_LADSPA_PLUGIN_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

extern "C" {
  #include <ladspa.h>
};

#include <string>
#include <map>
#include <memory>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioProcessor.h>


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
@brief  Use a LADSPA plugin as an audio processor
@author Tobias Blomberg / SM0SVX
@date   2023-12-09

This class is used to load a LADSPA plugin and use it as an audio processor.

\include AsyncAudioLADSPAPlugin_demo.cpp
*/
class AudioLADSPAPlugin : public AudioProcessor
{
  public:
    using PortNumber  = decltype(LADSPA_Descriptor::PortCount);
    using PluginIndex = unsigned long;
    using UniqueID    = unsigned long;

    /**
     * @brief   Find any LADSPA plugins in the given subdirectory
     * @param   dir The path to the directory to look in
     *
     * This function will go through all files in the given subdirectory,
     * reading any *.so files it can find and load them as LADSPA plugins. The
     * plugins are not fully initialized in this process but rather just the
     * necessary calls are made to extract enough information to determine if
     * this is a plugin that is compatible with this class. Any plugin found to
     * be usable is put in an index so that we later can find plugins using
     * their label.
     */
    static bool findPluginsInDir(std::string dir);

    /**
     * @brief   Find LADSPA plugins in standard subdirectories
     *
     * LADSPA plugins are typically installed in /usr/lib64/ladspa (on a 64 bit
     * x86 system). This default value is adapted automatically to match the
     * target system during compilation of the software.
     *
     * If the LADSPA_PATH environment variable is set it will override the
     * default path.
     *
     * This function is called by the "label variant" of the constructor if the
     * plugin index is empty.
     */
    static bool findPlugins(void);

    /**
     * @brief   Constructor
     * @param   path  The full path of the plugin to load
     * @param   index The index of the plugin to instantiate
     *
     * This constructor is normally not used directly unless there is a special
     * requirement to load a specific LADSPA plugin file with a known plugin
     * index.
     */
    AudioLADSPAPlugin(const std::string& path, PluginIndex index)
      : m_path(path), m_index(index) {}

    /**
     * @brief   Constructor
     * @param   id The plugin unique id to look for
     *
     * Use this constructor for creating a LADSPA plugin instance if you know
     * its unique ID. The easiest way normally is to use the label to find the
     * plugin.
     */
    AudioLADSPAPlugin(UniqueID id);

    /**
     * @brief   Constructor
     * @param   label The plugin label to look for
     *
     * This is the main constructor for creating a LADSPA plugin instance.
     */
    AudioLADSPAPlugin(const std::string& label);

    /**
     * @brief   Disallow copy construction
     */
    AudioLADSPAPlugin(const AudioLADSPAPlugin&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    AudioLADSPAPlugin& operator=(const AudioLADSPAPlugin&) = delete;

    /**
     * @brief   Destructor
     */
    ~AudioLADSPAPlugin(void);

    /**
     * @brief   Initialize the plugin
     * @return  Return \em true on success or else \em false is returned
     *
     * All loading, instantiation and initialization of the plugin is done in
     * this function. The LADSPA activate call is called at the end of the
     * function so it is directly ready to process audio when this function
     * returns \em true. If the function returns \em false, it's not allowed to
     * call any other functions so the object should be deleted as soon as
     * possible.
     */
    bool initialize(void);

    /**
     * @brief   Set a control input to the given value
     * @param   portno  The port number to set
     * @param   val     The value to set
     */
    bool setControl(PortNumber portno, LADSPA_Data val);

    /**
     * @brief   Activate the plugin
     *
     * Use this function to activate this plugin. Activation is done in the
     * initialize function so manual activation is normally not needed. Read
     * more about plugin activation in the LADSPA documentation.
     */
    void activate(void);

    /**
     * @brief   Deactivate the plugin
     *
     * Use this function to deactivate this plugin. Read more about plugin
     * activation in the LADSPA documentation.
     */
    void deactivate(void);

    /**
     * @brief   Get the path to the plugin
     * @returns Returns the path to the plugin
     */
    std::string path(void) const { return m_path; }

    /**
     * @brief   Get the unique ID for the plugin
     * @returns Returns the unique ID of the plugin
     *
     * All plugins have a unique ID which can be used to find it.
     */
    UniqueID uniqueId(void) const { return m_desc->UniqueID; }

    /**
     * @brief   Get the unique label for the plugin
     * @returns Returns the unique label for the plugin
     *
     * The label is what most often is used to find a specific plugin.
     */
    std::string label(void) const { return m_desc->Label; }

    /**
     * @brief   Get the name of the plugin
     * @returns Returns the name of the plugin
     *
     * This function return the free text name/display name for the plugin.
     */
    std::string name(void) const { return m_desc->Name; }

    /**
     * @brief   Get information on the maker of the plugin
     * @returns Returns a string describing the plugin author.
     */
    std::string maker(void) const { return m_desc->Maker; }

    /**
     * @brief   Get the copyright information for the plugin
     * @returns Returns a string describing the copyright
     */
    std::string copyright(void) const { return m_desc->Copyright; }

    /**
     * @brief   Get the number of ports for the plugin
     * @returns Returns the total number of control/audio input/output ports
     */
    PortNumber portCount(void) const { return m_desc->PortCount; }

    /**
     * @brief   Check if a port is a control port
     * @returns Returns \em true if this is a control port
     */
    bool portIsControl(PortNumber portno) const;

    /**
     * @brief   Check if a port is an audio port
     * @returns Returns \em true if this is an audio port
     */
    bool portIsAudio(PortNumber portno) const;

    /**
     * @brief   Check if a port is an input port
     * @returns Returns \em true if this is an input port
     */
    bool portIsInput(PortNumber portno) const;

    /**
     * @brief   Check if a port is an output port
     * @returns Returns \em true if this is an output port
     */
    bool portIsOutput(PortNumber portno) const;

    /**
     * @brief   Print some useful information for the plugin
     */
    void print(const std::string& prefix="");

  protected:
    /**
     * @brief Process incoming samples and put them into the output buffer
     * @param dest  Destination buffer
     * @param src   Source buffer
     * @param count Number of samples in the source buffer
     *
     * This function should be reimplemented by the inheriting class to
     * do the actual processing of the incoming samples. All samples must
     * be processed, otherwise they are lost and the output buffer will
     * contain garbage.
     */
    virtual void processSamples(float *dest, const float *src,
                                int count) override;

  private:
    struct InstanceInfo
    {
      std::string   m_path;
      PluginIndex   m_index;
      UniqueID      m_unique_id;
      std::string   m_label;
    };
    using InstanceInfoP = std::shared_ptr<InstanceInfo>;
    using LabelMap      = std::map<std::string, InstanceInfoP>;
    using IDMap         = std::map<UniqueID, InstanceInfoP>;

    static const PortNumber NOPORT = 9999UL;

    static IDMap& idMap(void)
    {
      static IDMap id_map;
      return id_map;
    }

    static LabelMap& labelMap(void)
    {
      static LabelMap label_map;
      return label_map;
    }

    const LADSPA_Descriptor* ladspaDescriptor(void);
    bool setDefault(PortNumber portno);

    std::string               m_path;
    void*                     m_handle              = nullptr;
    PluginIndex               m_index               = 0;
    const LADSPA_Descriptor*  m_desc                = nullptr;
    LADSPA_Handle             m_inst_handle         = nullptr;
    bool                      m_is_active           = false;
    LADSPA_Data*              m_ctrl_buf            = nullptr;
    PortNumber                m_sample_input_port   = NOPORT;
    PortNumber                m_sample_output_port  = NOPORT;

};  /* class AudioLADSPAPlugin */


} /* namespace Async */

#endif /* ASYNC_AUDIO_LADSPA_PLUGIN_INCLUDED */

/*
 * This file has not been truncated
 */
