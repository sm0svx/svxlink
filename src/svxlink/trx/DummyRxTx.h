/**
@file	 DummyRxTx.h
@brief   Dummy RX and TX classes
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-10

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

#ifndef DUMMY_RX_TX_INCLUDED
#define DUMMY_RX_TX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


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

#include "Rx.h"
#include "Tx.h"


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
@brief	A dummy transmitter
@author Tobias Blomberg / SM0SVX
@date   2013-05-10

This dummy transmitter can be used instead of a real transmitter object if
a real transmitter is not needed. Using a dummy object instead of using a
NULL pointer may make the code simpler since checks for NULL pointer can
be omitted.
*/
class DummyTx : public Tx
{
  public:
    DummyTx(const std::string& name) : Tx(name) {}
    virtual ~DummyTx(void) {}
    virtual bool initialize(void) { return true; }
    virtual void setTxCtrlMode(TxCtrlMode mode) {}
    virtual int writeSamples(const float *samples, int count) { return count; }
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }
};


/**
@brief	A dummy receiver
@author Tobias Blomberg / SM0SVX
@date   2013-05-10

This dummy receiver can be used instead of a real receiver object if
a real receiver is not needed. Using a dummy object instead of using a
NULL pointer may make the code simpler since checks for NULL pointer can
be omitted.
*/
class DummyRx : public Rx
{
  public:
    DummyRx(Async::Config &cfg, const std::string &name) : Rx(cfg, name) {}
    virtual ~DummyRx(void) {}
    virtual bool initialize(void) { return Rx::initialize(); }
    virtual void setMuteState(Rx::MuteState new_mute_state) {}
    virtual void reset(void) {}
    virtual void resumeOutput(void) {}
    virtual void allSamplesFlushed(void) {}
};



//} /* namespace */

#endif /* DUMMY_RX_TX_INCLUDED */



/*
 * This file has not been truncated
 */
