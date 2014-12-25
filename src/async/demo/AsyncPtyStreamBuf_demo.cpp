#include <string>
#include <cstdlib>
#include <ostream>
#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncPty.h>
#include <AsyncPtyStreamBuf.h>

using namespace std;

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

  Async::PtyStreamBuf *psb = new Async::PtyStreamBuf(&pty);
  ostream os(psb);

    // Flush the stream buffer after every write
  os.setf(ios::unitbuf);

  cout << "\nThis demo app will write a line of text every second to a PTY.\n";
  cout << "To see the printouts, in another console do:\n";
  cout << "\n\tcat /tmp/testpty\n\n";
  for (;;)
  {
    os << "Hello, PTY!\n";
    sleep(1);
  }
}

