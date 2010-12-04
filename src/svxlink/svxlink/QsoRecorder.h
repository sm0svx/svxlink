/**
@file	 QsoRecorder.h
@brief   The QSO recorder is used to write radio traffic to file
@author  Tobias Blomberg / SM0SVX
@date	 2009-06-06

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

#ifndef QSO_RECORDER_INCLUDED
#define QSO_RECORDER_INCLUDED


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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class AudioSelector;
  class AudioRecorder;
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
@brief	The QSO recorder is used to write radio traffic to file
@author Tobias Blomberg / SM0SVX
@date   2009-06-06
*/
class QsoRecorder
{
  public:
    /**
     * @brief 	Constuctor
     * @param   rec_dir The path to the directory where the recordings
     *                  are placed
     */
    explicit QsoRecorder(const std::string &rec_path);

    /**
     * @brief 	Destructor
     */
    ~QsoRecorder(void);

    /**
     * @brief   Add an audio source to the QSO recorder
     * @param   source The audio source to add
     * @param   prio   The priority to set. Higher numbers give higher priority.
     */
    void addSource(Async::AudioSource *source, int prio);

    /**
     * @brief 	Enable or disable the recorder
     * @param 	enable Set to \em true to enable the recorder or \em false to
     *                 disable it
     */
    void setEnabled(bool enable);

    bool isEnabled(void) const { return (recorder != 0); }


  protected:

  private:
    Async::AudioSelector  *selector;
    Async::AudioRecorder  *recorder;
    std::string           rec_dir;
    std::string           rec_timestamp;

    QsoRecorder(const QsoRecorder&);
    QsoRecorder& operator=(const QsoRecorder&);

};  /* class QsoRecorder */


//} /* namespace */

#endif /* QSO_RECORDER_INCLUDED */



/*
 * This file has not been truncated
 */

