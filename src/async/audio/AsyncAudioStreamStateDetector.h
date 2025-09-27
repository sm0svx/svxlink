/**
@file	 AsyncAudioStreamStateDetector.h
@brief   This file contains a class that just passes the audio through and
         fires an event when the stream state changes.
@author  Tobias Blomberg / SM0SVX
@date	 2008-05-30

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2025 Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_STREAM_STATE_DETECTOR_INCLUDED
#define ASYNC_AUDIO_STREAM_STATE_DETECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioPassthrough.h>


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
 * @brief Emit a signal when the state changes for the audio stream
 *
 * A class that just passes the audio through and emit signals when the
 * stream state changes. The audio stream may be in three different states. In
 * each of these states two booleans are set up.
 *
 *   State    Active  Idle
 *   IDLE     false   true
 *   ACTIVE   true    false
 *   FLUSHING false   false
 *
 * The stream state start at IDLE.
 * When the audio stream source write a sample the state go to ACTIVE.
 * When the audio stream source flush the stream the state go to FLUSHING.
 * When the audio sink indicate that all samples has been flushed the state go
 * to IDLE again.
 */
class AudioStreamStateDetector
  : public AudioPassthrough, virtual public sigc::trackable
{
  public:
    /**
     * @brief An enum representing the stream state
     */
    enum class State
    {
      IDLE,     ///< There are no samples left in the stream
      ACTIVE,   ///< Samples have been written to the stream
      FLUSHING  ///< The stream source have requested stream flush
    };

    /**
     * @brief 	Default constuctor
     */
    AudioStreamStateDetector(void) {}

    /**
     * @brief 	Destructor
     */
    virtual ~AudioStreamStateDetector(void) override {}

    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count 	The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count) override
    {
      setState(State::ACTIVE);
      return AudioPassthrough::writeSamples(samples, count);
    }

    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void) override
    {
      setState(State::FLUSHING);
      AudioPassthrough::flushSamples();
    }

    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void) override
    {
      setState(State::IDLE);
      AudioPassthrough::allSamplesFlushed();
    }

    /**
     * @brief 	Check if the steam is idle or not
     * @returns Returns \em true if the stream is idle or \em false if it's not
     */
    bool isIdle(void) const { return (m_state == State::IDLE); }

    /**
     * @brief 	Check if the steam is active or not
     * @returns Returns \em true if the stream is active or \em false if it's
     *        	not
     */
    bool isActive(void) const { return (m_state == State::ACTIVE); }

    /**
     * @brief  Check if the steam is flushing or not
     * @returns Returns \em true if the stream is flushing or \em false if
     *         it's not
     */
    bool isFlushing(void) const { return (m_state == State::FLUSHING); }

    /**
     * @brief   Get the stream state
     * @returns Returns the state of the audio stream
     */
    State state(void) const { return m_state; }

    /**
     * @brief A signal that is emitted when the stream state changes
     * @param is_active Is \em true if the stream is active
     * @param is_idle 	Is \em  true if the stream is idle
     */
    sigc::signal<void, bool, bool> sigStreamStateChanged;

    /**
     * @brief A signal that is emitted when stream activity state changes
     * @param is_active Is \em true if the stream is active
     */
    sigc::signal<void, bool> sigStreamIsActive;

    /**
     * @brief A signal that is emitted when stream idle state changes
     * @param is_idle Is \em  true if the stream is idle
     */
    sigc::signal<void, bool> sigStreamIsIdle;

  private:
    AudioStreamStateDetector(const AudioStreamStateDetector&);
    AudioStreamStateDetector& operator=(const AudioStreamStateDetector&);

    struct StateMapValue
    {
      bool is_active;
      bool is_idle;
    };
    using StateMap = std::map<State, StateMapValue>;

    const StateMap STATE_MAP {
      {State::IDLE,     {false, true} },
      {State::ACTIVE,   {true, false} },
      {State::FLUSHING, {false, false}}
    };
    State m_state {State::IDLE};

    void setState(State new_state)
    {
      if (new_state == m_state)
      {
        return;
      }

      auto& old_mapping = STATE_MAP.at(m_state);
      auto& new_mapping = STATE_MAP.at(new_state);
      m_state = new_state;

      if (new_mapping.is_active != old_mapping.is_active)
      {
        sigStreamIsActive(new_mapping.is_active);
      }

      if (new_mapping.is_idle != old_mapping.is_idle)
      {
        sigStreamIsIdle(new_mapping.is_idle);
      }

      sigStreamStateChanged(new_mapping.is_active, new_mapping.is_idle);
    }
}; /* AudioStreamStateDetector */


} /* namespace */

#endif /* ASYNC_AUDIO_STREAM_STATE_DETECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

