/**
@file    SquelchCombine.h
@brief   A squelch type that combines multiple squelch types
@author  Tobias Blomberg / SM0SVX
@date    2020-07-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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
     * @brief   Reset the squelch detector
     *
     * Reset the squelch so that the detection process starts from
     * the beginning again.
     */
    virtual void reset(void);

    /**
     * @brief   Restart the squelch detector
     *
     * Restarting a squelch is a kind of soft reset. The only thing that
     * happens right now is that the squelch start delay is activated. This
     * function is typically called by a transceiver implementation after
     * turing the transmitter off.
     */
    virtual void restart(void);

  protected:
    /**
     * @brief   Process the incoming samples in the squelch detector
     * @param   samples A buffer containing samples
     * @param   count The number of samples in the buffer
     * @return  Return the number of processed samples
     */
    virtual int processSamples(const float *samples, int count);

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

    void onSquelchOpen(bool is_open);
    bool tokenize(const std::string& expr);
    Node* parseInstExpression(void);
    Node* parseUnaryOpExpression(void);
    Node* parseAndExpression(void);
    Node* parseOrExpression(void);
    Node* parseExpression(void);

};  /* class SquelchCombine */


//} /* namespace */

#endif /* SQUELCH_COMBINE_INCLUDED */

/*
 * This file has not been truncated
 */
