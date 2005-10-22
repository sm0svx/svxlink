/**
@file	 MsgHandler.cpp
@brief   Handling of playback of audio clips
@author  Tobias Blomberg / SM0SVX
@date	 2005-10-22

This file contains an object that handles the playback of audio clips.

\verbatim
<A brief description of the program or library this file belongs to>
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



#ifndef MSG_HANDLER_INCLUDED
#define MSG_HANDLER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <list>
#include <map>

#include <sigc++/signal_system.h>



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

class QueueItem;



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


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
@brief	Handling of playback of audio clips
@author Tobias Blomberg / SM0SVX
@date   2005-10-22

This class handles the playback of audio clips.
*/
class MsgHandler : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     * @param	sample_rate The sample rate of the playback system
     */
    explicit MsgHandler(int sample_rate);

    /**
     * @brief 	Destructor
     */
    ~MsgHandler(void);
    
    /**
     * @brief 	Play a file
     * @param 	path The full path to the file to play
     */
    void playFile(const std::string& path);
    
    /**
     * @brief 	Play the given number of milliseconds of silence
     * @param 	length The length in milliseconds of the silence
     */
    void playSilence(int length);
    
    /**
     * @brief 	Play a sinus tone
     * @param 	fq The frequency of the tone to play
     * @param 	amp The amplitude of the tone to play
     * @param 	length The length in milliseconds of the tone to play
     */
    void playTone(int fq, int amp, int length);
    
    /**
     * @brief 	Stop or start output of samples
     * @param 	is_full \em true to stop output or \em false to start output
     */
    void writeBufferFull(bool is_full);
    
    /**
     * @brief 	Check if a message is beeing written
     * @return	Return \em true if a message is beeing written
     */
    bool isWritingMessage(void) const { return !msg_queue.empty(); }
    
    /**
     * @brief 	Clear all messages
     */
    void clear(void);
    
    /**
     * @brief 	Mark the beginning of a block of messages
     *
     * All the playXxx functions will be queued until the end() method is
     * called. Multiple begin/end can be nested.
     */
    void begin(void);
    
    /**
     * @brief 	Mark the end of a block of messages
     *
     * All the playXxx functions that have been previously queued will be
     * executed.
     */
    void end(void);    
    
    /**
     * @brief 	A signal that is emitted to write audio
     * @param 	samples A buffer containing the samples to write
     * @param 	count The number of samples to write
     * @return	The slot should return the number of samples written
     */
    SigC::Signal2<int, short*, int> writeAudio;
    
    /**
     * @brief 	A signal that is emitted when all messages has been written
     */
    SigC::Signal0<void>       	    allMsgsWritten;
    
    
  private:
    std::list<QueueItem*>   msg_queue;
    int			    sample_rate;
    int      	      	    nesting_level;
    bool      	      	    pending_play_next;
    
    MsgHandler(const MsgHandler&);
    MsgHandler& operator=(const MsgHandler&);
    void addItemToQueue(QueueItem *item);
    void playMsg(void);
    void writeSamples(void);

}; /* class MsgHandler */


//} /* namespace */

#endif /* MSG_HANDLER_INCLUDED */



/*
 * This file has not been truncated
 */
