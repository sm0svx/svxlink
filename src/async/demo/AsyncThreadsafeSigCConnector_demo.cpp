#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#include <AsyncCppApplication.h>
#include <AsyncTimer.h>
#include <AsyncThreadsafeSigCConnector.h>
#include <AsyncThreadSigCAsyncConnector.h>
#include <AsyncThreadSigCSignal.h>

std::atomic<bool> done(false);

struct MyThread
{
  void operator()(int n)
  {
    int cnt = n;
    while (!done)
    {
      sig(cnt++);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  sigc::signal<void, int> sig;
};

struct MyHandler : public sigc::trackable
{
  std::string m_name;
  MyHandler(const std::string& name) : m_name(name) {}
  void mySlot(int n)
  {
    std::cout << "MyHandler::mySlot[" << m_name << "]: ";
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::cout << n;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::cout << std::endl;
  }
};

void timeout(Async::Timer* t)
{
  std::cout << "--- TIMEOUT ---" << std::endl;
}

void handle_signal(int signum)
{
  std::cout << "EXIT" << std::endl;
  Async::Application::app().quit();
}

int main(void)
{
    // Create application object and catch Ctrl-C (SIGINT)
  Async::CppApplication app;
  app.catchUnixSignal(SIGINT);
  app.unixSignalCaught.connect(std::ptr_fun(handle_signal));

    // Create a thread and handler object then connect a signal from the thread
    // object to a member function in the handler object
  MyThread my_thread;
  MyHandler h1("h1");
  Async::ThreadsafeSigCConnector<void, int> con1(
      my_thread.sig, sigc::mem_fun(h1, &MyHandler::mySlot));

  MyHandler h2("h2");
  Async::ThreadSigCAsyncConnector<int> async_con2(
      my_thread.sig, sigc::mem_fun(h2, &MyHandler::mySlot));

    // Create an Async::Timer which show that the main thread is alive
  Async::Timer tmo(1000, Async::Timer::TYPE_PERIODIC);
  tmo.expired.connect(sigc::ptr_fun(timeout));

    // Start the thread
  std::thread t(my_thread, 42);

    // Execute the main thread
  app.exec();

    // Tell the thread that we are done then join it when it exits
  done = true;
  t.join();
}
