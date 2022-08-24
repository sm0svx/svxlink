/**
@file	 LocalRx.h
@brief   A receiver class to handle local receivers
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that handle local analogue receivers. A local
receiver is a receiver that is directly connected to the sound card on the
computer where the SvxLink core is running.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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


#ifndef LOCAL_RX_INCLUDED
#define LOCAL_RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



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

#include "RefCountingPty.h"
#include "LocalRxBase.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class AudioIO;
};


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
@brief	A class to handle local receivers
@author Tobias Blomberg
@date   2004-03-21

This class handle local analogue receivers. A local receiver is a receiver that
is directly connected to the sound card on the computer where the SvxLink core
is running.
*/
class LocalRx : public LocalRxBase
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit LocalRx(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~LocalRx(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void);
    
    /**
     * @brief   Set the receiver frequency
     * @param   fq The frequency in Hz
     */
    virtual void setFq(unsigned fq);

    /**
     * @brief   Set the receiver modulation mode
     * @param   mod The modulation to set (@see Modulation::Type)
     */
    virtual void setModulation(Modulation::Type mod);

  protected:
    /**
     * @brief   Open the audio input source
     * @return  Returns \em true on success or else \em false
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the audio source object is initialized before calling
     * the LocalRxBase::initialize function.
     */
    virtual bool audioOpen(void);

    /**
     * @brief   Close the audio input source
     *
     * This function may be used during the initialization of LocalRxBase so
     * make sure that the audio source object is initialized before calling the
     * LocalRxBase::initialize function.
     */
    virtual void audioClose(void);

    /**
     * @brief   Get the sampling rate of the audio source
     * @return  Returns the sampling rate of the audio source
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the proper sampling rate can be returned before calling
     * the LocalRxBase::initialize function.
     */
    virtual int audioSampleRate(void);

    /**
     * @brief   Get the audio source object
     * @return  Returns an instantiated audio source object
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the audio source object is initialized before calling
     * the LocalRxBase::initialize function.
     */
    virtual Async::AudioSource *audioSource(void);
    
  private:
    Async::Config   &cfg;
    Async::AudioIO  *audio_io;
    RefCountingPty  *ctrl_pty;
    
};  /* class LocalRx */


//} /* namespace */

#endif /* LOCAL_RX_INCLUDED */


/*
 * This file has not been truncated
 */
