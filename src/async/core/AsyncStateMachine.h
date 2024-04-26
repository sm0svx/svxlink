/**
@file   AsyncStateMachine.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2022-03-19

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

/** @example AsyncStateMachine_demo.cpp
An example of how to use the StateMachine class
*/

#ifndef ASYNC_STATE_MACHINE_INCLUDED
#define ASYNC_STATE_MACHINE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <typeinfo>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncAtTimer.h>


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

template <class ContextT, class TopStateT> class StateTopBase;


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
@brief  Implements a hierarchial state machine
@author Tobias Blomberg / SM0SVX
@date   2022-03-19
@param  ContextT  State machine context
@param  StateTopT The top state class

A class that implements a Hierarchial Finite State Machine.

  struct Context
  {
    // Variables and functions used within the state machine
  };
  struct StateTop : Async::StateTopBase<Context, StateTop>::Type
  {
    static constexpr auto NAME = "Top";
      // Event functions implemented by states that handle them
    virtual void eventA(void) {}
    virtual void eventB(void) {}
  };
  struct StateMyStateA : Async::StateBase<StateTop, StateMyStateA>
  {
    static constexpr auto NAME = "MyStateA";
      // Event handler functions
    virtual void eventA(void) override {}
  };
  struct StateMyStateB : Async::StateBase<StateTop, StateMyStateB>
  {
    static constexpr auto NAME = "MyStateB";
      // Event handler functions
    virtual void eventB(void) override {}
  };
  Context ctx;
  Async::StateMachine<Context, StateTop> sm(&ctx);
  sm.start()

The NAME constant is only needed for state transition debugging. To enable
debugging, define ASYNC_STATE_MACHINE_DEBUG before including this file.

Full example below.

\include AsyncStateMachine_demo.cpp
*/
template <class ContextT, class StateTopT>
class StateMachine
{
  public:
    /**
     * @brief   A type alias simplifying access to the top state base type
     */
    using StateTopBaseT = StateTopBase<ContextT,StateTopT>;

    /**
     * @brief   Constructor
     * @param   ctx The context object
     */
    StateMachine(ContextT* ctx) : m_ctx(ctx)
    {
      m_timer.expired.connect(
          [&](Timer*)
          {
            assert(m_state != nullptr);
            clearTimeout();
            static_cast<StateTopBaseT*>(m_state)->timeoutEvent();
          });
      m_at_timer.expired.connect(
          [&](AtTimer*)
          {
            assert(m_state != nullptr);
            clearTimeoutAt();
            static_cast<StateTopBaseT*>(m_state)->timeoutAtEvent();
          });
    }

    /**
     * @brief   Disallow copy construction
     */
    StateMachine(const StateMachine&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    StateMachine& operator=(const StateMachine&) = delete;

    /**
     * @brief   Destructor
     */
    ~StateMachine(void)
    {
      delete m_state;
      m_state = nullptr;
    }

    /**
     * @brief   Start the state machine
     *
     * This function must be called after constructing the state machine. The
     * top state will be initialized and entered. Do not call any other
     * functions in this class until this function has been called.
     */
    void start(void)
    {
      setState<StateTopT>();
    }

    /**
     * @brief   Get the context object
     * @return  Returns a reference to the context object
     */
    ContextT& ctx(void)
    {
      return *m_ctx;
    }

    /**
     * @brief   Switch to the given state
     * @param   A state object to switch to
     *
     * Use this function to switch to the given state. The state to switch to
     * is best given as a template argument to the function. E.g.
     *
     *   setState<NextState>();
     *
     * NOTE: The state machine implementation cannot handle state switching
     * loops right now. It's ok to set the same state as the current one since
     * it will just be ignored. Switching to the current state via other
     * states init functions, will cause an infinite loop.
     */
    template <class NewStateT>
    void setState(NewStateT* state=new NewStateT)
    {
      state->setMachine(this);
      StateTopT* old_state = m_state;
      if ((old_state != nullptr) && (typeid(NewStateT) == old_state->typeId()))
      {
        delete state;
        return;
      }
      static_cast<StateTopBaseT*>(state)->initHandler();
      if (old_state != m_state)
      {
        delete state;
        return;
      }
#ifdef ASYNC_STATE_MACHINE_DEBUG
      std::cout << "### StateMachine: ";
      std::cout << (old_state != nullptr ? old_state->name() : "NULL");
      std::cout << " -> " << state->name() << std::endl;
#endif
      if (old_state != nullptr)
      {
        static_cast<StateTopBaseT*>(old_state)->exitHandler(state);
      }
      m_state = state;
      static_cast<StateTopBaseT*>(m_state)->entryHandler(old_state);
      delete old_state;
    }

    /**
     * @brief   Check if the given state is the active one
     * @return  Returns \em true if the given state is the active one
     *
     * Use this function to check if the given state is the active one. The
     * state to check is given as a template argument to the function.  E.g.
     *
     *   if (isActive<NextState>()) {}
     *
     */
    template <class T>
    bool isActive(void) const
    {
      return typeid(T) == m_state->typeId();
    }

    /**
     * @brief   Get the active state
     * @return  Returns a reference to the active state
     */
    StateTopT& state(void) { return *m_state; }

    /**
     * @brief   Set a timeout after which the timeoutEvent is issued
     * @param   timeout_ms The timeout value in milliseconds
     *
     * Use this function to set a timeout to occur after the specified number
     * of milliseconds. The timeoutEvent will be issued after the time has
     * expired.
     */
    void setTimeout(int timeout_ms)
    {
#ifdef ASYNC_STATE_MACHINE_DEBUG
      std::cout << "### StateMachine: setTimeout(" << timeout_ms << ")"
                << std::endl;
#endif
      m_timer.setTimeout(timeout_ms);
      m_timer.setEnable(true);
    }

    /**
     * @brief   Set a timeout after which the timeoutAtEvent is issued
     * @param   tm            The absolute time when the timeout should occur
     * @param   expire_offset A millisecond offset for the timer expiration
     *
     * Use this function to set a timeout to occur at the specified absolute
     * time, plus or minus the offset value. The time is specified in local
     * time. The timeoutAtEvent will be issued after the time has expired.
     */
    void setTimeoutAt(struct tm& tm, int expire_offset=0)
    {
      m_at_timer.setTimeout(tm);
      m_at_timer.setExpireOffset(expire_offset);
      m_at_timer.start();
    }

    /**
     * @brief   Clear a pending timeout
     *
     * Use this function to immediately cancel a running timeout timer. See
     * \ref setTimeout for more information.
     */
    void clearTimeout(void)
    {
#ifdef ASYNC_STATE_MACHINE_DEBUG
      std::cout << "### StateMachine: clearTimeout()" << std::endl;
#endif
      m_timer.setEnable(false);
    }

    /**
     * @brief   Clear a pending absolute time timeout
     *
     * Use this function to immediately cancel a running absolute time timeout
     * timer. See \ref setTimeoutAt for more information.
     */
    void clearTimeoutAt(void)
    {
      m_at_timer.stop();
    }

  private:
    StateTopT*  m_state     = nullptr;
    ContextT*   m_ctx       = nullptr;
    Timer       m_timer     = -1;
    AtTimer     m_at_timer;
}; /* StateMachine */


/**
@brief  Implements a hierarchial state machine
@author Tobias Blomberg / SM0SVX
@date   2022-03-19
@param  ParentT The parent state
@param  T       The state class itself

This class should be used together with Async::StateMachine to form a state in
the state machine. All states must inherit from this class.

  struct StateMyState : Async::StateBase<StateTop, StateMyState>
  {
    static constexpr auto NAME = "MyState";
  };

The NAME constant is only needed for state transition debugging.

Full example below.

\include AsyncStateMachine_demo.cpp
*/
template <class ParentT, class T>
class StateBase : public ParentT
{
  public:
    /**
     * @brief   Get the typeid for this state
     * @return  Returns the typeid for this state
     */
    virtual const std::type_info& typeId(void) const override
    {
      return typeid(T);
    }

    virtual const char* name(void) const { return T::NAME; }

  protected:
    /**
     * @brief   Handle calling the init function on state transitions
     */
    virtual void initHandler(void) override
    {
      dynamic_cast<T*>(this)->init();
    }

    /**
     * @brief   Handle calling the entry function on state transitions
     * @param   from The state that we transition from
     */
    virtual void entryHandler(typename ParentT::StateT* from) override
    {
      if (dynamic_cast<T*>(from) == nullptr)
      {
        ParentT::entryHandler(from);
        dynamic_cast<T*>(this)->entry();
      }
    }

    /**
     * @brief   Handle calling the exit function on state transitions
     * @param   to The state that we transition to
     */
    virtual void exitHandler(typename ParentT::StateT* to) override
    {
      if (dynamic_cast<T*>(to) == nullptr)
      {
        dynamic_cast<T*>(this)->exit();
        ParentT::exitHandler(to);
      }
    }

    /**
     * @brief   Called before a transition from one state to another
     *
     * The init function is called before a state transition is executed so
     * this is the place to switch to another state using the setState
     * function. That is typically used to select a substate to activate when a
     * state is activated.
     *
     * The init function will only be called in the specific target state.
     */
    void init(void) {}

    /**
     * @brief   Called when a state is entered
     *
     * The entry function is called when a state is entered. It is not allowed
     * to initiate any state transitions in this function. That should be done
     * in the init function or in event handlers.
     *
     * The entry function will be called, from top to bottom, for all states in
     * the hierarchy. States that are common to the source and the target
     * states will not have the entry function called.
     */
    void entry(void) {}

    /**
     * @bief  Called when a state is exited
     *
     * The exit function is called when a state is exited. It is not allowed to
     * initiate any state transitions in this function. That should be done in
     * the init function or in event handlers.
     *
     * The exit function will be called, from bottom to top, for all states in
     * the hierarchy. States that are common to the source and the target
     * states will not have the exit function called.
     */
    void exit(void) {}

    /**
     * @brief   An event function that will be called when a timeout occurs
     *
     * This event function will be called when a timeout, previously set up
     * using the setTimeout function, has occurred.
     *
     * As all event functions this is a virtual function which work like any
     * other virtual function in C++. The state which is furtherest down in the
     * hierarchy, which have the timeoutEvent function implemented, will have
     * the function called.
     */
    virtual void timeoutEvent(void) override
    {
      assert(!"Async::StateBase: Unhandled timeoutEvent");
    }

    virtual void timeoutAtEvent(void) override
    {
      assert(!"Async::StateBase: Unhandled timeoutAtEvent");
    }

}; /* StateBase */


/**
@brief  The base class for the top state of a state machine
@author Tobias Blomberg / SM0SVX
@date   2022-03-19

This class should be used together with Async::StateMachine to form the top
state of the state machine. The top state must use this class as its parent
state. A type alias is available to simplify the syntax. E.g.

  struct StateTop : Async::StateTopBase<Context, StateTop>::Type
  {
    static constexpr auto NAME = "Top";
  };

The NAME constant is only needed for state transition debugging.

Full demo below.

\include AsyncStateMachine_demo.cpp
*/
template <class ContextT, class TopStateT>
class StateTopBase
{
  public:
    /**
     * @brief   A type alias to simplify declaration of the top state
     */
    using Type = StateBase<StateTopBase<ContextT, TopStateT>, TopStateT>;

    /**
     * @brief   A type alias to access the top state type
     */
    using StateT = TopStateT;

    /**
     * @brief   A type alias to simplify usage of the state machine type
     */
    using StateMachineT = StateMachine<ContextT, StateT>;

    /**
     * @brief   Destructor
     */
    virtual ~StateTopBase(void) {}

    /**
     * @brief   Get the context object
     * @return  Returns a reference to the context object
     */
    ContextT& ctx(void) { return m_sm->ctx(); }

  protected:
    /**
     * @brief   Transition to the given state
     *
     * See \ref Async::StateMachine::setState.
     */
    template <class NewStateT>
    void setState(void) { m_sm->setState(new NewStateT); }

    /**
     * @brief   Set a timeout after which the timeoutEvent is issued
     * @param   timeout_ms The timeout value in milliseconds
     *
     * Use this function to set a timeout to occur after the specified number
     * of milliseconds. The timeoutEvent will be issued after the time has
     * expired.
     */
    void setTimeout(int timeout_ms) { m_sm->setTimeout(timeout_ms); }

    /**
     * @brief   Set a timeout after which the timeoutAtEvent is issued
     * @param   tm            The absolute time when the timeout should occur
     * @param   expire_offset A millisecond offset for the timer expiration
     *
     * Use this function to set a timeout to occur at the specified absolute
     * time, plus or minus the offset value. The time is specified in local
     * time. The timeoutAtEvent will be issued after the time has expired.
     */
    void setTimeoutAt(struct tm& tm, int expire_offset=0)
    {
      m_sm->setTimeoutAt(tm, expire_offset);
    }

    /**
     * @brief   Clear a pending timeout
     *
     * Use this function to immediately cancel a running timeout timer. See
     * \ref setTimeout for more information.
     */
    void clearTimeout(void) { m_sm->clearTimeout(); }

    /**
     * @brief   Clear a pending absolute time timeout
     *
     * Use this function to immediately cancel a running absolute time timeout
     * timer. See \ref setTimeoutAt for more information.
     */
    void clearTimeoutAt(void) { m_sm->clearTimeoutAt(); }

    /**
     * @brief   Get the typeid for this state
     */
    virtual const std::type_info& typeId(void) const = 0;

    /**
     * @brief   Run the init functon in a state
     */
    virtual void initHandler(void) = 0;

    /**
     * @brief   Run all entry handlers in the state hierarchy, top to bottom
     * @param   from The state that we transition from
     */
    virtual void entryHandler(StateT* from) {}

    /**
     * @brief   Run all exit handlers in the state hierarchy, bottom to top
     * @param   to The state that we transition to
     */
    virtual void exitHandler(StateT* to) {}

    /**
     * @brief   An event function that will be called when a timeout occurs
     *
     * This event function will be called when a timeout, previously set up
     * using the setTimeout function, has occurred.
     *
     * As all event functions this is a virtual function which work like any
     * other virtual function in C++. The state which is furtherest down in the
     * hierarchy, which have the timeoutEvent function implemented, will have
     * the function called.
     */
    virtual void timeoutEvent(void) = 0;

    /**
     * @brief   Event function called when an absolute time timeout occurs
     *
     * This event function will be called when an absolute time timeout,
     * previously set up using the setTimeoutAt function, has occurred.
     *
     * As all event functions this is a virtual function which work like any
     * other virtual function in C++. The state which is furtherest down in the
     * hierarchy, which have the timeoutAtEvent function implemented, will have
     * the function called.
     */
    virtual void timeoutAtEvent(void) = 0;

  private:
    StateMachineT* m_sm;

    void setMachine(StateMachineT* sm) { m_sm = sm; }

    friend StateMachineT;
}; /* StateTopBase */



} /* namespace Async */

#endif /* ASYNC_STATE_MACHINE_INCLUDED */

/*
 * This file has not been truncated
 */
