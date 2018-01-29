/**
@file	 HdlcFramer.h
@brief   Create HDLC frames from data bytes
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#ifndef HDLC_FRAMER_INCLUDED
#define HDLC_FRAMER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <stdint.h>
#include <sigc++/sigc++.h>


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
@brief  Create HDLC frames from data bytes
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

This class will create a framed HDLC bitstream from the incoming data bytes.
The generated frames will look like:

  <flag>[...<flag>]<data>[...<data>]<FCS><flag>

Where flag is 01111110 and FCS is the 16 bit Frame Check Sequence (CRC). The
should be at least one data byte in each frame.
*/
class HdlcFramer : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    HdlcFramer(void);

    /**
     * @brief 	Destructor
     */
    ~HdlcFramer(void);

    /**
     * @brief 	Set how many flag bytes that should prefix each frame
     * @param 	cnt The number of flag bytes
     */
    void setStartFlagCnt(size_t cnt) { start_flag_cnt = cnt; }

    /**
     * @brief   Get the number of flag bytes prefixed to each frame
     * @return  Returns the number of flags prefixed to each frame
     */
    size_t startFlagCnt(void) const { return start_flag_cnt; }

    /**
     * @brief   Pack the given data bytes into an HDLC frame
     * @param   frame The data bytes to put into the frame
     */
    void sendBytes(const std::vector<uint8_t> &frame);

    /**
     * @brief   A signal emitted when there are bits to transmit
     * @param   bits The bits to transmit
     */
    sigc::signal<void, const std::vector<bool>&> sendBits;

  private:
    static const size_t DEFAULT_START_FLAG_CNT = 4;

    unsigned  ones;
    bool      prev_was_mark;
    size_t    start_flag_cnt;

    HdlcFramer(const HdlcFramer&);
    HdlcFramer& operator=(const HdlcFramer&);
    void encodeByte(std::vector<bool> &bitbuf, uint8_t data);

};  /* class HdlcFramer */


//} /* namespace */

#endif /* HDLC_FRAMER_INCLUDED */

/*
 * This file has not been truncated
 */
