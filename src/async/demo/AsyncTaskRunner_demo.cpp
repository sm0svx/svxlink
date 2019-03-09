#include <iostream>
#include <thread>
#include <AsyncCppApplication.h>
#include <AsyncTaskRunner.h>

class MyClass
{
  public:
    void operator()(void)
    {
      std::cout << "Before m_runner: thread_id="
                << std::this_thread::get_id() << std::endl;
      callDelayedTask(&MyClass::delayedTask, this, 42);
      std::cout << "After m_runner: thread_id="
                << std::this_thread::get_id() << std::endl;
    }
  private:
    Async::TaskRunner callDelayedTask;

    void delayedTask(int n)
    {
      std::cout << "delayedTask: thread_id=" << std::this_thread::get_id()
               << " n=" << n << std::endl;
      Async::Application::app().quit();
    }
};

int main(void)
{
  Async::CppApplication app;
  std::cout << "Main thread_id=" << app.threadId() << std::endl;
  MyClass mc;
  std::thread th(std::ref(mc));
  app.exec();
  th.join();
  return 0;
}
