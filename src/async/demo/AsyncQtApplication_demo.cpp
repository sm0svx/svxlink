#include <AsyncQtApplication.h>
#include <QPushButton>

using namespace Async;

int main(int argc, char **argv)
{
  QtApplication app(argc, argv);
  QPushButton hello("Hello, Async::QtApplication", 0);
  QObject::connect(&hello, SIGNAL(clicked()), &app, SLOT(quit()));
  hello.show();
  app.exec();
}
