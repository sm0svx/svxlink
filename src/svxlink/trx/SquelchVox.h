/**
@file	 SquelchVox.h
@brief   Implementes a voice activated squelch
@author  Tobias Blomberg
@date	 2004-02-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_VOX_INCLUDED
#define SQUELCH_VOX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


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
@brief	Implements an audio level triggered squelch
@author Tobias Blomberg
@date   2004-02-15
*/
class SquelchVox : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "VOX";

    /**
     * @brief 	Default constuctor
     */
    explicit SquelchVox(void);

    /**
     * @brief 	Destructor
     */
    ~SquelchVox(void);

    /**
     * @brief 	Initialize the VOX
     * @param 	cfg A previously initialize config object
     * @param   rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name);

    /**
     * @brief 	Set the VOX threshold
     * @param 	thresh The threshold to set
     */
    void setVoxThreshold(short thresh);

    /**
     * @brief 	Reset the squelch detector
     *
     * Reset the squelch so that the detection process starts from
     * the beginning again.
     */
    virtual void reset(void);

  protected:
    int processSamples(const float *samples, int count);

  private:
    float   *buf;
    int     buf_size;
    int     head;
    double  sum;
    double  up_thresh;
    double  down_thresh;

};  /* class SquelchVox */


//} /* namespace */

#endif /* SQUELCH_VOX_INCLUDED */



/*
 * This file has not been truncated
 */

