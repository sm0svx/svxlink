#include <iostream>
#include <thread>
#include <chrono>

#include <AsyncCppApplication.h>
#include <AsyncTimer.h>
#include <AsyncThreadSigCSignal.h>

class MyThread
{
  public:
    void operator()(void)
    {
      for (;;)
      {
        sig("thread");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

      // Replace Async::ThreadSigCSignal with sigc::signal to see what happens
      // when using a non-threadsafe signal
    Async::ThreadSigCSignal<void, std::string> sig;
};

void print(const std::string& name)
{
  std::cout << "Called from ";
  std::this_thread::sleep_for(std::chrono::milliseconds(33));
  std::cout << name;
  std::this_thread::sleep_for(std::chrono::milliseconds(33));
  std::cout << std::endl;
}

void handle_timeout(Async::Timer*)
{
  print("timer");
}

int main(void)
{
  Async::CppApplication app;
  MyThread my_thread;
  my_thread.sig.connect(sigc::ptr_fun(&print));
  std::thread th(std::ref(my_thread));

  Async::Timer t(33, Async::Timer::TYPE_PERIODIC);
  t.expired.connect(sigc::ptr_fun(&handle_timeout));

  app.exec();

  return 0;
}
