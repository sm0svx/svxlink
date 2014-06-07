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

#include <pty.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat()
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>


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
    SquelchPty(void) : master(-1), slave(-1), watch(0) {}

    /**
     * @brief 	Destructor
     */
    ~SquelchPty(void)
    {
      closePty();
    }

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      this->rx_name = rx_name;

      if (!Squelch::initialize(cfg, rx_name))
      {
      	return false;
      }

      if (!cfg.getValue(rx_name, "PTY_PATH", link_path))
      {
        std::cerr << "*** ERROR: Config variable " << rx_name
                  << "/PTY_PATH not set\n";
        return false;
      }

      return openPty();
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
    std::string     link_path;
    int     	    master;
    int             slave;
    Async::FdWatch  *watch;
    std::string     rx_name;

    SquelchPty(const SquelchPty&);
    SquelchPty& operator=(const SquelchPty&);

    /**
     * @brief   Open the PTY
     *
     * Use this function to open the PTY. If the PTY is already open it will
     * be closed first.
     */
    bool openPty(void)
    {
      closePty();

        // Create the master pty
      master = posix_openpt(O_RDWR|O_NOCTTY);

      char *slave_path = NULL;
      if ((master < 0) ||
          (grantpt(master) < 0) ||
          (unlockpt(master) < 0) ||
          (slave_path = ptsname(master)) == NULL)
      {
        closePty();
        return false;
      }

        // Open the slave device to keep it open even if the external script
        // close the device. If we do not do this an I/O error will occur
        // if the script close the device.
      int slave = open(slave_path, O_RDWR|O_NOCTTY);
      if (slave == -1)
      {
        std::cerr << "*** ERROR: Could not open slave PTY " << slave_path <<
          " for receiver " << rx_name << std::endl;
        closePty();
        return false;
      }

        // Watch the master pty
      watch = new Async::FdWatch(master, Async::FdWatch::FD_WATCH_RD);
      assert(watch != 0);
      watch->activity.connect(mem_fun(*this, &SquelchPty::charactersReceived));

        // Create symlink to make the access for user scripts a bit easier
      if (symlink(slave_path, link_path.c_str()) == -1)
      {
        std::cerr << "*** ERROR: Failed to create symlink " << slave_path
             << " -> " << link_path << " for " << rx_name << std::endl;
        closePty();
        return false;
      }

      std::cout << "### " << rx_name << ": Created pseudo tty slave (SQL) "
                << slave_path << " -> " << link_path << "\n";

      return true;
    } /* openPty */

    /**
     * @brief   Close the PTY if it's open
     *
     * Close the PTY if it's open. This function is safe to call even if
     * the PTY is not open or if it's just partly opened.
     */
    void closePty(void)
    {
      if (!link_path.empty())
      {
        unlink(link_path.c_str());
      }
      delete watch;
      watch = 0;
      if (slave >= 0)
      {
        close(slave);
        slave = -1;
      }
      if (master >= 0)
      {
        close(master);
        master = -1;
      }
    } /* closePty */

    /**
     * @brief   Reopen the PTY
     *
     * Try to reopen the PTY. On failure an error message will be printed
     * and the PTY will stay closed.
     */
    bool reopenPty(void)
    {
      if (!openPty())
      {
        std::cerr << "*** ERROR: Failed to reopen the PTY. The squelch will "
                     "be inoperational for " << rx_name << std::endl;
        return false;
      }
      return true;
    } /* reopenPty */

    /**
     * @brief   Called when characters are received on the master PTY
     * @param   w The watch that triggered the event
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
    void charactersReceived(Async::FdWatch *w)
    {
      char cmd;
      int rd = read(w->fd(), &cmd, 1);
      if (rd != 1)
      {
        std::cerr << "*** ERROR: Failed to read master PTY: "
                  << std::strerror(errno) << ". "
                  << "Trying to reopen the PTY.\n";
        reopenPty();
        return;
      }

      if (cmd == 'Z') // The squelch is closed
      {
        setSignalDetected(false);
      }
      if (cmd == 'O') // The squelch is open
      {
        setSignalDetected(true);
      }
    } /* charactersReceived */

};  /* class SquelchPty */


//} /* namespace */

#endif /* SQUELCH_PTY_INCLUDED */



/*
 * This file has not been truncated
 */

