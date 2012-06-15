/**
 * @file    AsyncCppApplication.h
 * @brief   The core class for writing asyncronous cpp applications.
 * @author  Tobias Blomberg
 * @date    2003-03-16
 *
 * This file contains the AsyncCppApplication class which is the core of an
 * application that use the Async classes in a non-GUI application.
 *
 * \verbatim
 * Async - A library for programming event driven applications
 * Copyright (C) 2003  Tobias Blomberg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * \endverbatim
 */


#ifndef ASYNC_CPP_APPLICATION_INCLUDED
#define ASYNC_CPP_APPLICATION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sigc++/sigc++.h>

#include <map>
#include <utility>


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
* @brief An application class for writing non GUI applications.
*/
class CppApplication : public Application
{
  public:
    /**
     * @brief Constructor
     */
    CppApplication(void);

    /**
     * @brief Destructor
     */
    ~CppApplication(void);
    
    /**
     * @brief Execute the application main loop
     *
     * When this member function is called the application core will enter the
     * main loop. It will not exit from this loop until the
     * Async::Application::quit method is called.
     */
    void exec(void);
    
    /**
     * @brief Exit the application main loop
     *
     * This function should be called to exit the application core main loop.
     */
    void quit(void);
    
  protected:
    
  private:
    struct lttimeval
    {
      bool operator()(const struct timeval& t1, const struct timeval& t2) const
      {
	return timercmp(&t1, &t2, <);
      }
    };
    typedef std::map<int, FdWatch*>   	      	      	      WatchMap;
    typedef std::multimap<struct timeval, Timer *, lttimeval> TimerMap;
    
    bool      	      	do_quit;
    int       	      	max_desc;
    fd_set    	      	rd_set;
    fd_set    	      	wr_set;
    WatchMap  	      	rd_watch_map;
    WatchMap  	      	wr_watch_map;
    TimerMap  	      	timer_map;
    
    void addFdWatch(FdWatch *fd_watch);
    void delFdWatch(FdWatch *fd_watch);
    void addTimer(Timer *timer);
    void addTimerP(Timer *timer, const struct timeval& current);
    void delTimer(Timer *timer);    
    DnsLookupWorker *newDnsLookupWorker(const std::string& label);
    
};  /* class CppApplication */


} /* namespace */

#endif /* ASYNC_CPP_APPLICATION_INCLUDED */



/*
 * This file has not been truncated
 */

