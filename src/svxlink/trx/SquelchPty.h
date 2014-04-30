/**
@file	 SquelchPty.h
@brief   A squelch detector that read squelch state on a pseudo Tty
@author  Tobias Blomberg / SM0SVX & Steve Koehler / DH1DM & Adi Bier / DL1HRC
@date	 2014-03-21

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

#include <iostream>
#include <string>
#include <pty.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat()


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

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

This squelch detector read the state of an external hardware squelch through
a pin in the serial port. The pins that can be used are CTS, DSR, DCD and RI.
*/
class SquelchPty : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SquelchPty(void) : slave(0) {}

    /**
     * @brief 	Destructor
     */
    ~SquelchPty(void)
    {
      master=0;
      unlink(slave);
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

      std::string serial_port;
      if (!cfg.getValue(rx_name, "SERIAL_PORT", serial_port))
      {
	    std::cerr << "*** ERROR: Config variable " << rx_name
	              << "/PTY_PORT not set\n";
	    return false;
      }

      unlink(serial_port.c_str());

      // creating the pty master
      master = posix_openpt(O_RDWR|O_NOCTTY);
      if (master < 0 ||
          grantpt(master) < 0 ||
          unlockpt(master) < 0 ||
          (slave = ptsname(master)) == NULL)
      {
	    master = 0;
	    return false;
      }

      if ((fd = open(slave, O_RDONLY|O_NOCTTY)) == -1)
      {
        std::cerr << "*** ERROR: Could not open event device " << slave <<
                " specified in " << rx_name << std::endl;
        return false;
      }

      // watch the master pty
      watch = new Async::FdWatch(master, Async::FdWatch::FD_WATCH_RD);
      assert(watch != 0);
      watch->setEnabled(true);
      watch->activity.connect(mem_fun(*this, &SquelchPty::charactersReceived));

      // create symlink to make the access for user scripts a bit easier
      if (symlink(slave, serial_port.c_str()) == -1)
      {
        std::cerr << "*** ERROR: creating symlink " << slave
             << " -> " << serial_port << std::endl;
        master = 0;
        return false;
      }

      // the created device is ptsname(master)
      std::cout << "created pseudo tty master (SQL) "
                << slave << " -> " << serial_port << "\n";

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
    int     	    master;
    int			    fd;
    char            *slave;
    Async::FdWatch	*watch;

    SquelchPty(const SquelchPty&);
    SquelchPty& operator=(const SquelchPty&);

    /** We declare a new and very easy squelch protocol here to interact between
      * the NHRC-x controller and SvxLink over a perl|python|xxx-script and the
      * linux pseudo-tty's:
      * Z -> SQL is closed
      * O -> SQL is open
    **/
    void charactersReceived(Async::FdWatch *w)
    {
      char buf[1];
      int rd = read(w->fd(), buf, 1);
      if (rd < 0)
      {
        std::cerr << "*** ERROR: reading characters from " <<
              "PTY-squelch device." << std::endl;
        return;
      }

      if (buf[0] == 'Z')
      {
        setSignalDetected(false);
      }
      if (buf[0] == 'O')
      {
        setSignalDetected(true);
      }
    } /* SquelchPty::charactersReceived */

};  /* class SquelchPty */


//} /* namespace */

#endif /* SQUELCH_PTY_INCLUDED */



/*
 * This file has not been truncated
 */

