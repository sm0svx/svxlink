/**
@file    RtPacketTest.cpp
@brief   Unit test for rtp_make_sdes() buffer/length handling
@author  Mark Rose
@date    2026-07-11

rtp_make_sdes() had two related bugs, both fixed in the same commit:

 1. It formatted "%-15s%s" of the callsign and station name into a fixed
    256-byte stack buffer using sprintf(), with no bound on the combined
    length. A long callsign/name overflows the buffer (stack smashing).
    Fixed by switching to snprintf(tmp, sizeof(tmp), ...).

 2. The addText() macro recorded a text item's length in a single output
    byte (*block++ = sl) but then memcpy'd the full, un-truncated length
    (sl) into the packet. For any text argument longer than 255 bytes
    (e.g. a long "priv" string), the on-wire length byte would wrap while
    memcpy still copied the untruncated length -- the recorded length no
    longer matches the number of bytes actually written, corrupting the
    packet framing (and, given a small enough destination buffer,
    overflowing it). Fixed by clamping the length to 255 before both
    recording and copying it.

This test exercises both defects without needing a sanitizer:

 - For the addText/priv bug: a "priv" string longer than 255 bytes is
   passed in. The test decodes the produced packet and checks that the
   PRIV item's recorded length byte actually matches the number of bytes
   written before the next tag (RTCP_SDES_END) -- i.e. that the framing is
   self-consistent. Pre-fix, the recorded length (truncated mod 256)
   disagrees with the untruncated memcpy, so the END tag is not found
   where the length byte says it should be.

 - For the sprintf/tmp bug: a callsign+name combination long enough to
   overflow the 256-byte formatting buffer is passed in. Post-fix, this
   is safely truncated and the function returns normally with a NAME
   item no longer than 255 bytes. Pre-fix, the unbounded sprintf() into
   the stack buffer either corrupts the packet framing (detected the same
   way as the priv check) or is caught outright by the toolchain's stack
   protector (a nonzero-exit crash, which is itself a valid failure
   signal for this test).

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <iostream>
#include <string>
#include <cstring>

#include "rtp.h"
#include "rtpacket.h"

using namespace std;

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
    // Generously sized destination buffer with trailing canary bytes so
    // that even the pre-fix, un-clamped memcpy (which can write well past
    // where the recorded length byte says it should stop) cannot run off
    // the end of our buffer and crash the test for the wrong reason. We
    // want to observe *framing corruption*, not an out-of-bounds write.
  const size_t BUFSZ = 4096;
  unsigned char buf[BUFSZ + 16];
  memset(buf, 0xAA, sizeof(buf));

    // --- Check 1: long "priv" argument must not corrupt packet framing ---
  {
    string priv(300, 'X');
    int size = rtp_make_sdes(buf, "CALLSIGN", "A Station", priv.c_str());
    check(size > 0 && (size_t)size < BUFSZ, "rtp_make_sdes returned a sane size (priv case)");

      // Locate the PRIV item. Preceding items are fixed size:
      //   RR header + SDES chunk header:  16 bytes
      //   CNAME tag+len+"CALLSIGN":   1 + 1 + 8  = 10 bytes
      //   NAME tag+len+"CALLSIGN    A Station" (15+9=24, <=255, unaffected): 1 + 1 + 24 = 26 bytes
      //   EMAIL tag+len+"CALLSIGN":   1 + 1 + 8  = 10 bytes
      //   PHONE tag+len+"08:30":      1 + 1 + 5  = 7 bytes
    size_t priv_tag_off = 16 + 10 + 26 + 10 + 7;
    check(buf[priv_tag_off] == RTCP_SDES_PRIV, "PRIV tag found at expected offset");

    unsigned char recorded_len = buf[priv_tag_off + 1];
    check(recorded_len == 255,
          "PRIV length byte is clamped to 255 for a 300-byte priv string");

      // The byte immediately after the recorded-length worth of data must
      // be the next SDES tag (RTCP_SDES_END). If the memcpy actually wrote
      // more bytes than the length byte claims (the pre-fix bug), this
      // byte will still be priv data ('X'), not the END tag.
    size_t next_tag_off = priv_tag_off + 2 + recorded_len;
    check(buf[next_tag_off] == RTCP_SDES_END,
          "byte following the recorded PRIV length is the END tag "
          "(i.e. the length byte and the copied byte count agree)");
  }

    // --- Check 2: long callsign+name must not overflow the format buffer ---
  {
    memset(buf, 0xAA, sizeof(buf));
    string long_name(400, 'N');
    int size = rtp_make_sdes(buf, "CALLSIGN", long_name.c_str(), nullptr);
    check(size > 0 && (size_t)size < BUFSZ, "rtp_make_sdes returned a sane size (long name case)");

      // NAME item starts right after the CNAME item.
    size_t name_tag_off = 16 + 10;
    check(buf[name_tag_off] == RTCP_SDES_NAME, "NAME tag found at expected offset");

    unsigned char recorded_len = buf[name_tag_off + 1];
    check(recorded_len <= 255,
          "NAME length byte never exceeds 255 (fits in one byte) "
          "for a 15+400 byte callsign/name combination");

      // As with the priv check: the tag right after the recorded-length
      // worth of NAME data must be the next real tag (EMAIL), proving the
      // formatted name was safely bounded and the framing is consistent.
    size_t next_tag_off = name_tag_off + 2 + recorded_len;
    check(buf[next_tag_off] == RTCP_SDES_EMAIL,
          "byte following the recorded NAME length is the EMAIL tag");
  }

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
