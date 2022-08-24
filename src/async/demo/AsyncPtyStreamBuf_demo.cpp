#include <sigc++/bind.h>
#include <string>
#include <cstdlib>
#include <ostream>
#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncPty.h>
#include <AsyncPtyStreamBuf.h>
#include <AsyncTimer.h>

using namespace std;


void write_to_stream(ostream *os)
{
  *os << "Hello, PTY\n";
}


void data_received(const void *buf, size_t len)
{
  cout << "Received: ";
  cout.write(static_cast<const char *>(buf), len);
}


int main(void)
{
  Async::CppApplication app;

  string pty_slave_path("/tmp/testpty");
  Async::Pty pty(pty_slave_path);
  if (!pty.open())
  {
    cout << "Failed to open PTY " << pty_slave_path << endl;
    exit(1);
  }
  pty.dataReceived.connect(sigc::ptr_fun(&data_received));

  Async::PtyStreamBuf psb(&pty);
  ostream os(&psb);

    // Flush the stream buffer after every write
  os.setf(ios::unitbuf);

  cout << "\nThis demo app will write a line of text every second to a PTY.\n";
  cout << "To see the printouts, in another console do:\n";
  cout << "\n\tcat /tmp/testpty\n\n";
  cout << "Or to also echo back the received text to the PTY:\n";
  cout << "\n\ttee /tmp/testpty </tmp/testpty\n\n";

    // Start a timer that periodically calls function write_to_stream
  Async::Timer t(1000, Async::Timer::TYPE_PERIODIC);
  sigc::slot<void, ostream*> timer_handler = sigc::ptr_fun(&write_to_stream);
  t.expired.connect(sigc::hide(sigc::bind(timer_handler, &os)));

  app.exec();
}

