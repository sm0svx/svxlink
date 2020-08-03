/**
@file    SquelchCombine.h
@brief   A squelch type that combines multiple squelch types
@author  Tobias Blomberg / SM0SVX
@date    2020-07-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_COMBINE_INCLUDED
#define SQUELCH_COMBINE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <deque>


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

#include <Squelch.h>


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
@brief  A squelch type that combines multiple squelch types
@author Tobias Blomberg / SM0SVX
@date   2020-07-26

This squelch type is really not a squelch on its own but rather a way to
combine multiple squelch types using a logical expression. An example would be
to use both a CTCSS squelch and a level squelch at the same time. The thought
behind that would be that the level squelch react to higher signal levels and
the CTCSS squelch opens for signals with lower signal strength. Since the level
squelch is much quicker the combined squelch will open quicker for strong
signals while still being able to handle weaker signals without letting
weaker interference through.

The configuration would look something like the setup below.

[Rx1:CTCSS]
SQL_DET=CTCSS
CTCSS_FQ=77.0
CTCSS_SNR_OFFSET=-3.22

[Rx1:SIGLEV]
SQL_DET=SIGLEV
SIGLEV_RX_NAME=Rx1
SIGLEV_OPEN_THRESH=40
SIGLEV_CLOSE_THRESH=2

[Rx1]
...
SQL_DET=COMBINE
SQL_COMBINE=Rx1:CTCSS | Rx1:SIGLEV
...
*/
class SquelchCombine : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "COMBINE";

    /**
     * @brief   Default constructor
     */
    SquelchCombine(void);

    /**
     * @brief   Disallow copy construction
     */
    SquelchCombine(const SquelchCombine&) = delete;

    /**
     * @brief   Disallow assignment
     */
    SquelchCombine& operator=(const SquelchCombine&) = delete;

    /**
     * @brief   Destructor
     */
    virtual ~SquelchCombine(void);

    /**
     * @brief   Initialize the squelch detector
     * @param   cfg A previsously initialized config object
     * @param   name The name of the config section
     * @return  Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config& cfg, const std::string& name);

    /**
     * @brief   Set the squelch start delay
     * @param   delay The delay in milliseconds to set
     *
     * Use this function to set the squelch startup delay. The delay is
     * specified in milliseconds. When a value > 0 is specified, the squelch
     * will not open within this time after the Squelch::reset function has
     * been called.
     */
    virtual void setStartDelay(int delay);

    /**
     * @brief  Set the time the squelch should hang open after squelch close
     * @param  hang The number of milliseconds to hang
     */
    virtual void setHangtime(int hang);

    /**
     * @brief   Set extended time squelch hang open after squelch close
     * @param   hang The number of milliseconds to hang
     *
     * This is the squelch hangtime that is used in extended hangtime mode.
     */
    virtual void setExtendedHangtime(int hang);

    /**
     * @brief   Choose if extended hangtime mode should be active or not
     * @param   enable Set to \em true to enable or \em false to disable
     *
     * Using extended hangtime mode it is possible to temporarily prolong the
     * time that the squelch will hang open. This can be of use in low signal
     * strength conditions for example. The switch to extended hangtime is not
     * handled by this class so the condition for switching must be handled by
     * the user of this class.
     */
    virtual void enableExtendedHangtime(bool enable);

    /**
     * @brief   Set the time a squelch open should be delayed
     * @param   delay The delay in milliseconds
     */
    virtual void setDelay(int delay);

    /**
     * @brief   Set the maximum time the squelch is allowed to stay open
     * @param   timeout The squelch timeout in seconds
     */
    virtual void setSqlTimeout(int timeout);

    /**
     * @brief   Reset the squelch detector
     *
     * Reset the squelch so that the detection process starts from
     * the beginning again.
     */
    virtual void reset(void);

    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     */
    virtual int writeSamples(const float *samples, int count);

    /**
     * @brief   Get the state of the squelch
     * @return  Return \em true if the squelch is open, or else \em false
     */
    virtual bool isOpen(void) const { return m_is_open; }

  protected:

  private:
    typedef std::deque<std::string> Tokens;
    class Node;
    class LeafNode;
    class UnaryOpNode;
    struct NegationOpNode;
    class BinaryOpNode;
    struct OrOpNode;
    struct AndOpNode;

    Tokens  m_tokens;
    Node*   m_comb    = nullptr;
    bool    m_is_open = false;

    bool tokenize(const std::string& expr);
    Node* parseInstExpression(void);
    Node* parseUnaryOpExpression(void);
    Node* parseAndExpression(void);
    Node* parseOrExpression(void);
    Node* parseExpression(void);
    void onSquelchOpen(bool is_open);

};  /* class SquelchCombine */


//} /* namespace */

#endif /* SQUELCH_COMBINE_INCLUDED */

/*
 * This file has not been truncated
 */
