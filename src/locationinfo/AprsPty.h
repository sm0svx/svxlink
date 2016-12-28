/**
@file	 AprsPty.h
@brief   A interface to receive Aprs messages over a pseudo Tty device
         to send the information into the Aprs network
@author  Tobias Blomberg / SM0SVX & Steve Koehler / DH1DM & Adi Bier / DL1HRC
@date	 2014-08-05

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014  Tobias Blomberg / SM0SVX

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

#ifndef APRS_PTY_INCLUDED
#define APRS_PTY_INCLUDED


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

#include <AsyncPty.h>


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

using namespace sigc;


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
@brief   A interface to receive Aprs messages over a pseudo Tty device
         to send the information into the Aprs network
@author  Tobias Blomberg / SM0SVX & Steve Koehler / DH1DM & Adi Bier / DL1HRC
@date	 2014-08-05

This interface receive messages through a PTY device.  This
can be used to gateway Aprs messages to the Aprs network
*/
class AprsPty
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AprsPty(void) : pty(0) {}

    /**
     * @brief 	Destructor
     */
    ~AprsPty(void)
    {
      delete pty;
    }

    /**
     * @brief 	Initialize the AprsPty device
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(const std::string link_path)
    {
      pty = new Async::Pty(link_path);
      if (pty == 0)
      {
        return false;
      }
      pty->dataReceived.connect(sigc::mem_fun(*this, &AprsPty::dataReceived));
      return pty->open();
    }

    /*
    */
    sigc::signal<void, std::string> messageReceived;

  protected:

  private:
    Async::Pty      *pty;
    std::string     message;

    AprsPty(const AprsPty&);
    AprsPty& operator=(const AprsPty&);

    /**
     * @brief   Called when a command is received on the master PTY
     * @param   message The received message
     *
     * All other commands are ignored.
     */
    void dataReceived(const void *buf, size_t count)
    {
      const char *ptr = reinterpret_cast<const char*>(buf);
      for (size_t i=0; i<count; ++i)
      {
        const char &digit = *ptr++;
        if (digit != '\n')
        {
          message += digit;
        }
        else
        {
          messageReceived(message);
          message = "";
        }
      }
    } /* dataReceived */

};  /* class AprsPty */


//} /* namespace */

#endif /* APRS_PTY_INCLUDED */



/*
 * This file has not been truncated
 */

