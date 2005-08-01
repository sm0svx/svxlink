/**
@file	 LocalRx.h
@brief   A receiver class to handle local receivers
@author  Tobias Blomberg
@date	 2004-03-21

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


#ifndef LOCAL_RX_INCLUDED
#define LOCAL_RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSerial.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class AudioIO;
  class Timer;
};

class ToneDurationDet;


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

class Vox;
class DtmfDecoder;
class ToneDetector;


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
@date   2004-03-21

A_detailed_class_description

\include Template_demo.cpp
*/
class LocalRx : public Rx
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit LocalRx(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~LocalRx(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
    
    /**
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    void mute(bool do_mute);
    
    /**
     * @brief 	Check the squelch status
     * @return	Return \em true if the squelch is open or else \em false
     */
    bool squelchIsOpen(void) const;
    
    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    bool addToneDetector(int fq, int bw, int required_duration);
    

  protected:
    
  private:
    typedef enum
    {
      SQL_DET_UNKNOWN,
      SQL_DET_VOX,
      SQL_DET_CTCSS,
      SQL_DET_SERIAL
    } SqlDetType;
    
    static const int  	    NPOLES = 4;
    static const int  	    NZEROS = 4;
    
    Async::AudioIO    	    *audio_io;
    bool      	      	    is_muted;
    Vox       	      	    *vox;
    DtmfDecoder       	    *dtmf_dec;
    ToneDetector      	    *ctcss_det;
    int       	      	    ctcss_fq;
    SqlDetType	      	    sql_up_det;
    SqlDetType	      	    sql_down_det;
    bool      	      	    sql_is_open;
    Async::Serial     	    *serial;
    Async::Serial::InPin    sql_pin;
    bool      	      	    sql_pin_act_lvl;
    Async::Timer      	    *sql_pin_poll_timer;
    float     	      	    xv[NZEROS+1];
    float     	      	    yv[NPOLES+1];
    std::list<ToneDurationDet*>  tone_detectors;
    
    void voxSqlOpen(bool is_open);
    void activatedCtcss(bool is_activated);
    SqlDetType sqlDetStrToEnum(const std::string& sql_det_str);
    int audioRead(short *samples, int count);
    void sqlPinPoll(Async::Timer *t);
    void resetHighpassFilter(void);
    void highpassFilter(short *samples, int count);

};  /* class LocalRx */


//} /* namespace */

#endif /* LOCAL_RX_INCLUDED */



/*
 * This file has not been truncated
 */

