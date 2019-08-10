/**
@file	 AsyncAudioSelector.h
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2019 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_SELECTOR_INCLUDED
#define ASYNC_AUDIO_SELECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

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
@brief	This class is used to select one of many audio streams
@author Tobias Blomberg / SM0SVX
@date   2006-08-01

This class is used to select one of many incoming audio streams. Incoming
samples on non-selected branches will be thrown away.
*/
class AudioSelector : public AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioSelector(void);
  
    /**
     * @brief 	Destructor
     */
    ~AudioSelector(void);
  
    /**
     * @brief 	Add an audio source to the selector
     * @param 	source The audio source to add
     */
    void addSource(AudioSource *source);
    
    /**
     * @brief 	Remove a previously added audio source from the selector
     * @param 	source The audio source to remove
     */
    void removeSource(AudioSource *source);
    
    /**
     * @brief 	Set the prio to be used for selection
     * @param 	source The audio source
     * @param 	prio   The priority to set. Higher numbers give higher priority.
     */
    void setSelectionPrio(AudioSource *source, int prio);
    
    /**
     * @brief 	Enable autoselection on the given source
     * @param 	source The audio source
     * @param 	prio   The priority to set. Higher numbers give higher priority.
     */
    void enableAutoSelect(AudioSource *source, int prio);

    /**
     * @brief 	Disable autoselection on the given source
     * @param 	source The audio source
     */
    void disableAutoSelect(AudioSource *source);
    
    /**
     * @brief 	Find out if auto select is enabled or not for the given source
     * @param 	source The audio source
     * @return	Returns \em true if auto select is enabled for the given source
     *          or else \em false is returned
     */
    bool autoSelectEnabled(const AudioSource *source) const;

    /**
     * @brief 	Select one of the previously added audio sources
     * @param 	source The audio source to select. 0 = none selected.
     */
    void selectSource(AudioSource *source);

    /**
     * @brief   Find out which source that is currently selected
     * @return  Returns the selected source or 0 if no source is selected
     */
    AudioSource *selectedSource(void) const;

    /**
     * @brief   Set if this souce want to wait for allSamplesFlushed
     * @param 	source The audio source to select. 0 = none selected.
     * @param   flush_wait Set to \em true to wait for flush or else \em false
     *
     * Normally after a source signals flush, the audio selector will wait until
     * the connected sink signals that all samples have been flushed before
     * any other source with the same or lower priority can be selected.
     * If flush_wait is set to false, the selector will immediately signal
     * all samples flushed to the source and if any other source is active,
     * that source will immediately be switched in without sending a flush
     * command to the sink.
     */
    void setFlushWait(AudioSource *source, bool flush_wait);

    /**
     * @brief Resume audio output to the sink
     *
     * This function will be called when the registered audio sink is ready to
     * accept more samples. It is normally only called from a connected sink
     * object.
     */
    virtual void resumeOutput(void);

  protected:
    virtual void allSamplesFlushed(void);
    
  private:
    typedef enum
    {
      STATE_IDLE, STATE_WRITING, STATE_STOPPED, STATE_FLUSHING
    } StreamState;

    class Branch;
    typedef std::map<Async::AudioSource *, Branch *> BranchMap;
    
    BranchMap 	m_branch_map;
    Branch *    m_selected_branch;
    StreamState m_stream_state;
    
    AudioSelector(const AudioSelector&);
    AudioSelector& operator=(const AudioSelector&);
    void selectBranch(Branch *branch);
    Branch *selectedBranch(void) const { return m_selected_branch; }
    void selectHighestPrioActiveBranch(bool clear_if_no_active);
    int branchWriteSamples(const float *samples, int count);
    void branchFlushSamples(void);
    
    friend class Branch;
    
};  /* class AudioSelector */


} /* namespace */

#endif /* ASYNC_AUDIO_SELECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

