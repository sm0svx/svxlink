/**
@file	 AsyncSampleFifo.h
@brief   A FIFO for handling samples
@author  Tobias Blomberg
@date	 2004-03-07

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

/** @example Template_demo.cpp
An example of how to use the Template class
*/


#ifndef SAMPLE_FIFO_INCLUDED
#define SAMPLE_FIFO_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2003-04-

A_detailed_class_description

\include SampleFifo_demo.cpp
*/
class SampleFifo : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit SampleFifo(int fifo_size);
  
    /**
     * @brief 	Destructor
     */
    ~SampleFifo(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    int addSamples(short *samples, int count);
    void stopOutput(bool stop);
    bool empty(void) const { return tail == head; }
    bool full(void) const;
    int samplesInFifo(void) const;
    void writeBufferFull(bool is_full);
    void setOverwrite(bool overwrite) { do_overwrite = overwrite; }
    bool overwrite(void) const { return do_overwrite; }
    int readSamples(short *samples, int count);
    void clear(void) { tail = head; }

     
    /**
     * @brief 	A signal that is emitted when thefifo is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    SigC::Signal1<void, bool> fifoFull;
    
    SigC::Signal2<int, short *, int>  	    writeSamples;
    SigC::Signal0<void>     	      	    allSamplesWritten;
    
    
  protected:
    
  private:
    static const int  MAX_WRITE_SIZE = 800;
    
    short *fifo;
    int   fifo_size;
    int   head, tail;
    bool  is_stopped;
    bool  do_overwrite;
    
    void writeSamplesFromFifo(void);

};  /* class SampleFifo */


} /* namespace */

#endif /* SAMPLE_FIFO_INCLUDED */


/*
 * This file has not been truncated
 */

