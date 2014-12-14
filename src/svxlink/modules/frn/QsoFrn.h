/**
@file	 QsoFrn.h
@brief   Data for Frn Qso.
@author  Tobias Blomberg / SM0SVX
@date	 2004-06-02

This file contains a class that implementes the things needed for one
Frn Qso.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
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

#ifndef QSO_FRN_INCLUDED
#define QSO_FRN_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
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

namespace Async
{
  class Config;
  class AudioPacer;
  class AudioPassthrough;
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

class MsgHandler;
class EventHandler;
class AsyncTimer;
class ModuleFrn;


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
@brief	Implements the things needed for Frn QSO.
@author Tobias Blomberg
@date   2004-06-02

A class that implementes the things needed for Frn Qso.
*/
class QsoFrn
  : public Async::AudioSink, public Async::AudioSource, public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    QsoFrn(ModuleFrn *module);
  
    /**
     * @brief 	Destructor
     */
    virtual ~QsoFrn(void);
     
    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples could be written.
     * If the returned number of written samples is lower than the count
     * parameter value, the sink is not ready to accept more samples.
     * In this case, the audio source requires sample buffering to temporarily
     * store samples that are not immediately accepted by the sink.
     * The writeSamples function should be called on source buffer updates
     * and after a source output request has been received through the
     * requestSamples function.
     * This function is normally only called from a connected source object.
     */
     virtual int writeSamples(const float *samples, int count);

     /**
      * @brief   Tell the sink to flush the previously written samples
      *
      * This function is used to tell the sink to flush previously written
      * samples. When done flushing, the sink should call the
      * sourceAllSamplesFlushed function.
      * This function is normally only called from a connected source object.
      */
     virtual void flushSamples(void);
 
     /**
      * @brief Resume audio output to the sink
      * 
      * This function will be called when the registered audio sink is ready
      * to accept more samples.
      * This function is normally only called from a connected sink object.
      */
     virtual void resumeOutput(void);

  protected:
     /**
      * @brief The registered sink has flushed all samples
      *
      * This function will be called when all samples have been flushed in the
      * registered sink. If it is not reimplemented, a handler must be set
      * that handle the function call.
      * This function is normally only called from a connected sink object.
      */
     virtual void allSamplesFlushed(void);

  private:

};


//} /* namespace */

#endif /* QSO_FRN_INCLUDED */


/*
 * This file has not been truncated
 */

