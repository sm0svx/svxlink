/**
@file	 PtyDtmfDecoder.h
@brief   This file contains a class that add support for the Pty interface
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
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


#ifndef PTY_DTMF_DECODER_INCLUDED
#define PTY_DTMF_DECODER_INCLUDED


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

#include "HwDtmfDecoder.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class FdWatch;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{

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
 * @brief   This class add support for the Pty interface board
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2014-03-21
 *
 * This class implements support for the hardware DTMF decoder on the
 * Pty SvxLink interface board.
 */
class PtyDtmfDecoder : public HwDtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    PtyDtmfDecoder(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Destructor
     */
    virtual ~PtyDtmfDecoder(void);

    /**
     * @brief 	Initialize the DTMF decoder
     * @returns Returns \em true if the initialization was successful or
     *          else \em false.
     *
     * Call this function to initialize the DTMF decoder. It must be called
     * before using it.
     */
    virtual bool initialize(void);

  protected:

  private:
    int     	    master;
    char            *slave;
    int			    fd;
    Async::FdWatch	*watch;

    void charactersReceived(Async::FdWatch *w);

};  /* class PtyDtmfDecoder */


//} /* namespace */

#endif /* PTY_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

