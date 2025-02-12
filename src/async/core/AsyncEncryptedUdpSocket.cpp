/**
@file   AsyncEncryptedUdpSocket.cpp
@brief  Contains a class for sending encrypted UDP datagrams
@author Tobias Blomberg / SM0SVX
@date   2023-07-23

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2023 Tobias Blomberg / SM0SVX

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

#include <openssl/rand.h>
#include <openssl/err.h>

#include <cassert>
#include <cstring>
#include <iterator>


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

#include "AsyncEncryptedUdpSocket.h"


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
 * Static class variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

namespace {


/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/



}; /* End of anonymous namespace */

/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

const EncryptedUdpSocket::Cipher* EncryptedUdpSocket::fetchCipher(
    const std::string& type)
{
//#if OPENSSL_VERSION_MAJOR >= 3
//  return EVP_CIPHER_fetch(NULL, type.c_str(), NULL);
//#else
  return EVP_get_cipherbyname(type.c_str());
//#endif
} /* EncryptedUdpSocket::fetchCipher */


void EncryptedUdpSocket::freeCipher(Cipher* cipher)
{
#if OPENSSL_VERSION_MAJOR >= 3
  EVP_CIPHER_free(cipher);
#endif
} /* EncryptedUdpSocket::freeCipher */


const std::string EncryptedUdpSocket::cipherName(
    const EncryptedUdpSocket::Cipher* cipher)
{
  return EVP_CIPHER_name(cipher);
} /* EncryptedUdpSocket::cipherName */


bool EncryptedUdpSocket::randomBytes(std::vector<uint8_t>& bytes)
{
  if (bytes.size() == 0)
  {
    return true;
  }
  return (RAND_bytes(bytes.data(), bytes.size()) == 1);
} /* EncryptedUdpSocket::randomBytes */


EncryptedUdpSocket::EncryptedUdpSocket(uint16_t local_port,
    const IpAddress &bind_ip)
  : UdpSocket(local_port, bind_ip)
{
  m_cipher_ctx = EVP_CIPHER_CTX_new();
} /* EncryptedUdpSocket::EncryptedUdpSocket */


EncryptedUdpSocket::~EncryptedUdpSocket(void)
{
  EVP_CIPHER_CTX_free(m_cipher_ctx);
  m_cipher_ctx = nullptr;
} /* EncryptedUdpSocket::~EncryptedUdpSocket */


bool EncryptedUdpSocket::setCipher(const std::string& type)
{
  //std::cout << "### EncryptedUdpSocket::setCipher: type=" << type << std::endl;
  return setCipher(fetchCipher(type));
} /* EncryptedUdpSocket::setCipher */


bool EncryptedUdpSocket::setCipher(const EncryptedUdpSocket::Cipher* cipher)
{
    // Clean up the context, free all memory except the context itself
  if (!EVP_CIPHER_CTX_reset(m_cipher_ctx))
  {
    std::cout << "### EVP_CIPHER_CTX_reset failed" << std::endl;
    return false;
  }

  if (cipher == nullptr)
  {
    return true;
  }

    // Set cipher type in the cipher context
  if (!EVP_EncryptInit_ex(m_cipher_ctx, cipher, NULL, NULL, NULL) ||
      !EVP_DecryptInit_ex(m_cipher_ctx, cipher, NULL, NULL, NULL))
  {
    std::cout << "### EVP_EncryptInit_ex failed" << std::endl;
    return false;
  }

  return true;
} /* EncryptedUdpSocket::setCipher */


bool EncryptedUdpSocket::setCipherIV(std::vector<uint8_t> iv)
{
  m_cipher_iv = iv;
  size_t iv_length = EVP_CIPHER_CTX_iv_length(m_cipher_ctx);
  //std::cout << "### EncryptedUdpSocket::setCipherIV: iv_length="
  //          << iv_length << " iv.size()=" << iv.size() << std::endl;
  return (iv.size() == iv_length);
} /* EncryptedUdpSocket::setCipherIV */


const std::vector<uint8_t> EncryptedUdpSocket::cipherIV(void) const
{
  return m_cipher_iv;
} /* EncryptedUdpSocket::cipherIV */


bool EncryptedUdpSocket::setCipherKey(std::vector<uint8_t> key)
{
  //std::cout << "### EncryptedUdpSocket::setCipherKey: key.size()="
  //          << key.size() << std::endl;
  m_cipher_key = key;
  size_t key_length = EVP_CIPHER_CTX_key_length(m_cipher_ctx);
  return (key.size() == key_length);
} /* EncryptedUdpSocket::setCipherKey */


bool EncryptedUdpSocket::setCipherKey(void)
{
  std::vector<uint8_t> cipher_key(EVP_CIPHER_CTX_key_length(m_cipher_ctx));
  //std::cout << "### EncryptedUdpSocket::setCipherKey: cipher_key.size()="
  //          << cipher_key.size() << std::endl;
  if (!randomBytes(cipher_key))
  {
    return false;
  }
  return setCipherKey(cipher_key);
} /* EncryptedUdpSocket::setCipherKey */


const std::vector<uint8_t> EncryptedUdpSocket::cipherKey(void) const
{
  return m_cipher_key;
} /* EncryptedUdpSocket::cipherKey */


bool EncryptedUdpSocket::write(const IpAddress& remote_ip, int remote_port,
                               const void *buf, int count)
{
  return write(remote_ip, remote_port, nullptr, 0, buf, count);
} /* EncryptedUdpSocket::write */


bool EncryptedUdpSocket::write(const IpAddress& remote_ip, int remote_port,
                               const void *aad, int aadlen,
                               const void *buf, int cnt)
{
  //std::cout << "### EncryptedUdpSocket::write: "
  //          << "aadlen=" << aadlen << " cnt=" << cnt
  //          << " iv=";
  //std::copy(m_cipher_iv.begin(), m_cipher_iv.end(),
  //    std::ostream_iterator<int>(std::cout << std::hex, " "));
  //std::cout << std::dec << std::endl;

  assert(m_cipher_ctx != nullptr);
  assert((aad == nullptr) == (aadlen <= 0));

  auto inbuf = static_cast<const uint8_t*>(buf);
  auto aadbuf = static_cast<const uint8_t*>(aad);

  auto key_length = EVP_CIPHER_CTX_key_length(m_cipher_ctx);
  //auto iv_length = EVP_CIPHER_CTX_iv_length(m_cipher_ctx);
  //std::cout << "### key_length=" << key_length << std::endl;
  //std::cout << "### iv_length=" << iv_length << std::endl;
  if (key_length > 0)
  {
    //OPENSSL_assert(key_length == m_cipher_key.size());
    //OPENSSL_assert(iv_length == m_cipher_iv.size());

      // Set key and IV in the cipher context
    EVP_EncryptInit_ex(m_cipher_ctx, NULL, NULL, m_cipher_key.data(),
                       m_cipher_iv.data());
  }

  //auto taglen = EVP_CIPHER_CTX_get_tag_length(m_cipher_ctx);
  //std::cout << "### taglen=" << m_taglen << std::endl;

    // Allow enough space in output buffer for AAD, tag, encrypted plaintext
    // and one additional block
  uint8_t outbuf[aadlen + m_taglen + cnt + EVP_MAX_BLOCK_LENGTH];
  auto outbufp = outbuf;
  int outlen = 0;
  int totoutlen = aadlen + m_taglen;
  if (aadlen > 0)
  {
    std::memcpy(outbufp, aadbuf, aadlen);
    if(!EVP_EncryptUpdate(m_cipher_ctx, nullptr, &outlen, aadbuf, aadlen))
    {
      std::cout << "### EVP_EncryptUpdate with AAD failed" << std::endl;
      ERR_print_errors_fp(stderr);
      return false;
    }
  }
  outbufp += aadlen + m_taglen;

  if(!EVP_EncryptUpdate(m_cipher_ctx, outbufp, &outlen, inbuf, cnt))
  {
    std::cout << "### EVP_EncryptUpdate failed" << std::endl;
    return false;
  }
  outbufp += outlen;
  totoutlen += outlen;

  if(!EVP_EncryptFinal_ex(m_cipher_ctx, outbufp, &outlen))
  {
    std::cout << "### EVP_EncryptFinal failed" << std::endl;
    return false;
  }
  totoutlen += outlen;

  if (m_taglen > 0)
  {
    outbufp = outbuf + aadlen;
    if (!EVP_CIPHER_CTX_ctrl(m_cipher_ctx, EVP_CTRL_AEAD_GET_TAG,
          m_taglen, outbufp))
    {
      std::cout << "### EVP_CIPHER_CTX_ctrl(EVP_CTRL_AEAD_GET_TAG) failed"
                << std::endl;
      return false;
    }
  }

  //std::cout << "### EncryptedUdpSocket::write: totoutlen=" << totoutlen
  //          << " data=";
  //std::copy(outbuf, outbuf+totoutlen,
  //    std::ostream_iterator<int>(std::cout << std::hex, " "));
  //std::cout << std::dec << std::endl;

  return UdpSocket::write(remote_ip, remote_port, outbuf, totoutlen);

} /* EncryptedUdpSocket::write */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void EncryptedUdpSocket::onDataReceived(const IpAddress& ip, uint16_t port,
                                        void* buf, int count)
{
  if ((count < 0) || cipherDataReceived(ip, port, buf, count))
  {
    return;
  }

  assert(m_cipher_ctx != nullptr);
  //std::cout << "### EncryptedUdpSocket::onDataReceived: count="
  //          << count << " iv=";
  //std::copy(m_cipher_iv.begin(), m_cipher_iv.end(),
  //    std::ostream_iterator<int>(std::cout << std::hex, " "));
  //std::cout << std::dec << std::endl;

  auto inbuf = static_cast<unsigned char*>(buf);

  /* Allow enough space in output buffer for additional block */
  unsigned char outbuf[count + EVP_MAX_BLOCK_LENGTH];

  auto key_length = EVP_CIPHER_CTX_key_length(m_cipher_ctx);
  //auto iv_length = EVP_CIPHER_CTX_iv_length(m_cipher_ctx);
  //std::cout << "### key_length=" << key_length << std::endl;
  //std::cout << "### iv_length=" << iv_length << std::endl;
  if (key_length > 0)
  {
    //OPENSSL_assert(key_length == m_cipher_key.size());
    //OPENSSL_assert(iv_length == m_cipher_iv.size());

      // Set key and IV in the cipher context
    EVP_DecryptInit_ex(m_cipher_ctx, NULL, NULL, m_cipher_key.data(),
                      m_cipher_iv.data());
  }

  int outlen = 0;
  void* aad = nullptr;
  if (m_aadlen > 0)
  {
    if (static_cast<size_t>(count) < m_aadlen)
    {
      std::cout << "### EncryptedUdpSocket::onDataReceived: count=" << count
                << " m_aadlen=" << m_aadlen << std::endl;
      return;
    }
    if(!EVP_DecryptUpdate(m_cipher_ctx, nullptr, &outlen, inbuf, m_aadlen))
    {
      std::cout << "### : EVP_DecryptUpdate AAD failed" << std::endl;
      return;
    }
    assert(static_cast<size_t>(outlen) == m_aadlen);
    aad = inbuf;
    inbuf += m_aadlen;
    count -= m_aadlen;
  }

  //auto taglen = EVP_CIPHER_CTX_get_tag_length(m_cipher_ctx);
  //std::cout << "### taglen=" << m_taglen << std::endl;
  if (m_taglen > 0)
  {
    if (static_cast<size_t>(count) < m_taglen)
    {
      std::cout << "### Required tag does not fit within incoming data"
                << std::endl;
      return;
    }
    if (!EVP_CIPHER_CTX_ctrl(
          m_cipher_ctx, EVP_CTRL_AEAD_SET_TAG, m_taglen, inbuf))
    {
      std::cout << "### EVP_CIPHER_CTX_ctrl(EVP_CTRL_AEAD_SET_TAG) failed"
                << std::endl;
      return;
    }
    inbuf += m_taglen;
    count -= m_taglen;
  }

  if(!EVP_DecryptUpdate(m_cipher_ctx, outbuf, &outlen, inbuf, count))
  {
    std::cout << "### EVP_DecryptUpdate failed" << std::endl;
    return;
  }

  int totoutlen = outlen;
  if(!EVP_DecryptFinal_ex(m_cipher_ctx, outbuf+outlen, &outlen))
  {
    std::cout << "### EVP_DecryptFinal_ex failed" << std::endl;
    return;
  }
  totoutlen += outlen;

  //std::cout << "### EncryptedUdpSocket::onDataReceived: totoutlen="
  //          << totoutlen << std::endl;

  dataReceived(ip, port, aad, outbuf, totoutlen);
} /* EncryptedUdpSocket::onDataReceived */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
