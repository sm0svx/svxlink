/**
@file    FramedTcpConnectionTest.cpp
@brief   Unit tests for the frame parsing in Async::FramedTcpConnection
@author  Mark Rose
@date    2026-11-26

Async::FramedTcpConnection splits the received byte stream into frames, each
sent as a four byte big endian size followed by that many bytes. These tests
feed data straight to the receive handler, without a socket, and check which
frames are delivered and when.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "AsyncFramedTcpConnection.h"

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


  // Gives access to the receive handler and records the delivered frames.
  // The connection has no socket, so isConnected() is false and the parser
  // stops after each delivered frame, as it does after a disconnect.
class TestConnection : public FramedTcpConnection
{
  public:
    vector<vector<uint8_t> > frames;

    TestConnection(void)
    {
      frameReceived.connect(
          sigc::mem_fun(*this, &TestConnection::onFrame));
    }

    int feed(const vector<uint8_t>& data)
    {
      vector<uint8_t> buf(data);
      return onDataReceived(buf.data(), static_cast<int>(buf.size()));
    }

  private:
    void onFrame(FramedTcpConnection*, vector<uint8_t>& frame)
    {
      frames.push_back(frame);
    }
};


vector<uint8_t> frame(const string& payload)
{
  uint32_t size = payload.size();
  vector<uint8_t> data;
  data.push_back((size >> 24) & 0xff);
  data.push_back((size >> 16) & 0xff);
  data.push_back((size >> 8) & 0xff);
  data.push_back(size & 0xff);
  data.insert(data.end(), payload.begin(), payload.end());
  return data;
}


void test_frame(void)
{
  TestConnection con;
  int cnt = con.feed(frame("abc"));
  check(cnt == 7, "all bytes of a frame are consumed");
  check((con.frames.size() == 1) && (con.frames[0].size() == 3) &&
        (string(con.frames[0].begin(), con.frames[0].end()) == "abc"),
        "a frame is delivered");
}


void test_frame_in_parts(void)
{
  TestConnection con;
  vector<uint8_t> data(frame("abcd"));
  int cnt = con.feed(vector<uint8_t>(data.begin(), data.begin() + 6));
  check(cnt == 6, "the first part of a frame is consumed");
  check(con.frames.empty(), "an incomplete frame is not delivered");
  cnt = con.feed(vector<uint8_t>(data.begin() + 6, data.end()));
  check(cnt == 2, "the rest of a frame is consumed");
  check((con.frames.size() == 1) && (con.frames[0].size() == 4),
        "a frame received in parts is delivered when complete");
}


void test_incomplete_size(void)
{
  TestConnection con;
  vector<uint8_t> data(frame("ab"));
  int cnt = con.feed(vector<uint8_t>(data.begin(), data.begin() + 3));
  check(cnt == 0, "an incomplete frame size is left in the buffer");
  check(con.frames.empty(), "nothing is delivered for an incomplete size");
}


void test_empty_frame(void)
{
  TestConnection con;
  int cnt = con.feed(frame(""));
  check(cnt == 4, "all bytes of an empty frame are consumed");
  check((con.frames.size() == 1) && con.frames[0].empty(),
        "an empty frame is delivered at once");

  cnt = con.feed(frame("xyz"));
  check(cnt == 7, "all bytes of the next frame are consumed");
  check((con.frames.size() == 2) && (con.frames[1].size() == 3),
        "the frame after an empty frame is delivered");
}

} /* anonymous namespace */


int main(void)
{
  test_frame();
  test_frame_in_parts();
  test_incomplete_size();
  test_empty_frame();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Async::FramedTcpConnection tests passed" << endl;
    return 0;
  }
  cout << failures << " Async::FramedTcpConnection test check(s) FAILED"
       << endl;
  return 1;
}
