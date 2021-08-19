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

using namespace std;

namespace Briand {

	/** This class is a simple socket client (not SSL) */
	class BriandIDFSocketClient : public BriandESPHeapOptimize {
		private:

		protected:
		
		/* Client name for debugging */
		string CLIENT_NAME;
		/* Flag */
		bool VERBOSE;
		/* Flag */
		bool CONNECTED;
		/* Connection timeout */
		unsigned short CONNECT_TIMEOUT_S;
		/* Read/write timeout */
		unsigned short IO_TIMEOUT_S;
		/* Internal buffer size for read operation */
		unsigned short RECV_BUF_SIZE;
		/* Internal socket */
		int _socket;
		/* Poll/Select operations timeout in seconds (default: 10 seconds) */
		const unsigned char poll_default_timeout_s = 10;

		/**
		 * Method set default socket options (timeout, keepalive...)
		*/
		virtual void SetDefaultSocketOptions();

		public:

		/** Constructor, initialize resources */
		BriandIDFSocketClient();

		/** Destructor, if connected disconnects and releases all resources */
		~BriandIDFSocketClient();

		/**
		 * Set verbosity (disabled by default)
		 * @param verbose if true prints to stdout
		*/
		virtual void SetVerbose(const bool& verbose);

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
		 * Set receiving buffer chunk size, in bytes. Default 64.
		 * @param size Buffer chunk, in bytes (default 64)
		*/
		virtual void SetReceivingBufferSize(const unsigned short& size);

		/**
		 * Opens a new clear connection with the host
		 * @param host hostname (a DNS request will be made)
		 * @param port port to connect
		 * @return true if connected, false otherwise
		*/
		virtual bool Connect(const string& host, const short& port);

		/**
		 * Opens a new clear connection with given address
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
		 * Returns connection status
		 * @return true if connected, false otherwise
		*/
		virtual bool IsConnected();

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
		 * Return number of available bytes that could be read. 
		 * @return number of waiting bytes
		*/
		virtual size_t AvailableBytes();

		/**
		 * Return the internal socket file descriptor
		 * @return internal socket fd
		*/
		virtual int GetSocketDescriptor();

		/** Inherited from BriandESPHeapOptimize */
		virtual void PrintObjectSizeInfo();
		/** Inherited from BriandESPHeapOptimize */
		virtual size_t GetObjectSize();

	};
}