/**
@file	 AsyncAudioSelector.h
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2017  Tobias Blomberg / SM0SVX

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
    bool autoSelectEnabled(AudioSource *source);

    /**
     * @brief 	Select one of the previously added audio sources
     * @param 	source The audio source to select. 0 = none selected.
     */
    void selectSource(AudioSource *source);
    
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

    void setDebug(bool debug) { m_debug = debug; }

    /**
     * @brief Resume audio output to the sink
     * 
     * This function must be reimplemented by the inheriting class. It
     * will be called when the registered audio sink is ready to accept
     * more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void);

  protected:
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function should be implemented by the inheriting class. It
     * will be called when all samples have been flushed in the
     * registered sink. If it is not reimplemented, a handler must be set
     * that handle the function call.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void);
    
  private:
    typedef enum
    {
      STATE_IDLE, STATE_ACTIVE, STATE_STOPPED, STATE_FLUSHING
    } State;

    class Branch;
    typedef std::map<Async::AudioSource *, Branch *> BranchMap;
    class NullBranch;
    
    BranchMap 	m_branch_map;
    Branch*     m_selected;
    State       m_state;
    bool        m_debug;
    
    static const char *stateToString(State state);

    AudioSelector(const AudioSelector&);
    AudioSelector& operator=(const AudioSelector&);
    int writeSamples(Branch *branch, float *samples, int count);
    void flushSamples(Branch *branch);
    void selectHighestPrioActiveBranch(void);
    void selectBranch(Branch *branch);
    void setState(State state);
    
    friend class Branch;
    
};  /* class AudioSelector */


} /* namespace */

#endif /* ASYNC_AUDIO_SELECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

