/**
@file	 DtmfDigitHandler.h
@brief   Handle incoming DTMF digits to form commands
@author  Tobias Blomberg / SM0SVX
@date	 2013-08-20

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

#ifndef DTMF_DIGIT_HANDLER_INCLUDED
#define DTMF_DIGIT_HANDLER_INCLUDED


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

#include <AsyncTimer.h>


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
@brief	Keep track of received DTMF digits to form commands
@author Tobias Blomberg / SM0SVX
@date   2013-08-20

This class will keep track of received DTMF digits and put them together into a
command. A complete command is considered to be received when a # is received.
The maximum command length is 20 digits. If the maximum length is exceeded,
no more digits will be added to the digit buffer.
If no digits are entered for 10 seconds, the command will time out and the
digit buffer will be cleared.
*/
class DtmfDigitHandler : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constructor
     */
    DtmfDigitHandler(void);

    /**
     * @brief 	Destructor
     */
    ~DtmfDigitHandler(void);

    /**
     * @brief 	Call this funtion to add new digits
     * @param 	digit The DTMF digit to add (0-9, A-D, * and #)
     */
    void digitReceived(char digit);

    /**
     * @brief   Reset the digit handler
     *
     * When resetting the digit handler the digit buffer will be cleard,
     * the timeout timer will be stopped and anti flutter mode will be
     * disabled if it's enabled.
     */
    void reset(void);

    /**
     * @brief   Return the contents of the DTMF digit buffer
     * @returns Returns the DTMF digits received so far
     *
     * Use this function to read the digits that are stored in the buffer.
     * The buffer will not be cleared when reading. To clear it, use the reset
     * function. The buffer will however be automatically cleared whenever
     * the commandComplete signal is emitted so if this functions is called
     * as a response to that, reset() does not have to be called.
     */
    std::string command(void) const { return received_digits; } 

    /**
     * @brief   Force command complete
     *
     * Normally a DTMF command is considered complete when a # is received.
     * Sometimes one might like to force a command complete event. This
     * function is used to do just that.
     * If the digit buffer is empty, no event will be generated.
     */
    void forceCommandComplete(void);

    /**
     * @brief   Find out if anti flutter is active or not
     * @returns Returns \em true if anti flutter is active or else \em false.
     */
    bool antiFlutterActive(void) const { return anti_flutter; }

    /**
     * @brief   Signal that is emitted when a complete command has been received
     */
    sigc::signal<void> commandComplete;

  private:
    Async::Timer  cmd_tmo_timer;
    std::string   received_digits;
    bool          anti_flutter;
    char          prev_digit;

    DtmfDigitHandler(const DtmfDigitHandler&);
    DtmfDigitHandler& operator=(const DtmfDigitHandler&);
    void cmdTimeout(void);
    
};  /* class DtmfDigitHandler */


//} /* namespace */

#endif /* DTMF_DIGIT_HANDLER_INCLUDED */



/*
 * This file has not been truncated
 */
