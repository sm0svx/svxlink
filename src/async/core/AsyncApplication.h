/**
@file	 AsyncApplication.h
@brief   The core class for writing asyncronous applications
@author  Tobias Blomberg
@date	 2003-03-16

This file contains the AsyncApplication class which is the core of an
application that use the Async classes.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg

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

#ifndef ASYNC_APPLICATION_INCLUDED
#define ASYNC_APPLICATION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <thread>
#include <mutex>


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

class Timer;
class FdWatch;
class DnsLookupWorker;
class TaskRunner;


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
 * @brief The base class for asynchronous applications
 *
 * This is the base class for all asynchronous applications. It is an abstract
 * class and so it must be inherited from to create a class that can be
 * instantiated.
 */   
class Application : public sigc::trackable
{
  public:
    /**
     * @brief   The type for identifying a task added by runTask.
     */
    typedef uint64_t TaskId;

    /**
     * @brief 	Get the one and only application instance
     *
     * Use this static member function to get the one and only instance of the
     * application object. If an application object has not been previously
     * created, the application will crash with a failed assertion.
     * @return	Returns a reference to the applicaton instance
     */
    static Application &app(void);
    
    /**
     * @brief Default constructor
     */
    Application(void);
    
    /**
     * @brief Destructor
     */
    virtual ~Application(void);
    
    /**
     * @brief Execute the application main loop
     *
     * When this member function is called the application core will enter the
     * core main loop. It will not exit from this loop until the
     * Async::Application::quit method is called.
     */
    virtual void exec(void) = 0;
    
    /**
     * @brief Exit the application main loop
     *
     * This function should be called to exit the application core main loop.
     */
    virtual void quit(void) = 0;

    /**
     * @brief   Run a task from the Async main loop
     * @param   handler The handler function for the task to run
     * @returns Return the task id associated with the task
     *
     * The runTask function can be used to delay a function call until the call
     * chain have returned to the Async main loop. This may be required in some
     * cases where one otherwise would get into trouble with complex callback
     * chains causing strange errors.
     *
     * Use it like this to call a normal function taking no arguments:
     *
     *   runTask(&the_function);
     *
     * If the function takes arguments, you need to use std::bind but then it's
     * better to use the other version of this function which calls bind for
     * you. But anyway, with std::bind it will look something like this for a
     * function taking a bool and an int as arguments:
     *
     *   runTask(std::bind(&the_function, true, 42));
     *
     * For a member function the second argument to std::bind must be the
     * object pointer so the syntax for a member function taking two arguments
     * would look something like this:
     *
     *   runTask(std::bind(&MyClass::func, this, true, 42));
     *
     * This function is thread safe so a separate thread can add tasks which
     * are then executed on the main Async thread.
     */
    template <typename TaskHandler>
    TaskId runTask(TaskHandler&& handler)
    {
      class Task : public TaskBase
      {
        public:
          Task(TaskHandler&& handler)
            : TaskBase(0), handler(std::move(handler)) {}
          void operator()(void) { handler(); }
        private:
          TaskHandler handler;
      };
      return addTaskToQueue(new Task(std::forward<TaskHandler>(handler)));
    }

    /**
     * @brief   Run a task from the Async main loop
     * @param   handler The handler function for the task to run
     * @returns Return the task id associated with the task
     *
     * This function do the same as the runTask function but in addition to
     * queueing the task for execution it will also associate an owner object
     * with the task. This will make it possible to later cancel all tasks
     * associated with the object using the cancelTasks function.
     *
     * Normal usage is not to call this function directly but rather make the
     * owner class inherit from Async::TaskRunner and use the runTask function
     * from that class. All pending tasks will then be automatically cancelled
     * upon object destruction.
     */
    template <typename TaskHandler>
    TaskId runTask(TaskRunner *owner, TaskHandler&& handler)
    {
      class Task : public TaskBase
      {
        public:
          Task(TaskRunner *owner, TaskHandler&& handler)
            : TaskBase(owner), handler(std::move(handler)) {}
          void operator()(void) { handler(); }
        private:
          TaskHandler handler;
      };
      return addTaskToQueue(new Task(owner, std::forward<TaskHandler>(handler)));
    }

    /**
     * @brief   Run a task from the Async main loop
     * @param   f     The function to call
     * @param   arg1  The first argument to the function
     * @param   args  The rest of the arguments to the function
     * @returns Return the task id associated with the task
     *
     * The runTask function can be used to delay a function call until the call
     * chain have returned to the Async main loop. This may be required in some
     * cases where one otherwise would get into trouble with complex callback
     * chains causing strange errors.
     *
     * This overloaded variant of this function is used if more than one
     * argument is given when called. For example, a call to a function taking
     * two arguments can look like this:
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
    TaskId runTask(Func&& f, Arg1&& arg1, Args&&... args)
    {
      typedef decltype(std::bind(f, arg1, args...)) HandlerType;
      class Task : public TaskBase
      {
        public:
          Task(Func&& f, Arg1&& arg1, Args&&... args)
            : TaskBase(0),
              handler(std::bind(std::forward<Func>(f),
                  std::forward<Arg1>(arg1), std::forward<Args>(args)...)) {}
          void operator()(void) { handler(); }
        private:
          HandlerType handler;
      };
      return addTaskToQueue(
          new Task(std::forward<Func>(f), std::forward<Arg1>(arg1),
                       std::forward<Args>(args)...));
    }

    /**
     * @brief   Run a task from the Async main loop
     * @param   owner The object that owns the task
     * @param   f     The function to call
     * @param   arg1  The first argument to the function
     * @param   args  The rest of the arguments to the function
     * @returns Return the task id associated with the task
     *
     * This function do the same as the runTask function but in addition to
     * queueing the task for execution it will also associate an owner object
     * with the task. This will make it possible to later cancel all tasks
     * associated with the object using the cancelTasks function.
     *
     * Normal usage is not to call this function directly but rather make the
     * owner class inherit from Async::TaskRunner and use the runTask function
     * from that class. All pending tasks will then be automatically cancelled
     * upon object destruction.
     */
    template <typename Func, typename Arg1, typename... Args>
    TaskId runTask(TaskRunner *owner, Func&& f, Arg1&& arg1, Args&&... args)
    {
      typedef decltype(std::bind(f, arg1, args...)) HandlerType;
      class Task : public TaskBase
      {
        public:
          Task(TaskRunner *owner, Func&& f, Arg1&& arg1, Args&&... args)
            : TaskBase(owner), handler(std::bind(std::forward<Func>(f),
                  std::forward<Arg1>(arg1), std::forward<Args>(args)...)) {}
          void operator()(void) { handler(); }
        private:
          HandlerType handler;
      };
      return addTaskToQueue(
          new Task(owner, std::forward<Func>(f), std::forward<Arg1>(arg1),
                   std::forward<Args>(args)...));
    }

    /**
     * @brief   Cancel a task
     * @param   task_id A task ID as returned from runTask or runOwnedTask
     *
     * This function will cancel the task corresponding to the given task ID.
     * The null task_id TaskId(), or an invalid task_id, will be ignored.
     */
    void cancelTask(TaskId task_id);

    /**
     * @brief   Cancel all tasks owned by the given owner
     * @param   owner An existing owner object
     */
    void cancelTasks(TaskRunner *owner);

    /**
     * @brief Get the thread ID for the main Async thread
     */
    std::thread::id threadId(void) const { return m_main_thread_id; }

    /**
     * @brief   Signal that is emitted when the exec function exit
     *
     * This signal is emitted at the end of the exec function, just before
     * it exits.
     */
    sigc::signal<void> execDone;

    /**
     * @brief   Signal that is emitted just after construction
     *
     * This signal is emitted right after the Application object is constructed.
     */
    sigc::signal<void> construct;

    /**
     * @brief   Signal that is emitted just before destruction
     *
     * This signal is emitted right before the Application object is destroyed.
     */
    sigc::signal<void> destroy;

  private:
    friend class FdWatch;
    friend class Timer;
    friend class DnsLookup;

    class TaskBase
    {
      public:
        TaskBase(TaskRunner *owner) : m_owner(owner) {}
        virtual ~TaskBase(void) {}
        virtual void operator()(void) = 0;
        TaskRunner *owner(void) const { return m_owner; }
      private:
        TaskRunner* const m_owner;
    };

    typedef std::map<TaskId, TaskBase*> TaskQueue;

    static Application *app_ptr;
    static TaskId       next_task_id;

    const std::thread::id m_main_thread_id;
    TaskQueue             m_task_queue;
    Timer*                m_task_timer;
    std::recursive_mutex  m_task_mu;
    Async::FdWatch*       m_task_rd_watch = 0;
    int                   m_task_wr_pipe = -1;

    TaskId addTaskToQueue(TaskBase *task);
    void processTaskQueue(void);
    void handleTaskWatch(Async::FdWatch* w);
    void clearTasks(void);

    virtual void addFdWatch(FdWatch *fd_watch) = 0;
    virtual void delFdWatch(FdWatch *fd_watch) = 0;
    virtual void addTimer(Timer *timer) = 0;
    virtual void delTimer(Timer *timer) = 0;
    virtual DnsLookupWorker *newDnsLookupWorker(const std::string& label) = 0;
    
};  /* class Application */


} /* namespace */

#endif /* ASYNC_APPLICATION_INCLUDED */



/*
 * This file has not been truncated
 */

