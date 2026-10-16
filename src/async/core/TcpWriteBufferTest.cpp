/**
@file    TcpWriteBufferTest.cpp
@brief   Unit test for the TcpConnection write buffer
@author  Mark Rose
@date    2026-10-16

Connects a TcpClient to a listening socket that never reads, writes much
more than the socket buffers can hold and checks that writeBufferSize()
reports the data waiting to be sent. The peer then resets the connection,
and the client must report a disconnect instead of busy-looping on the
failing write.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include <AsyncCppApplication.h>
#include <AsyncTcpClient.h>
#include <AsyncTimer.h>

using namespace std;
using namespace Async;

namespace {

int failures = 0;

void check(bool cond, const string& msg)
{
  cout << (cond ? "  ok   " : "  FAIL ") << msg << endl;
  if (!cond)
  {
    ++failures;
  }
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;

    // A listening socket on an ephemeral loopback port that never reads
  int lsock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  bind(lsock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
  listen(lsock, 1);
  socklen_t addr_len = sizeof(addr);
  getsockname(lsock, reinterpret_cast<struct sockaddr*>(&addr), &addr_len);
  const uint16_t port = ntohs(addr.sin_port);

  TcpClient<> client(IpAddress("127.0.0.1"), port);
  bool was_connected = false;
  bool was_disconnected = false;
  int peer_sock = -1;
  const vector<char> data(64 * 1024, 'x');

  client.connected.connect([&]() {
      was_connected = true;
      for (int i = 0; i < 64; ++i)
      {
        client.write(data.data(), data.size());
      }
    });
  client.disconnected.connect(
      [&](TcpConnection*, TcpConnection::DisconnectReason) {
        was_disconnected = true;
        app.quit();
      });

  Timer reset_timer(500);
  reset_timer.expired.connect([&](Timer*) {
      check(was_connected, "the client connects");
      check(client.writeBufferSize() > 0,
            "data the peer has not read is reported as waiting to be sent");
        // Reset the connection from the peer side
      peer_sock = accept(lsock, nullptr, nullptr);
      struct linger lin = {1, 0};
      setsockopt(peer_sock, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
      close(peer_sock);
    });
  Timer give_up_timer(5000);
  give_up_timer.expired.connect([&](Timer*) { app.quit(); });

  client.connect();
  app.exec();

  check(was_disconnected,
        "the client reports a disconnect when the peer resets the connection");
  check(client.writeBufferSize() == 0,
        "the write buffer is empty after the disconnect");

  close(lsock);

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
