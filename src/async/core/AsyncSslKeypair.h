/**
@file   MyNamespaceTemplate.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2020-08-03

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
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

/** @example MyNamespaceTemplate_demo.cpp
An example of how to use the SslKeypair class
*/

#ifndef ASYNC_SSL_KEYPAIR_INCLUDED
#define ASYNC_SSL_KEYPAIR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>

#include <cassert>
#include <string>


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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2020-

A_detailed_class_description

\include MyNamespaceTemplate_demo.cpp
*/
class SslKeypair
{
  public:
    /**
     * @brief   Default constructor
     */
    SslKeypair(void) {}

    explicit SslKeypair(EVP_PKEY* pkey)
    {
      m_pkey = pkey;
    }

    SslKeypair(SslKeypair&& other)
    {
      m_pkey = other.m_pkey;
      other.m_pkey = nullptr;
    }

    SslKeypair(SslKeypair& other)
    {
      EVP_PKEY_up_ref(other.m_pkey);
      m_pkey = other.m_pkey;
    }

    SslKeypair& operator=(SslKeypair& other)
    {
      EVP_PKEY_up_ref(other.m_pkey);
      m_pkey = other.m_pkey;
      return *this;
    }

    /**
     * @brief   Destructor
     */
    ~SslKeypair(void)
    {
      EVP_PKEY_free(m_pkey);
      m_pkey = nullptr;
    }

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    bool isNull(void) const { return (m_pkey == nullptr); }

    bool generate(unsigned int bits)
    {
      EVP_PKEY_free(m_pkey);
#if OPENSSL_VERSION_MAJOR >= 3
      m_pkey = EVP_RSA_gen(bits);
      return (m_pkey != nullptr);
#else
      m_pkey = EVP_PKEY_new();
      if (m_pkey == nullptr)
      {
        return false;
      }

      BIGNUM* rsa_f4 = BN_new();
      if (rsa_f4 == nullptr)
      {
        EVP_PKEY_free(m_pkey);
        m_pkey = nullptr;
        return false;
      }
      BN_set_word(rsa_f4, RSA_F4);

        // FIXME: Accoring to the manual page we need to seed the random number
        // generator. How?!
      RSA* rsa = RSA_new();
      if (rsa == nullptr)
      {
        BN_free(rsa_f4);
        EVP_PKEY_free(m_pkey);
        m_pkey = nullptr;
        return false;
      }
      int ret = RSA_generate_key_ex(
          rsa,    /* the RSA object to fill in */
          bits,   /* number of bits for the key - 2048 is a sensible value */
          rsa_f4, /* exponent - RSA_F4 is defined as 0x10001L */
          NULL    /* callback - can be NULL if we aren't displaying progress */
      );
      if (ret != 1)
      {
        RSA_free(rsa);
        BN_free(rsa_f4);
        EVP_PKEY_free(m_pkey);
        m_pkey = nullptr;
        return false;
      }
      ret = EVP_PKEY_assign_RSA(m_pkey, rsa);
      if (ret != 1)
      {
        RSA_free(rsa);
        BN_free(rsa_f4);
        EVP_PKEY_free(m_pkey);
        m_pkey = nullptr;
        return false;
      }
      BN_free(rsa_f4);
      return true;
#endif
    }

    template <class T>
    bool newRawPrivateKey(int type, const T& key)
    {
      EVP_PKEY_free(m_pkey);
#if OPENSSL_VERSION_MAJOR >= 3
      m_pkey = EVP_PKEY_new_raw_private_key(type, nullptr,
          reinterpret_cast<const unsigned char*>(key.data()), key.size());
#else
      m_pkey = EVP_PKEY_new_mac_key(type, nullptr, 
          reinterpret_cast<const unsigned char*>(key.data()), key.size());
#endif
      return (m_pkey != nullptr);
    }

    std::string privateKeyPem(void) const
    {
      assert(m_pkey != nullptr);
      BIO *mem = BIO_new(BIO_s_mem());
      assert(mem != nullptr);
      int ret = PEM_write_bio_PrivateKey(
          mem,    /* use the FILE* that was opened */
          m_pkey, /* EVP_PKEY structure */
          NULL,   /* default cipher for encrypting the key on disk */
          NULL,   /* passphrase required for decrypting the key on disk */
          0,      /* length of the passphrase string */
          NULL,   /* callback for requesting a password */
          NULL    /* data to pass to the callback */
      );
      std::string pem;
      if (ret == 1)
      {
        char buf[16384];
        int len = BIO_read(mem, buf, sizeof(buf));
        assert(len > 0);
        pem = std::string(buf, len);
      }
      BIO_free(mem);
      return pem;
    }

    bool privateKeyFromPem(const std::string& pem)
    {
      BIO *mem = BIO_new(BIO_s_mem());
      BIO_puts(mem, pem.c_str());
      if (m_pkey != nullptr)
      {
        EVP_PKEY_free(m_pkey);
      }
      m_pkey = PEM_read_bio_PrivateKey(mem, nullptr, nullptr, nullptr);
      BIO_free(mem);
      return (m_pkey != nullptr);
    }

    bool writePrivateKeyFile(const std::string& filename)
    {
      FILE* f = fopen(filename.c_str(), "wb");
      if (f == nullptr)
      {
        return false;
      }
      if (fchmod(fileno(f), 0600) != 0)
      {
        fclose(f);
        return false;
      }
      int ret = PEM_write_PrivateKey(
          f,      /* use the FILE* that was opened */
          m_pkey, /* EVP_PKEY structure */
          NULL,   /* default cipher for encrypting the key on disk */
          NULL,   /* passphrase required for decrypting the key on disk */
          0,      /* length of the passphrase string */
          NULL,   /* callback for requesting a password */
          NULL    /* data to pass to the callback */
      );
      if (ret != 1)
      {
        fclose(f);
        return false;
      }
      if (fclose(f) != 0)
      {
        return false;
      }
      return true;
    }

    bool readPrivateKeyFile(const std::string& filename)
    {
      FILE* f = fopen(filename.c_str(), "rb");
      if (f == nullptr)
      {
        return false;
      }
      EVP_PKEY* pkey = PEM_read_PrivateKey(f, NULL, NULL, NULL);
      if (pkey == nullptr)
      {
        fclose(f);
        return false;
      }
      if (fclose(f) != 0)
      {
        return false;
      }
      EVP_PKEY_free(m_pkey);
      m_pkey = pkey;
      return true;
    }

    std::string publicKeyPem(void) const
    {
      assert(m_pkey != nullptr);
      BIO *mem = BIO_new(BIO_s_mem());
      assert(mem != nullptr);
      int ret = PEM_write_bio_PUBKEY(mem, m_pkey);
      std::string pem;
      if (ret == 1)
      {
        char buf[16384];
        int len = BIO_read(mem, buf, sizeof(buf));
        assert(len > 0);
        pem = std::string(buf, len);
      }
      BIO_free(mem);
      return pem;
    }

    bool publicKeyFromPem(const std::string& pem)
    {
      if (m_pkey != nullptr)
      {
        EVP_PKEY_free(m_pkey);
      }
      BIO* mem = BIO_new(BIO_s_mem());
      assert(mem != nullptr);
      int rc = BIO_puts(mem, pem.c_str());
      std::cout << "### rc=" << rc << std::endl;
      if (rc > 0)
      {
        m_pkey = PEM_read_bio_PUBKEY(mem, nullptr, nullptr, nullptr);
      }
      BIO_free(mem);
      return (m_pkey != nullptr);
    }

    operator EVP_PKEY*(void) { return m_pkey; }
    operator const EVP_PKEY*(void) const { return m_pkey; }

    bool operator!=(const SslKeypair& other) const
    {
#if OPENSSL_VERSION_MAJOR >= 3
      return (EVP_PKEY_eq(m_pkey, other.m_pkey) != 1);
#else
      return (EVP_PKEY_cmp(m_pkey, other.m_pkey) != 1);
#endif
    }

  protected:

  private:
    EVP_PKEY* m_pkey = nullptr;

};  /* class SslKeypair */


} /* namespace */

#endif /* ASYNC_SSL_KEYPAIR_INCLUDED */

/*
 * This file has not been truncated
 */
