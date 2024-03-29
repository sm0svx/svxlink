/**
@file    DummyLogic.h
@brief   A simple dummy logic core that does not do anything
@author  Tobias Blomberg / SM0SVX
@date    2017-02-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

#ifndef DUMMY_LOGIC_INCLUDED
#define DUMMY_LOGIC_INCLUDED


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

#include "LogicBase.h"


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
@brief  A simple dummy logic core that does not do anything
@author Tobias Blomberg / SM0SVX
@date   2017-02-10

The dummy logic core is just an example of the simplest possible logic core. It
does not do anything but printing debug info when receiving audio from another
logic core. It's primary purpose is to serve as a start for writing new logic
cores.
*/
class DummyLogic : public LogicBase
{
  public:
    /**
     * @brief 	Default constructor
     */
    DummyLogic(void);

    /**
     * @brief   Initialize this logic
     * @param   cfgobj      A previously initialized config object
     * @param   logic_name  The name of the logic core
     * @return  Returns \em true on success or \em false on failure
     */
    virtual bool initialize(Async::Config& cfgobj,
                            const std::string& logic_name) override;

    /**
     * @brief   Get the audio pipe sink used for writing audio into this logic
     * @return  Returns an audio pipe sink object
     */
    virtual Async::AudioSink *logicConIn(void) override
    {
      return m_logic_con_in;
    }

    /**
     * @brief   Get the audio pipe source used for reading audio from this logic
     * @return  Returns an audio pipe source object
     */
    virtual Async::AudioSource *logicConOut(void) override
    {
      return m_logic_con_out;
    }

  protected:
    /**
     * @brief   Destructor
     */
    virtual ~DummyLogic(void) override;

  private:
    Async::AudioDebugger *m_logic_con_in  = nullptr;
    Async::AudioDebugger *m_logic_con_out = nullptr;

};  /* class DummyLogic */


//} /* namespace */

#endif /* DUMMY_LOGIC_INCLUDED */


/*
 * This file has not been truncated
 */
