/**
@file   AsyncAudioContainerOpus.cpp
@brief  Handle Ogg/Opus type audio container
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstring>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioContainerOpus.h"
#include "AsyncAudioEncoder.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioContainerOpus::AudioContainerOpus(void)
{
  m_enc = AudioEncoder::create("OPUS");
  assert(m_enc != 0);
  std::ostringstream ss;
  ss << FRAME_SIZE;
  m_enc->setOption("FRAME_SIZE", ss.str());
  setHandler(m_enc);
  m_enc->writeEncodedSamples.connect(
      sigc::mem_fun(*this, &AudioContainerOpus::onWriteEncodedSamples));

  //int ret = ogg_stream_init(&m_ogg_stream, m_ogg_serial++);
  int ret = ogg_stream_init(&m_ogg_stream, 1);
  assert(ret == 0);
  ret = writeOggOpusHeader();
} /* AudioContainerOpus::AudioContainerOpus */


AudioContainerOpus::~AudioContainerOpus(void)
{
  clearHandler();
  delete m_enc;
  m_enc = 0;
  int ret = ogg_stream_clear(&m_ogg_stream);
  assert(ret == 0);
} /* AudioContainerOpus::~AudioContainerOpus */


void AudioContainerOpus::endStream(void)
{
  m_enc->flushSamples();

    // Assemble nil Ogg page
  oggpack_buffer oggbuf;
  oggpack_writeinit(&oggbuf);
  oggpackWriteString(&oggbuf, "OggS", 0);             // Magic Signature
  oggpack_write(&oggbuf, 0x00, 8);                    // Stream structure rev
  oggpack_write(&oggbuf, 0x04, 8);                    // EOS flag
  oggpack_write(&oggbuf, 0xffffffff, 32);             // Absolute granule pos
  oggpack_write(&oggbuf, 0xffffffff, 32);             // Absolute granule pos
  oggpack_write(&oggbuf, m_ogg_stream.serialno, 32);  // Stream serial number
  oggpack_write(&oggbuf, m_ogg_stream.pageno, 32);    // Page sequence no
  oggpack_write(&oggbuf, 0, 32);                      // Page checksum
  oggpack_write(&oggbuf, 0, 8);                       // Page segments

  ogg_page page;
  page.header = oggpack_get_buffer(&oggbuf);
  page.header_len = oggpack_bytes(&oggbuf);
  page.body = 0;
  page.body_len = 0;
  ogg_page_checksum_set(&page);

  m_block.clear();
  writePage(page, m_block);
  writeBlock(m_block.data(), m_block.size());

  oggpack_writeclear(&oggbuf);
} /* AudioContainerOpus::endStream */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void AudioContainerOpus::onWriteEncodedSamples(const void *data, int len)
{
  //std::cout << "### AudioContainerOpus::onWriteEncodedSamples: len="
  //          << len << std::endl;

  //if (m_packet.packet == nullptr)
  //{
  //  int ret = ogg_stream_init(&m_ogg_stream, m_ogg_serial++);
  //  assert(ret == 0);
  //  writeOggOpusHeader();

  //  m_packet.packet = reinterpret_cast<unsigned char*>(malloc(len));
  //  assert(m_packet.packet != nullptr);
  //  std::memcpy(m_packet.packet, data, len);
  //  m_packet.bytes = len;
  //  m_packet.b_o_s = 0;
  //  m_packet.e_o_s = 0;
  //  m_packet.granulepos = 48000 * FRAME_SIZE / 1000;
  //  m_packet.packetno = 2;
  //  return;
  //}

  m_packet.packet = const_cast<unsigned char*>(
      reinterpret_cast<const unsigned char*>(data));
  m_packet.bytes = len;
  //m_packet.b_o_s = 0;
  //m_packet.e_o_s = 0;
  if (len > 0)
  {
    m_packet.granulepos += 48000 * FRAME_SIZE / 1000;
  }
  m_packet.packetno += 1;

  if (ogg_stream_packetin(&m_ogg_stream, &m_packet) != 0)
  {
    std::cerr << "*** ERROR: Could not add Ogg packet to stream" << std::endl;
    return;
  }
  m_pending_packets += 1;

  //free(m_packet.packet);
  //m_packet.packet = reinterpret_cast<unsigned char*>(malloc(len));
  //assert(m_packet.packet != nullptr);
  //std::memcpy(m_packet.packet, data, len);
  //m_packet.bytes = len;
  ////m_packet.b_o_s = 0;
  ////m_packet.e_o_s = 0;
  //m_packet.granulepos += 48000 * FRAME_SIZE / 1000;
  //m_packet.packetno += 1;

  ogg_page page;
  while ((ogg_stream_pageout(&m_ogg_stream, &page) != 0) ||
         (((m_pending_packets >= 5) || m_packet.e_o_s) &&
          (ogg_stream_flush(&m_ogg_stream, &page) != 0)))
  {
    m_block.clear();
    writePage(page, m_block);
    writeBlock(m_block.data(), m_block.size());
    assert(ogg_page_packets(&page) <= m_pending_packets);
    m_pending_packets -= ogg_page_packets(&page);
  }

  if (ogg_stream_check(&m_ogg_stream) != 0)
  {
    printf("### Ogg stream error\n");
  }
} /* AudioContainerOpus::onWriteEncodedSamples */


void AudioContainerOpus::oggpackWriteString(oggpack_buffer* oggbuf,
                                            const char *str, int lenbits)
{
  size_t len = std::strlen(str);
  if (lenbits > 0)
  {
    oggpack_write(oggbuf, len, lenbits);
  }
  char ch;
  while ((ch = *str++))
  {
    oggpack_write(oggbuf, ch, 8);
  }
} /* AudioContainerOpus::oggpackWriteString */


void AudioContainerOpus::oggpackWriteCommentList(oggpack_buffer* oggbuf,
    const std::vector<const char*> &comments)
{
  oggpack_write(oggbuf, comments.size(), 32); // User Comment List Length
  for (auto& comment : comments)
  {
    oggpackWriteString(oggbuf, comment);
  }
} /* AudioContainerOpus::oggpackWriteCommentList */


bool AudioContainerOpus::writePage(const ogg_page& page, std::vector<char>& buf)
{
  //std::cout << "### Writing Ogg/Opus page:"
  //          << " serialno=" << ogg_page_serialno(&page)
  //          << " pageno=" << ogg_page_pageno(&page)
  //          << " gpos=" << ogg_page_granulepos(&page)
  //          << " cont=" << ogg_page_continued(&page)
  //          << " pkts=" << ogg_page_packets(&page)
  //          << " bos=" << ogg_page_bos(&page)
  //          << " eos=" << ogg_page_eos(&page)
  //          << " ver=" << ogg_page_version(&page)
  //          << std::endl;
  buf.insert(buf.end(), page.header, page.header + page.header_len);
  if (page.body_len > 0)
  {
    buf.insert(buf.end(), page.body, page.body + page.body_len);
  }
  return true;
} /* AudioContainerOpus::writePage */


bool AudioContainerOpus::writeOggOpusHeader(void)
{
  m_header.clear();

    // Assemble Opus ID Header buffer
  oggpack_buffer oggbuf;
  oggpack_writeinit(&oggbuf);
  oggpackWriteString(&oggbuf, "OpusHead", 0);       // Magic Signature
  oggpack_write(&oggbuf, 1, 8);                     // Version
  oggpack_write(&oggbuf, 1, 8);                     // Channel Count
  oggpack_write(&oggbuf, 48000 * FRAME_SIZE / 1000, 16); // Pre-skip
  oggpack_write(&oggbuf, INTERNAL_SAMPLE_RATE, 32); // Sample Rate
  oggpack_write(&oggbuf, 0, 16);                    // Output Gain
  oggpack_write(&oggbuf, 0, 8);                     // Mapping Family

    // Put Opus ID Header in an Ogg packet and add it to the stream
  ogg_packet oggpkt;
  oggpkt.packet = oggpack_get_buffer(&oggbuf);
  oggpkt.bytes = oggpack_bytes(&oggbuf);
  oggpkt.b_o_s = 1;
  oggpkt.e_o_s = 0;
  oggpkt.granulepos = 0;
  oggpkt.packetno = 0;
  if (ogg_stream_packetin(&m_ogg_stream, &oggpkt) != 0)
  {
    std::cerr << "*** ERROR: Could not add Ogg packet to stream" << std::endl;
    return false;
  }
  oggpack_writeclear(&oggbuf);

    // Flush the stream, get the Opus ID Header Ogg page and send it
  ogg_page page;
  if (ogg_stream_flush(&m_ogg_stream, &page) == 0)
  {
    std::cerr << "*** ERROR: Added Ogg packet produced no page" << std::endl;
    return false;
  }
  if (!writePage(page, m_header))
  {
    std::cout << "### Ogg Header: ogg_stream_flush" << std::endl;
    return false;
  }

    // Assemble Opus Comment Header buffer
  oggpack_writeinit(&oggbuf);
  oggpackWriteString(&oggbuf, "OpusTags", 0); // Magic Signature
  oggpackWriteString(&oggbuf, "SvxLink");     // Vendor String
  oggpackWriteCommentList(&oggbuf, {          // User Comment List
      //"R128_TRACK_GAIN=1536",
      //"R128_ALBUM_GAIN=1536",
      "TITLE=SvxLink Audio Stream",
      "DESCRIPTION=A SvxLink audio stream",
      //"DATE=2020-02-07",
      "GENRE=Ham Radio"
      });

    // Put Opus Comment Header in an Ogg packet and add it to the stream
  oggpkt.packet = oggpack_get_buffer(&oggbuf);
  oggpkt.bytes = oggpack_bytes(&oggbuf);
  oggpkt.b_o_s = 0;
  oggpkt.e_o_s = 0;
  oggpkt.granulepos = 0;
  oggpkt.packetno = 1;
  if (ogg_stream_packetin(&m_ogg_stream, &oggpkt) != 0)
  {
    std::cerr << "*** ERROR: Could not add Ogg packet to stream" << std::endl;
    return false;
  }
  oggpack_writeclear(&oggbuf);

    // Flush the stream, get the Opus Comment Header Ogg pages and send them
  while (ogg_stream_flush(&m_ogg_stream, &page) != 0)
  {
    //std::cout << "### Comment Header: ogg_stream_flush" << std::endl;
    if (!writePage(page, m_header))
    {
      return false;
    }
  }

  return true;
} /* AudioContainerOpus::writeOggOpusHeader */


/*
 * This file has not been truncated
 */
