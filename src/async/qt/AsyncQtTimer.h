/**
@file	 AsyncQtTimer.h
@brief   Contains an async library internal class for handling timers
@author  Tobias Blomberg
@date	 2003-03-30

This file contains a class that is used internally by the async library
when creating timers in a Qt application.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2010 Tobias Blomberg

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



#ifndef ASYNC_QT_TIMER_INCLUDED
#define ASYNC_QT_TIMER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Qt Includes
 *
 ****************************************************************************/

#include <QTimer>
#undef emit


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

#include "AsyncTimer.h"


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

namespace Async
{

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
@brief	Async library internal class for creating timer in a Qt application
@author Tobias Blomberg
@date   2003-03-30
*/
class AsyncQtTimer : public QObject
{
  Q_OBJECT
  
  public:
    /**
     * @brief 	Constructor
     * @param 	timer The async timer object to associate the Qt timer to
     */
    AsyncQtTimer(Timer *timer) : timer(timer), qtimer(0)
    {
      qtimer = new QTimer(this);
      qtimer->setSingleShot(timer->type() == Timer::TYPE_ONESHOT);
      qtimer->start(timer->timeout());
      QObject::connect(qtimer, SIGNAL(timeout()),
                       this, SLOT(timerExpired()));
      
    }
    
    /**
     * @brief 	Destructor
     */
    virtual ~AsyncQtTimer(void) {}
  
  protected:
    
  private:
    Timer  *timer;
    QTimer *qtimer;
    
  private slots:
    void timerExpired(void)
    {
      timer->expired(timer);
    }
    
};  /* class AsyncQtTimer */


} /* namespace */

#endif /* ASYNC_QT_TIMER_INCLUDED */



/*
 * This file has not been truncated
 */

