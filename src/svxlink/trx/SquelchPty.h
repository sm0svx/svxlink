/**
@file	 SquelchPty.h
@brief   A squelch detector that read squelch state on a pseudo Tty
@author  Tobias Blomberg / SM0SVX & Steve Koehler / DH1DM & Adi Bier / DL1HRC
@date	 2014-05-21

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

#ifndef SQUELCH_PTY_INCLUDED
#define SQUELCH_PTY_INCLUDED


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
#include "Squelch.h"


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
@brief	External squelch detector over a pseudo tty
@author Tobias Blomberg / SM0SVX
@date   2014-03-21

This squelch detector read the state of the squelch through a PTY device.  This
can be used to interface SvxLink to any custom hardware using an interface
script.
*/
class SquelchPty : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SquelchPty(void) : pty(0) {}

    /**
     * @brief 	Destructor
     */
    ~SquelchPty(void)
    {
      if (pty != 0) pty->destroy();
    }

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      if (!Squelch::initialize(cfg, rx_name))
      {
      	return false;
      }

      std::string link_path;
      if (!cfg.getValue(rx_name, "PTY_PATH", link_path))
      {
        std::cerr << "*** ERROR: Config variable " << rx_name
                  << "/PTY_PATH not set\n";
        return false;
      }

      pty = RefCountingPty::instance(link_path);
      if (pty == 0)
      {
        return false;
      }
      pty->dataReceived.connect(
          sigc::mem_fun(*this, &SquelchPty::dataReceived));
      return true;
    }

  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    int processSamples(const float *samples, int count)
    {
      return count;
    }

  private:
    RefCountingPty  *pty;

    SquelchPty(const SquelchPty&);
    SquelchPty& operator=(const SquelchPty&);

    /**
     * @brief   Called when data is received on the master PTY
     * @param   buf A buffer containing the received data
     * @param   count The number of bytes in the buffer
     *
     * We implement a very basic squelch protocol here to interface the
     * actual squelch detector to SvxLink through a (perl|python|xxx)-script
     * using a pseudo-tty (PTY). The following commands are understood:
     *
     *   'O' -> Squelch is open
     *   'Z' -> Squelch is closed
     *
     * All other commands are ignored.
     */
    void dataReceived(const void *buf, size_t count)
    {
      const char *ptr = reinterpret_cast<const char*>(buf);
      for (size_t i=0; i<count; ++i)
      {
        const char &cmd = *ptr++;
        switch (cmd)
        {
          case 'O': // The squelch is open
            setSignalDetected(true);
            break;
          case 'Z': // The squelch is closed
            setSignalDetected(false);
            break;
        }
      }
    } /* dataReceived */

};  /* class SquelchPty */


//} /* namespace */

#endif /* SQUELCH_PTY_INCLUDED */



/*
 * This file has not been truncated
 */

