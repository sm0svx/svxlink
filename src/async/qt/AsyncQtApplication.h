/**
@file	 AsyncQtApplication.h
@brief   The core class for writing asyncronous cpp applications.
@author  Tobias Blomberg
@date	 2003-03-16

This file contains the AsyncQtApplication class which is the core of an
application that use the Async classes in a Qt application.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

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

/** @example AsyncQtApplication_demo.cpp
An example of how to use the Async::QtApplication class
*/


#ifndef ASYNC_QT_APPLICATION_INCLUDED
#define ASYNC_QT_APPLICATION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <QObject>
#include <QApplication>
#undef emit

#include <utility>
#include <map>
#include <set>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>


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

class QSocketNotifier;
class AsyncQtTimer;


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

class DnsLookup;

/*
 *----------------------------------------------------------------------------
 * Macro:   
 * Purpose: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 * Bugs:    
 *----------------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------------
 * Type:    
 * Purpose: 
 * Members: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 *----------------------------------------------------------------------------
 */


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
@brief	An application class for writing GUI applications in Qt.
@author Tobias Blomberg
@date   2003-03-16

This class is used when writing Qt applications. It should be one of the first
things done in the \em main function of your application. Use the
Async::QtApplication class in place of the QApplication class.

In the application you do not have to use the Async classes for timers and file
descriptor watches. It is perfecly legal to use the Qt variants (QTimer and
QSocketNotifier). You can mix them as much as you like.

\include AsyncQtApplication_demo.cpp
*/
class QtApplication : public QApplication, public Application
{
  Q_OBJECT
    
  public:
    /**
     * @brief Constructor
     *
     * The two arguments typically are the arguments given to the \em main
     * function.
     * @param argc  The number of command line arguments + 1
     * @param argv  An array containing the commandline arguments
     */
    QtApplication(int &argc, char **argv);

    /**
     * @brief Destructor
     */
    virtual ~QtApplication(void);
    
    /**
     * @brief Execute the application main loop
     *
     * When this member function is called the application core will enter the
     * main loop. It will not exit from this loop until the
     * Async::Application::quit method is called.
     */
    void exec(void);
    
  public slots:
    /**
     * @brief Exit the application main loop
     *
     * This function should be called to exit the application core main loop.
     */
    void quit(void);
    
  protected:
    
  private:
    typedef std::pair<Async::FdWatch*, QSocketNotifier*>  FdWatchMapItem;
    typedef std::map<int, FdWatchMapItem> 	      	  FdWatchMap;
    typedef std::map<Timer *, AsyncQtTimer *>         	  TimerMap;
    
    FdWatchMap  rd_watch_map;
    FdWatchMap  wr_watch_map;
    TimerMap  	timer_map;
    
    void addFdWatch(FdWatch *fd_watch);
    void delFdWatch(FdWatch *fd_watch);
    void addTimer(Timer *timer);
    void delTimer(Timer *timer);
    DnsLookupWorker *newDnsLookupWorker(const DnsLookup& lookup);

  private slots:
    void rdFdActivity(int socket);
    void wrFdActivity(int socket);
    
};  /* class QtApplication */


} /* namespace */

#endif /* ASYNC_QT_APPLICATION_INCLUDED */



/*
 * This file has not been truncated
 */

