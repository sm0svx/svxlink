#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncFdWatch.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
      stdin_watch->activity.connect(mem_fun(*this, &MyClass::onActivity));
    }
    
    ~MyClass(void)
    {
      delete stdin_watch;
    }

  private:
    FdWatch *stdin_watch;
    
    void onActivity(FdWatch *watch)
    {
      char buf[1024];
      int cnt = read(watch->fd(), buf, sizeof(buf)-1);
      if (cnt == -1)
      {
      	perror("read");
	Application::app().quit();
	return;
      }
      buf[cnt] = 0;
      cout << "Read from STDIN: " << buf << endl;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
