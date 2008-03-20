/**
@file	 AsyncAudioSelector.h
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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
    
    
  protected:
    
  private:
    class Branch;
    typedef std::map<Async::AudioSource *, Branch *> BranchMap;
    class NullBranch;
    
    BranchMap 	branch_map;
    NullBranch	*null_branch;
    
    AudioSelector(const AudioSelector&);
    AudioSelector& operator=(const AudioSelector&);
    void selectBranch(Branch *branch);
    
    friend class Branch;
    
};  /* class AudioSelector */


} /* namespace */

#endif /* ASYNC_AUDIO_SELECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

