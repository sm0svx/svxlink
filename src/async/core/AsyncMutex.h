/**
@file   AsyncMutex.h
@brief  A mutex for synchronizing threads with the main Async thread
@author Tobias Blomberg / SM0SVX
@date   2019-01-19

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

/** @example AsyncMutex_demo.cpp
An example of how to use the Async::Mutex class
*/

#ifndef ASYNC_MUTEX_INCLUDED
#define ASYNC_MUTEX_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <mutex>
#include <condition_variable>


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

class FdWatch;


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
@brief  Synchronize execution from a thread with the main Async thread
@author Tobias Blomberg / SM0SVX
@date   2019-01-19

This class is used to synchronize the execution of a thread with the main Async
thread so that it is safe to access resources owned by the main thread that
otherwise does not know about thread synchronization. When a thread holds the
lock the main Async thread is guaranteed to be in a safe place. All other
threads trying to get the same lock will be blocked until the thread holding
the lock release it.

As always with locks, try to hold the lock the shortest time possible since it
will block the main Async thread while the lock is held. That for example means
that no timer handlers nor file descriptor watch handlers will be called

Note that locking this mutex is almost always expensive since the main Async
thread own the lock if no other thread owns it. The main thread must then get
to a safe place before the mutex can be locked by another thread. Only use this
mutex if synchronization with the main Async thread is needed. If the purpose
is to only synchronize between other threads, use a standard mutex. A standard
mutex is also sufficient if synchronizing access to some data structure that
you have full control over.

Note that all instances of Async::Mutexes are connected so that if one
instance is locked, no other instance can be locked. That is, the other threads
wanting to lock an Async::Mutex will be blocked while waiting for the lock to
be unlocked.

The Async::Mutex may be used with standard C++ constructs like lock_guard,
unique_lock, condition_variable_any, etc.

\include AsyncMutex_demo.cpp
*/
class Mutex
{
  public:
    /**
     * @brief Default constructor
     *
     * A mutex must be created on the main Async thread. Also it cannot be
     * created before an Async::Application has been created. This means that
     * an Async::Mutex cannot be declared as a global variable since those are
     * initialized before the main function is called.
     */
    Mutex(void) noexcept;

    /**
     * @brief Copy constructor is deleted
     */
    Mutex(const Mutex&) = delete;

    /**
     * @brief Destructor
     *
     * A mutex must be destructed on the main Async thread.
     *
     * The behavior is undefined if the mutex is owned by any thread or if any
     * thread terminates while holding any ownership of the mutex.
     */
    ~Mutex(void);

    /**
     * @brief   Assignment operator is deleted
     */
    Mutex& operator=(const Mutex&) = delete;

    /**
     * @brief   Lock the mutex
     *
     * Calling this function will lock the mutex if it's not already locked by
     * another thread. If the lock is held by another thread this thread will
     * block until the lock can be acquired.
     * NOTE: If called from the main Async thread it's a noop since if we
     * already are executing in the main thread we by definition own the lock.
     */
    void lock(void);

    //bool try_lock() { return mu.try_lock(); }

    /**
     * @brief   Unlock the mutex
     *
     * Unlock a previously locked mutex. It is not allowed to call this
     * function if the calling thread does not own the lock.
     * NOTE: If called from the main Async thread it's a noop since if we
     * already are executing in the main thread we by definition own the lock.
     */
    void unlock(void);

  private:
    static std::mutex               mu;
    static std::condition_variable  lock_available_cond;
    static std::condition_variable  no_locks_waiting_cond;
    static int                      pipe_wr;
    static Async::FdWatch*          rd_watch;
    static std::thread::id          main_thread;
    static std::thread::id          lock_owner;
    static size_t                   instance_cnt;
    static size_t                   lock_wait_cnt;

    static void lockHandler(FdWatch *w);
    static void mainThreadDone(void);

};  /* class Mutex */


} /* namespace */

#endif /* ASYNC_MUTEX_INCLUDED */

/*
 * This file has not been truncated
 */
