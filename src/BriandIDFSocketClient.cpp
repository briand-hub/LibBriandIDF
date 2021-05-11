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

#include "BriandIDFSocketClient.hxx"

#include <iostream>
#include <memory>

using namespace std;

namespace Briand {

	BriandIDFSocketClient::BriandIDFSocketClient() {
		this->CLIENT_NAME = string("BriandIDFSocketClient");
		this->CONNECTED = false;
		this->VERBOSE = false;
		this->CONNECT_TIMEOUT_S = 0;
		this->IO_TIMEOUT_S = 0;
		this->RECV_BUF_SIZE = 64;
		this->_socket = -1;
	}
	
	BriandIDFSocketClient::~BriandIDFSocketClient() {
		if (this->CONNECTED) this->Disconnect();
	}

	void BriandIDFSocketClient::SetDefaultSocketOptions() {
		if (this->CONNECTED) {
			// Set read and write timeout if requested
			if (this->IO_TIMEOUT_S > 0) {
				struct timeval receiving_timeout;
				receiving_timeout.tv_sec = this->IO_TIMEOUT_S;
				receiving_timeout.tv_usec = 0;
				
				// Set timeout for socket read
				if (setsockopt(this->_socket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
					if (this->VERBOSE) printf("[%s] Error on setting socket option read timeout.\n", this->CLIENT_NAME.c_str());
				}

				// Set timeout for socket write
				if (setsockopt(this->_socket, SOL_SOCKET, SO_SNDTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
					if (this->VERBOSE) printf("[%s] Error on setting socket option write timeout.\n", this->CLIENT_NAME.c_str());
				}
			}

			// Set always common good options
			int enableFlag = 1;

			// Enable Tcp no delay
			if (setsockopt(this->_socket, IPPROTO_TCP, TCP_NODELAY, &enableFlag, sizeof(enableFlag)) < 0) {
				if (this->VERBOSE) printf("[%s] Error on setting socket option tcp no delay.\n", this->CLIENT_NAME.c_str());
			}

			// Enable Keep-Alive
			if (setsockopt(this->_socket, SOL_SOCKET, SO_KEEPALIVE, &enableFlag, sizeof(enableFlag)) < 0) {
				if (this->VERBOSE) printf("[%s] Error on setting socket option keep-alive.\n", this->CLIENT_NAME.c_str());
			}
		}
	}

	void BriandIDFSocketClient::SetVerbose(const bool& verbose) {
		this->VERBOSE = verbose;
	}
	
	void BriandIDFSocketClient::SetID(const int& id) {
		this->CLIENT_NAME = "BriandIDFSocketClient#" + std::to_string(id);
	}

	void BriandIDFSocketClient::SetTimeout(const unsigned short& connectTimeout_s, const unsigned short& ioTimeout_s) {
		this->CONNECT_TIMEOUT_S = connectTimeout_s;
		this->IO_TIMEOUT_S = ioTimeout_s;
	}

	void BriandIDFSocketClient::SetReceivingBufferSize(const unsigned short& size) {
		this->RECV_BUF_SIZE = size;
	}

	bool BriandIDFSocketClient::Connect(const struct addrinfo& address, const short& port) {
		// If previous connection is in progress, close it.
		if (this->CONNECTED) {
			this->Disconnect();
		}

		this->_socket = socket(address.ai_family, address.ai_socktype, 0);

		if (this->_socket < 0) {
			if (this->VERBOSE) printf("[%s] Failed to allocate socket.\n", this->CLIENT_NAME.c_str());
			return false;
		}
		
		if (this->VERBOSE) printf("[%s] Socket allocated.\n", this->CLIENT_NAME.c_str());

		if(connect(this->_socket, address.ai_addr, address.ai_addrlen) != 0) {
			if (this->VERBOSE) printf("[%s] Socket connection failed, errno = %d\n", this->CLIENT_NAME.c_str(), errno);
			close(this->_socket);
			return false;
		}

		if (this->VERBOSE) printf("[%s] Socket connected.\n", this->CLIENT_NAME.c_str());

		// Now connected!
		this->CONNECTED = true;

		// Set socket options
		this->SetDefaultSocketOptions();

		return true;
	}

	bool BriandIDFSocketClient::Connect(const string& host, const short& port) {
		// If previous connection is in progress, close it.
		if (this->CONNECTED) {
			this->Disconnect();
		}

		// Make a DNS request, then call the main Connect.

		// TODO : IPv6 support
		
		struct addrinfo hints {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		struct addrinfo* res;

		int err = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);

		if(err != 0 || res == NULL) {
			if (this->VERBOSE) printf("[%s] DNS lookup failed err=%d res=%p\n", this->CLIENT_NAME.c_str(), err, res);
			if (res != NULL) freeaddrinfo(res);
			return false;
		}

		bool connected = this->Connect(*res, port);

		// Free resources
		freeaddrinfo(res);

		return connected;
	}

	bool BriandIDFSocketClient::IsConnected() {
		return this->CONNECTED;
	}

	void BriandIDFSocketClient::Disconnect() {
		if (this->CONNECTED) {
			close(this->_socket);
			this->CONNECTED = false;
			if (this->VERBOSE) printf("[%s] Disconnected.\n", this->CLIENT_NAME.c_str());
		}
	}

	bool BriandIDFSocketClient::WriteData(const unique_ptr<vector<unsigned char>>& data) {
		if (!this->CONNECTED) return false;

		if (write(this->_socket, data->data(), data->size()) < 0) {
			if (this->VERBOSE) printf("[%s] Error on write.\n", this->CLIENT_NAME.c_str());
			return false;
		}
		
		if (this->VERBOSE) printf("[%s] %d bytes written.\n", this->CLIENT_NAME.c_str(), data->size());

		return true;
	}

	unique_ptr<vector<unsigned char>> BriandIDFSocketClient::ReadData(bool oneChunk /* = false*/) {
		auto data = make_unique<vector<unsigned char>>();

		if (!this->CONNECTED) return std::move(data);

		// Read until bytes received or just one chunk requested
		int receivedBytes;
		do {
			auto recvBuffer = make_unique<unsigned char[]>(this->RECV_BUF_SIZE);
			receivedBytes = read(this->_socket, recvBuffer.get(), RECV_BUF_SIZE);
			if (receivedBytes > 0) {
				data->insert(data->end(), recvBuffer.get(), recvBuffer.get() + receivedBytes);
			}
		} while(receivedBytes > 0 && !oneChunk);

		if (this->VERBOSE) printf("[%s] Received %d bytes. %s\n", this->CLIENT_NAME.c_str(), data->size(), ((this->AvailableBytes() > 0) ? "More bytes are available." : "No more bytes available."));

		return std::move(data);
	}

	int BriandIDFSocketClient::AvailableBytes() {
		int bytes_avail = 0;

		if (this->CONNECTED)
			//ioctl(this->_socket, FIONREAD, &bytes_avail);
			lwip_ioctl(this->_socket, FIONREAD, &bytes_avail);

		return bytes_avail;
	}
	
}