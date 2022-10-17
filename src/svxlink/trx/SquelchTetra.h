/**
@file	 SquelchTetra.h
@brief   A squelch detector that read squelch state on a PEI response
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2022-09-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2022  Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_TETRA_INCLUDED
#define SQUELCH_TETRA_INCLUDED


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
@brief	An PEI squelch detector
@author Tobias Blomberg / SM0SVX
@date   2022-09-12

This squelch detector reports squelch open/close by a PEI answer
*/
class SquelchTetra : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "TETRA_SQL";

    /*
     *
     */
    explicit SquelchTetra(void) {}

    /*
     * The destructor
     */
    ~SquelchTetra(void) {}

    /*
     * Set the state of the sql from a logic
     */
    void setSql(bool is_open)
    {
      std::cout << "----- SquechTetra::setSql(" << (is_open ? "TRUE" : "FALSE") << "\n";
      setSignalDetected(is_open);
    }


  protected:

    int processSamples(const float *samples, int count)
    {
      return count;
    }

  private:

    SquelchTetra(const SquelchTetra&);
    SquelchTetra& operator=(const SquelchTetra&);

};  /* class SquelchTetra */


//} /* namespace */

#endif /* SQUELCH_TETRA_INCLUDED */



/*
 * This file has not been truncated
 */

