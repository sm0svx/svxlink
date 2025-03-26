/**
@file   AsyncSslKeypair.h
@brief  Represent private and public keys
@author Tobias Blomberg / SM0SVX
@date   2020-08-03

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

/** @example AsyncSslX509_demo.cpp
An example of how to use the Async::SslKeypair class
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
@brief  A class representing private and public keys
@author Tobias Blomberg / SM0SVX
@date   2024-07-11

\include AsyncX509_demo.cpp
*/
class SslKeypair
{
  public:
    /**
     * @brief   Default constructor
     */
    SslKeypair(void) {}

    /**
     * @brief   Constructor
     * @param   pkey A pointer to an existing EVP_PKEY object
     */
    explicit SslKeypair(EVP_PKEY* pkey)
    {
      m_pkey = pkey;
    }

    /**
     * @brief   Move constructor
     * @param   other The object to move from
     */
    SslKeypair(SslKeypair&& other)
    {
      m_pkey = other.m_pkey;
      other.m_pkey = nullptr;
    }

    /**
     * @brief   Copy constructor
     * @param   other The other object to copy
     */
    SslKeypair(SslKeypair& other)
    {
      EVP_PKEY_up_ref(other.m_pkey);
      m_pkey = other.m_pkey;
    }

    /**
     * @brief   Copy assignment operator
     * @param   other The object to copy from
     * @return  Returns a reference to this object
     */
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
     * @brief   Check if the object is empty
     * @return  Returns \em true if the object is empty/uninitialized
     */
    bool isNull(void) const { return (m_pkey == nullptr); }

    /**
     * @brief   Generate a new RSA keypair
     * @param   bits Modulus size in number of bits
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Generate a key using the given algorithm and raw key data
     * @param   type The algorithm to use
     * @param   key The raw key data
     * @return  Returns \em true on success
     *
     * To fully understand what this function do, read the documentation for
     * the OpenSSL function EVP_PKEY_new_raw_private_key.
     *
     * Ex: newRawPrivateKey(EVP_PKEY_HMAC, key);
     */
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

    /**
     * @brief   Return the private key on PEM form
     * @return  Returns a sting containing PEM data
     */
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

    /**
     * @brief   Create key from the given PEM data
     * @param   pem A sting containing PEM data
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Write key data to file on PEM format
     * @param   filename The path to the file to write
     * @return  Returns \rm true on success
     */
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

    /**
     * @brief   Read key data from PEM file
     * @param   filename The path to the file to read PEM data from
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Get the public key on PEM form
     * @return  Returns a string containing PEM data
     */
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

    /**
     * @brief   Create public key from PEM string
     * @param   pem A string containing PEM data
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Cast to pointer to EVP_PKEY
     * @return  Returns a pointer to the internal EVP_PKEY object
     */
    operator EVP_PKEY*(void) { return m_pkey; }
    operator const EVP_PKEY*(void) const { return m_pkey; }

    /**
     * @brief   Check if two keys is not equal to each other
     * @param   other The other key to use in the comparison
     * @return  Returns \em true if the keys are not the same
     */
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
