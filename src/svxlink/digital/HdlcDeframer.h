/**
@file	 HdlcDeframer.h
@brief   Deframe an HDLC bitstream
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

#ifndef HDLC_DEFRAMER_INCLUDED
#define HDLC_DEFRAMER_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <sigc++/sigc++.h>
#include <stdint.h>


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
@brief	Deframe an HDLC bitstream
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

The HDLC deframer will extract complete HDLC frames from the incoming
bitstream. An HDLC frame will start and end with one or more flag bytes,
01111110. The content must be one or more data bytes followed by two CRC bytes
(Frame Check Sequence). The deframed data bytes will be emitted without the CRC
bytes.
*/
class HdlcDeframer : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    HdlcDeframer(void);

    /**
     * @brief 	Destructor
     */
    ~HdlcDeframer(void);

    /**
     * @brief 	Process bitstream
     * @param 	bits The bitstream to process
     */
    void bitsReceived(std::vector<bool> &bits);

    /**
     * @brief 	Signal that is emitted when a complete frame have been received
     * @param 	frame The received frame bytes
     */
    sigc::signal<void, std::vector<uint8_t>&> frameReceived;

  private:
    typedef enum {
      STATE_SYNCHRONIZING, STATE_FRAME_START_WAIT, STATE_RECEIVING
    } State;

    State                 state;
    uint8_t               next_byte;
    uint8_t               bit_cnt;
    std::vector<uint8_t>  frame;
    unsigned              ones;

    HdlcDeframer(const HdlcDeframer&);
    HdlcDeframer& operator=(const HdlcDeframer&);

};  /* class HdlcDeframer */


//} /* namespace */

#endif /* HDLC_DEFRAMER_INCLUDED */

/*
 * This file has not been truncated
 */
