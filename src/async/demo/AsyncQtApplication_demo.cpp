#include <AsyncQtApplication.h>
#include <qpushbutton.h>

using namespace Async;

int main(int argc, char **argv)
{
  QtApplication app(argc, argv);
  QPushButton hello("Hello, Async::QtApplication", 0);
  QObject::connect(&hello, SIGNAL(clicked()), &app, SLOT(quit()));
  app.setMainWidget(&hello);
  hello.show();
  app.exec();
}
