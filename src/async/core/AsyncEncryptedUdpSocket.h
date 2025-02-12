/**
@file   AsyncEncryptedUdpSocket.h
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

/** @example AsyncEncryptedUdpSocket_demo.cpp
An example of how to use the Async::EncryptedUdpSocket class
*/

#ifndef ASYNC_ENCRYPTED_UDP_SOCKET_INCLUDED
#define ASYNC_ENCRYPTED_UDP_SOCKET_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <openssl/evp.h>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncUdpSocket.h>


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
@brief  A class for sending encrypted UDP datagrams
@author Tobias Blomberg / SM0SVX
@date   2023-07-23

Use this class to create a UDP socket that is used for sending and receiving
encrypted UDP datagrams. The available ciphers are the block ciphers provided
by the OpenSSL library, e.g. AES-128-GCM.

\include AsyncEncryptedUdpSocket_demo.cpp
*/
class EncryptedUdpSocket : public UdpSocket
{
  public:
    using Cipher = EVP_CIPHER;

    /**
     * @brief   Fetch a named cipher object
     * @param   name The name of the cipher
     * @return  Return a pointer to a cipher object
     *
     * Use this function to fetch a cipher object using its name, e.g.
     * AES-128-GCM. The returned object must be freed using the freeCipher
     * function if not used with the setCipher function. If the setCipher
     * function has been used, the object does not have to be freed.
     */
    static const Cipher* fetchCipher(const std::string& name);

    /**
     * @brief   Free memory for a previously allocated cipher object
     * @param   cipher The cipher object to free
     */
    static void freeCipher(Cipher* cipher);

    /**
     * @brief   Get the name of a cipher from a cipher object
     * @param   cipher The cipher object
     * @return  Returns the name of the cipher
     */
    static const std::string cipherName(const Cipher* cipher);

    /**
     * @brief   Fill a vector with random bytes
     * @param   bytes The vector to fill
     * @return  Returns \em true on success
     *
     * This function will fill the given vector with random bytes. Set the
     * vector size to the number of bytes that should be generated. A zero
     * length vector is valid and will always return true.
     * A cryptographically secure pseudo random generator (CSPRNG) is used to
     * generate the bytes.
     */
    static bool randomBytes(std::vector<uint8_t>& bytes);

    /**
     * @brief   Constructor
     * @param   local_port  The local UDP port to bind to, 0=ephemeral
     * @param   bind_ip     The local interface (IP) to bind to
     */
    EncryptedUdpSocket(uint16_t local_port=0,
        const IpAddress &bind_ip=IpAddress());

    /**
     * @brief   Disallow copy construction
     */
    //EncryptedUdpSocket(const EncryptedUdpSocket&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    //EncryptedUdpSocket& operator=(const EncryptedUdpSocket&) = delete;

    /**
     * @brief   Destructor
     */
    ~EncryptedUdpSocket(void) override;

    /**
     * @brief   Check if the initialization was ok
     * @return  Returns \em true if everything went fine during initialization
     *          or \em false if something went wrong
     *
     * This function should always be called after constructing the object to
     * see if everything went fine.
     */
    bool initOk(void) const override
    {
      return UdpSocket::initOk() && (m_cipher_ctx != nullptr);
    }

    /**
     * @brief   Set which cipher algorithm type to use
     * @param   type The algorithm type
     * @return  Return \em true on success
     *
     * This function must be called before sending or receiving any datagrams.
     * Use this function to set which block cipher algorithm to use, e.g.
     * AES-128-GCM, ChaCha20, NULL.
     */
    bool setCipher(const std::string& type);

    /**
     * @brief   Set which cipher algorithm type to use
     * @param   cipher A pre-created cipher object
     * @return  Return \em true on success
     *
     * The setCipher function must be called before sending or receiving any
     * datagrams. Use this function to set which block cipher algorithm to use.
     */
    bool setCipher(const Cipher* cipher);

    /**
     * @brief   Set the initialization vector to use with the cipher
     * @param   iv The initialization vector
     * @return  Returns \em true on success
     *
     * This function will set the initialization vector (IV) to use with the
     * selected cipher. Different ciphers require different IVs. Find and read
     * the requirements for a specific cipher for constructing a safe IV.
     * The setCipher function must be called before calling this function.
     */
    bool setCipherIV(std::vector<uint8_t> iv);

    /**
     * @brief   Get a previously set initialization vector (IV)
     * @return  Returns the IV or an empty vector if not set
     */
    const std::vector<uint8_t> cipherIV(void) const;

    /**
     * @brief   Set the cipher key to use
     * @param   key The cipher key
     * @return  Returns \em true on success
     *
     * This function will set the key to use with the selected cipher.
     * Different ciphers require different keys. Find and read the requirements
     * for a specific cipher for constructing a key. The setCipher function
     * must be called before calling this function.
     */
    bool setCipherKey(std::vector<uint8_t> key);

    /**
     * @brief   Set a random cipher key to use
     * @return  Returns \em true on success
     *
     * This function will set a random key to use with the selected cipher. A
     * cryptographically secure pseudo random generator (CSPRNG) is used to
     * generate the key.
     * The setCipher function must be called before calling this function.
     */
    bool setCipherKey(void);

    /**
     * @brief   Get the currently set cipher key
     * @return  Returns the key or an empty vector if the key is not set
     */
    const std::vector<uint8_t> cipherKey(void) const;

    /**
     * @brief   Set the length of the AEAD tag
     * @param   taglen The length of the tag in bytes
     *
     * Some ciphers, like AES-128-GCM, support AEAD (Authenticated Encryption
     * with Associated Data). A tag is then sent with the encrypted data to
     * authenticate the sender of the data. The tag can have differing lengths
     * for different applications and different levels of security.
     */
    void setTagLength(int taglen) { m_taglen = taglen; }

    /**
     * @brief   Get the currently set up tag length
     * @return  Returns the tag length
     */
    int tagLength(void) const { return m_taglen; }

    /**
     * @brief   Set the length of the associated data for AEAD ciphers
     * @param   aadlen The length of the additional associated data
     *
     * Some ciphers, like AES-128-GCM, support AEAD (Authenticated Encryption
     * with Associated Data). A tag is then sent with the encrypted data to
     * authenticate the sender of the data. Associated data, which is not
     * encypted, can be sent along with the encrypted data. The associated data
     * will be protected by the authentication present in AEAD ciphers if a tag
     * is sent along with the encrypted data (@see setTagLength).
     */
    void setCipherAADLength(int aadlen) { m_aadlen = aadlen; }

    /**
     * @brief   The currently set up length of the additional associated data
     * @return  Returns the length of the associated data
     */
    size_t cipherAADLength(void) const { return m_aadlen; }

    /**
     * @brief   Write data to the remote host
     * @param   remote_ip   The IP-address of the remote host
     * @param   remote_port The remote port to use
     * @param   buf         A buffer containing the data to send
     * @param   count       The number of bytes to write
     * @return  Return \em true on success or \em false on failure
     */
    bool write(const IpAddress& remote_ip, int remote_port,
               const void *buf, int count) override;

    /**
     * @brief   Write data to the remote host
     * @param   remote_ip   The IP-address of the remote host
     * @param   remote_port The remote port to use
     * @param   aad         Prepended unencrypted data
     * @param   buf         A buffer containing the data to send
     * @param   count       The number of bytes to write
     * @return  Return \em true on success or \em false on failure
     */
    bool write(const IpAddress& remote_ip, int remote_port,
               const void *aad, int aadlen, const void *buf, int cnt);

    /**
     * @brief   A signal that is emitted when cipher data has been received
     * @param   ip    The IP-address the data was received from
     * @param   port  The remote port number
     * @param   buf   The buffer containing the read cipher data
     * @param   count The number of bytes read
     */
    sigc::signal<bool, const IpAddress&, uint16_t,
                 void*, int> cipherDataReceived;

    /**
     * @brief 	A signal that is emitted when cipher data has been decrypted
     * @param 	ip    The IP-address the data was received from
     * @param   port  The remote port number
     * @param   aad   Additional Associated Data
     * @param 	buf   The buffer containing the read data
     * @param 	count The number of bytes read
     */
    sigc::signal<void, const IpAddress&, uint16_t,
                 void*, void*, int> dataReceived;

  protected:
    void onDataReceived(const IpAddress& ip, uint16_t port, void* buf,
        int count) override;

  private:
    EVP_CIPHER_CTX*       m_cipher_ctx  = nullptr;
    std::vector<uint8_t>  m_cipher_iv;
    std::vector<uint8_t>  m_cipher_key;
    size_t                m_taglen      = 0;
    size_t                m_aadlen      = 0;

};  /* class EncryptedUdpSocket */


} /* namespace Async */

#endif /* ASYNC_ENCRYPTED_UDP_SOCKET_INCLUDED */

/*
 * This file has not been truncated
 */
