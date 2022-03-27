#include <iostream>
#define ASYNC_STATE_MACHINE_DEBUG
#include <AsyncStateMachine.h>

struct Context
{
  int a;
};

struct StateTop;
struct StateDisconnected;
struct StateConnected;
struct StateConnectedA;
struct StateConnectedB;

struct StateTop : Async::StateTopBase<Context, StateTop>::Type
{
  static constexpr auto NAME = "Top";
  void init(void)
  {
    std::cout << "### StateTop::init" << std::endl;
    setState<StateDisconnected>();
  }
  void entry(void)
  {
    std::cout << "### StateTop::entry" << std::endl;
  }
  void exit(void)
  {
    std::cout << "### StateTop::exit" << std::endl;
  }
  virtual void eventA(void) {}
  virtual void eventB(void) {}
};

struct StateDisconnected : Async::StateBase<StateTop, StateDisconnected>
{
  static constexpr auto NAME = "Disconnected";
  void init(void)
  {
    std::cout << "### StateDisconnected::init" << std::endl;
    //setState<StateDisconnected>();
  }
  void entry(void)
  {
    std::cout << "### StateDisconnected::entry" << std::endl;
  }
  void exit(void)
  {
    std::cout << "### StateDisconnected::exit" << std::endl;
  }
  virtual void eventA(void) override
  {
    std::cout << "### StateDisconnected::eventA: ctx.a="
      << ctx().a << std::endl;
    ctx().a = 24;
    setState<StateConnected>();
  }
};

struct StateConnected : Async::StateBase<StateTop, StateConnected>
{
  static constexpr auto NAME = "Connected";
  void init(void)
  {
    std::cout << "### StateConnected::init" << std::endl;
    setState<StateConnectedA>();
    //setState<StateDisconnected>();
  }
  void entry(void)
  {
    std::cout << "### StateConnected::entry" << std::endl;
  }
  void exit(void)
  {
    std::cout << "### StateConnected::exit" << std::endl;
  }
  virtual void eventB(void) override
  {
    std::cout << "### StateConnected::eventB: ctx.a="
      << ctx().a << std::endl;
    setState<StateConnectedB>();
  }
};

struct StateConnectedA : Async::StateBase<StateConnected, StateConnectedA>
{
  static constexpr auto NAME = "ConnectedA";
  void init(void)
  {
    std::cout << "### StateConnectedA::init" << std::endl;
  }
  void entry(void)
  {
    std::cout << "### StateConnectedA::entry" << std::endl;
  }
  void exit(void)
  {
    std::cout << "### StateConnectedA::exit" << std::endl;
  }
};

struct StateConnectedB : Async::StateBase<StateConnected, StateConnectedB>
{
  static constexpr auto NAME = "ConnectedB";
  void entry(void)
  {
    std::cout << "### StateConnectedB::entry" << std::endl;
  }
  void exit(void)
  {
    std::cout << "### StateConnectedB::exit" << std::endl;
  }
};

int main()
{
  Context ctx;
  ctx.a = 42;
  Async::StateMachine<Context, StateTop> sm(&ctx);
  sm.start();
  sm.state().eventA();
  sm.state().eventB();
  sm.setState<StateDisconnected>();

  return 0;
}

