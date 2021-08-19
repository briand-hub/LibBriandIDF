/* 
    Briand IDF Library https://github.com/briand-hub/LibBriandIDF
    Copyright (C) 2021 Author: briand (https://github.com/briand-hub)
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "BriandESPHeapOptimize.hxx"

// Sockets
#if defined(ESP_PLATFORM)
    #include <lwip/sys.h>
	#include <lwip/dns.h>
	#include <lwip/sockets.h>
	#include <lwip/netdb.h>
	#include <sys/select.h>
#elif defined(__linux__)
	#include "BriandEspLinuxPorting.hxx"
#else
    #error "UNSUPPORTED PLATFORM (ESP32 OR LINUX REQUIRED)"
#endif


// TLS/SSL with mbedtls
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/error.h>

#include <BriandIDFSocketClient.hxx>

using namespace std;

namespace Briand {

	/** This class is a simple socket client over TLS (uses mbedtls library) */
	class BriandIDFSocketTlsClient : public BriandIDFSocketClient {
		private:
		
		protected:

		/** Error buffer size */
		const unsigned char ERR_BUF_SIZE = 128;
		/** A random personalization string */
		unsigned char personalization_string[16];
		/** The tls socket */
		mbedtls_net_context tls_socket;
		/** Mbedtls entropy */
		mbedtls_entropy_context entropy;
		/** Mbedtls RNG */
		mbedtls_ctr_drbg_context ctr_drbg;
		/** Mbedtls SSL context */
		mbedtls_ssl_context ssl;
		/** Mbedtls SSL configuration */
		mbedtls_ssl_config conf;
		/** Mbedtls certificate chain */
		mbedtls_x509_crt cacert;
		/** Minimum RSA key size in bits */
		unsigned short min_rsa_key_size;
		/** Flag */
		bool caChainLoaded;
		/** Flag */
		bool caChainFailed;
		/** Flag */
		bool resourcesReady;

		/**
		 * Method set default socket options (timeout, keepalive...)
		*/
		virtual void SetDefaultSocketOptions();

		/** Free any resource (RNG, Entropy, context...) */
		virtual void ReleaseResources();

		/** Perpare needed resource (RNG, Entropy, context...) */
		virtual void SetupResources();

		public:

		/** Constructor: initializes every resource (RNG, Entropy, context...) */
		BriandIDFSocketTlsClient();

		/** Destructor: every resource will be released (RNG, Entropy, context...) */
		~BriandIDFSocketTlsClient();

		/** 
		 * Set an additional ID field, for debugging.
		 * @param id an ID that will be added for debugging
		*/
		virtual void SetID(const int& id);

		/**
		 * Set timeout in seconds (default unlimited=0) for connect and for read/write (default unlimited=0)
		 * @param connectTimeout_s Connection timeout in seconds (default unlimited = 0)
		 * @param ioTimeout_s Read/write timeout in seconds (default unlimited = 5)
		*/
		virtual void SetTimeout(const unsigned short& connectTimeout_s, const unsigned short& ioTimeout_s);

		/**
		 * Set the minimum allowed RSA Key size (default 2048).
		 * @param keySize Key size, in bits (default 2048)
		*/
		virtual void SetMinRsaKeySize(const unsigned short& keySize);

		/** 
		 * Method set the Server's PEM CA certificate for the connection. Can be multiple, one following the other.
		 * @param pemCAcertificate The CA certificate chain. (First CA, then Server peer). PEM format including BEGIN/END tags
		*/
		virtual void SetCACertificateChainPEM(const string& pemCAcertificate);

		/** 
		 * Method set the Server's DER CA certificate for the connection. Chain could be built with multiple method callings.
		 * @param derCAcertificate The CA certificate (ONLY ONE). (First CA, then Server peer). DER bytes.
		*/
		virtual void AddCACertificateToChainDER(const vector<unsigned char>& derCAcertificate);

		/**
		 * Opens a new TLS connection with the host. If no certificate is set, mode will be INSECURE
		 * @param host hostname (a DNS request will be made)
		 * @param port port to connect
		 * @return true if connected, false otherwise
		*/
		virtual bool Connect(const string& host, const short& port);

		/**
		 * Opens a new TLS connection with given address
		 * @param address Address info
		 * @param port port to connect
		 * @return true if connected, false otherwise
		*/
		virtual bool Connect(const struct addrinfo& address, const short& port);

		/**
		 * Closes the socket connection
		*/
		virtual void Disconnect();

		/**
		 * Sends data
		 * @param data Data to send
		 * @return true on success, false otherwise
		*/
		virtual bool WriteData(const unique_ptr<vector<unsigned char>>& data);

		/**
		 * Read data.
		 * @param oneChunk limit the read bytes to ReceivingBufferSize(), check HasMoreBytes() to iterate.
		 * @return Pointer to vector with read data, empty if fails.
		*/
		virtual unique_ptr<vector<unsigned char>> ReadData(bool oneChunk = false);

		/**
		 * Read data.
		 * @param stop The stop byte (ex. '\n')
		 * @param limit Limit to this amount of bytes. If stop char not found, return (with empty data)
		 * @param found Set to true if the stop byte is found, otherwise returns the data until limit reached
		 * @return Pointer to vector with read data (NOT including stop byte!), empty if error occoured or EOF
		*/
		virtual unique_ptr<vector<unsigned char>> ReadDataUntil(const unsigned char& stop, const size_t& limit, bool& found);

		/**
		 * Return number of available bytes that could be read
		 * @return number of waiting bytes
		*/
		virtual size_t AvailableBytes();

		/** Inherited from BriandESPHeapOptimize */
		virtual void PrintObjectSizeInfo();
		/** Inherited from BriandESPHeapOptimize */
		virtual size_t GetObjectSize();
	};
}