/**
@file   AsyncTaskRunner.h
@brief  Run tasks from the Async main thread
@author Tobias Blomberg / SM0SVX
@date   2019-03-09

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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

/** @example AsyncTaskRunner_demo.cpp
An example of how to use the Async::TaskRunner class
*/


#ifndef ASYNC_TASK_RUNNER_INCLUDED
#define ASYNC_TASK_RUNNER_INCLUDED


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
@brief  Run tasks from the Async main thread with automatic cleanup
@author Tobias Blomberg / SM0SVX
@date   2019-03-09

This class is used to run delayed tasks from the Async main thread. If the
object is destroyed before the task has been executed all tasks queued by this
object are cancelled. That property is the main advantage for using this class
rather than calling the Application::runTask functions directly.

A TaskRunner is typically used to break up long callback chains or to run a
task on the main Async threa when running in another thread.

\include AsyncTaskRunner_demo.cpp
*/
class TaskRunner
{
  public:
    /**
     * @brief   Default constructor
     */
    TaskRunner(void) {}

    /**
     * @brief   Copy constructor is deleted
     */
    TaskRunner(const TaskRunner&) = delete;

    /**
     * @brief   Destructor
     *
     * Will cancel all pending tasks
     */
    ~TaskRunner(void)
    {
      cancel();
    }

    /**
     * @brief   Assignment operator is deleted
     */
    TaskRunner& operator=(const TaskRunner&) = delete;

    //template <typename TaskHandler>
    //Application::TaskId runTask(TaskHandler&& handler)
    //{
    //  return Application::app().runTask(this,
    //      std::forward<TaskHandler>(handler));
    //}

    //template <typename Func, typename Arg1, typename... Args>
    //Application::TaskId runTask(Func&& f, Arg1&& arg1, Args&&... args)
    //{
    //  return Application::app().runTask(this, std::forward<Func>(f),
    //      std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    //}

    /**
     * @brief   Run a task from the Async main loop
     * @param   handler The handler function for the task to run
     * @returns Return the task id associated with the task
     *
     * Use it like this to call a normal function taking no arguments:
     *
     *   myTaskRunnerInstance(&the_function);
     *
     * If the function takes arguments, you need to use std::bind but then it's
     * better to use the other version of this function which calls bind for
     * you. But anyway, with std::bind it will look something like this for a
     * function taking a bool and an int as arguments:
     *
     *   myTaskRunnerInstance(std::bind(&the_function, true, 42));
     *
     * For a member function the second argument to std::bind must be the
     * object pointer so the syntax for a member function taking two arguments
     * would look something like this:
     *
     *   myTaskRunnerInstance(std::bind(&MyClass::func, this, true, 42));
     *
     * This function is thread safe so a separate thread can add tasks which
     * are then executed on the main Async thread.
     */
    template <typename TaskHandler>
    Application::TaskId operator()(TaskHandler&& handler)
    {
      return Application::app().runTask(this,
          std::forward<TaskHandler>(handler));
    }

    /**
     * @brief   Run a task from the Async main loop
     * @param   f     The function to call
     * @param   arg1  The first argument to the function
     * @param   args  The rest of the arguments to the function
     * @returns Return the task id associated with the task
     *
     * This overloaded function is used if more than one argument is used in
     * the function call. Calling a function taking two arguments can look like
     * this:
     *
     *   runTask(&the_function, true, 42);
     *
     * For a member function the second argument must be the object pointer so
     * the syntax for a member function taking two arguments would look
     * something like this:
     *
     *   runTask(&MyClass::func, this, true, 42);
     *
     * This function is thread safe so a separate thread can add tasks which
     * are then executed on the main Async thread.
     */
    template <typename Func, typename Arg1, typename... Args>
    Application::TaskId operator()(Func&& f, Arg1&& arg1, Args&&... args)
    {
      return Application::app().runTask(this, std::forward<Func>(f),
          std::forward<Arg1>(arg1), std::forward<Args>(args)...);
    }

    /**
     * @brief   Cancel all queued tasks for this object
     */
    void cancel(void)
    {
      Application::app().cancelTasks(this);
    }
};  /* class TaskRunner */


} /* namespace */

#endif /* ASYNC_TASK_RUNNER_INCLUDED */

/*
 * This file has not been truncated
 */
