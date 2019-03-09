/**
@file	 AsyncApplication.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <stdlib.h>

#include <cassert>
#include <algorithm>
#include <iostream>
#include <cstring>


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

#include "AsyncFdWatch.h"
#include "AsyncTimer.h"
#include "AsyncApplication.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/

Application *Application::app_ptr = 0;
Application::TaskId Application::next_task_id = Application::TaskId()+1;


/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


Application &Application::app(void)
{
  assert(app_ptr != 0);
  return *app_ptr;
} /* Application::app */


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
Application::Application(void)
  : m_main_thread_id(std::this_thread::get_id())
{
  assert(app_ptr == 0);
  app_ptr = this;  

  m_task_timer = new Async::Timer(0, Timer::TYPE_ONESHOT, false);
  m_task_timer->expired.connect(
      sigc::hide(mem_fun(*this, &Application::processTaskQueue)));
  int pipefd[2];
  if (pipe(pipefd) == -1)
  {
    std::cerr << "*** ERROR: Could not create pipe in Async::Application: "
              << strerror(errno) << std::endl;
    abort();
  }
  m_task_rd_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD, false);
  m_task_rd_watch->activity.connect(
      sigc::mem_fun(this, &Application::handleTaskWatch));
  m_task_wr_pipe = pipefd[1];
  construct.connect(
      sigc::bind(sigc::mem_fun(*m_task_rd_watch, &FdWatch::setEnabled), true));
  destroy.connect(sigc::mem_fun(*this, &Application::clearTasks));
} /* Application::Application */


Application::~Application(void)
{
  delete m_task_timer;
  m_task_timer = 0;
  close(m_task_rd_watch->fd());
  delete m_task_rd_watch;
  m_task_rd_watch = 0;
  close(m_task_wr_pipe);
  m_task_wr_pipe = -1;
} /* Application::~Application */


void Application::cancelTask(TaskId task_id)
{
  if (task_id != TaskId())
  {
    std::lock_guard<std::recursive_mutex> lk(m_task_mu);
    TaskQueue::iterator it = m_task_queue.find(task_id);
    if (it != m_task_queue.end())
    {
      delete (*it).second;
      (*it).second = 0;
    }
  }
} /* Application::cancelTask */


void Application::cancelTasks(TaskRunner *owner)
{
  assert(owner != 0);
  std::lock_guard<std::recursive_mutex> lk(m_task_mu);
  for (auto it=m_task_queue.begin(); it!=m_task_queue.end(); ++it)
  {
    TaskBase *task = (*it).second;
    if ((task != 0) && (task->owner() == owner))
    {
      delete (*it).second;
      (*it).second = 0;
    }
  }
} /* Application::cancelTasks */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

Application::TaskId Application::addTaskToQueue(Application::TaskBase *task)
{
  TaskId task_id;
  {
    std::lock_guard<std::recursive_mutex> lk(m_task_mu);
    do
    {
      task_id = next_task_id++;
    } while (task_id == TaskId());
    m_task_queue.emplace(task_id, task);
  }
  if (std::this_thread::get_id() == m_main_thread_id)
  {
    m_task_timer->setEnable(true);
  }
  else
  {
    if (::write(m_task_wr_pipe, "", 1) == -1)
    {
      std::cerr << "*** ERROR: Could not write to pipe in Async::Application: "
                << std::strerror(errno) << std::endl;
      abort();
    }
  }
  return task_id;
} /* Application::addTaskToQueue */


void Application::processTaskQueue(void)
{
  std::unique_lock<std::recursive_mutex> lk(m_task_mu);
  while (!m_task_queue.empty())
  {
    TaskQueue::iterator it = m_task_queue.begin();
    TaskBase *task = (*it).second;
    (*it).second = 0;
    if (task != 0)
    {
      lk.unlock();
      (*task)();
      lk.lock();
      delete task;
    }
    m_task_queue.erase(it);
  }
  m_task_timer->setEnable(false);
} /* Application::processTaskQueue */


void Application::handleTaskWatch(Async::FdWatch* w)
{
  char buf[256];
  ssize_t cnt = read(w->fd(), buf, sizeof(buf));
  if (cnt == -1)
  {
    std::cerr << "*** ERROR: Could not read pipe in "
                 "Async::Application::handleTaskWatch: "
              << std::strerror(errno) << std::endl;
    abort();
  }
  processTaskQueue();
} /* Application::handleTaskWatch */


void Application::clearTasks(void)
{
  std::lock_guard<std::recursive_mutex> lk(m_task_mu);
  for (auto& item : m_task_queue)
  {
    delete item.second;
  }
  m_task_queue.clear();
  m_task_timer->setEnable(false);
  m_task_rd_watch->setEnabled(false);
} /* Application::clearTasks */


/*
 * This file has not been truncated
 */
