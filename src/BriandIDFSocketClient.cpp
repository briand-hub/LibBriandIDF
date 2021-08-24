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
		this->RECV_BUF_SIZE = 512;
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
			shutdown(this->_socket, SHUT_RDWR);
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
			shutdown(this->_socket, SHUT_RDWR);
			close(this->_socket);
			this->CONNECTED = false;
			if (this->VERBOSE) printf("[%s] Disconnected.\n", this->CLIENT_NAME.c_str());
		}
	}

	bool BriandIDFSocketClient::WriteData(const unique_ptr<vector<unsigned char>>& data) {
		if (!this->CONNECTED) return false;

		if (data == nullptr || data->size() == 0) {
			if (this->VERBOSE) printf("[%s] WriteData: no bytes! (nullptr or zero size!).\n", this->CLIENT_NAME.c_str());
			return false;
		}

		if (send(this->_socket, data->data(), data->size(), 0) < 0) {
			if (this->VERBOSE) printf("[%s] Error on send().\n", this->CLIENT_NAME.c_str());
			return false;
		}
		
		if (this->VERBOSE) printf("[%s] %d bytes written.\n", this->CLIENT_NAME.c_str(), data->size());

		return true;
	}

	unique_ptr<vector<unsigned char>> BriandIDFSocketClient::ReadData(bool oneChunk /* = false*/) {
		auto data = make_unique<vector<unsigned char>>();

		if (!this->CONNECTED) return std::move(data);

		// Error management
		//int ret;

		// Poll the connection for reading
		// if (this->VERBOSE) printf("[%s] Polling for read\n", this->CLIENT_NAME.c_str()); 
		// gives linker undefined reference error
		// ret = mbedtls_net_poll(&this->tls_socket, MBEDTLS_NET_POLL_READ, this->IO_TIMEOUT_S*1000);
		// polling not solving problem for long delays
		// pollfd fds;
		// memset(&fds, 0, sizeof(fds));
		// fds.fd = this->_socket;
		// fds.events = POLLRDNORM;
		// ret = poll(&fds, 1, this->poll_timeout_s*1000);
		// if (this->VERBOSE) printf("[%s] Poll result: %d flags: %d Bytes avail: %d\n", this->CLIENT_NAME.c_str(), ret, fds.revents, this->AvailableBytes());

		// Read until bytes received or just one chunk requested
		int receivedBytes;
		do {
			// The following seems to resolve the long delay. 
			// The blocking request for always RECV_BUF_SIZE seems to keep socket blocked until exactly recv_buf_size at most is read.

			size_t remainingBytes = this->AvailableBytes();
			int READ_SIZE = this->RECV_BUF_SIZE;
			if (remainingBytes > 0 && remainingBytes < READ_SIZE)
				READ_SIZE = remainingBytes;

			auto recvBuffer = make_unique<unsigned char[]>(READ_SIZE);

			// Before blocking socket, perform a select(), if timeout is not specified, a default 10 seconds will be used.
			fd_set filter;
			FD_ZERO(&filter);
			FD_SET(this->_socket, &filter);
			struct timeval timeout;
			bzero(&timeout, sizeof(timeout));
			timeout.tv_usec = 0;
			timeout.tv_sec = ( this->IO_TIMEOUT_S > 0 ? this->IO_TIMEOUT_S : this->poll_default_timeout_s);
			int selectResult = select(this->_socket+1, &filter, NULL, NULL, &timeout);

			if (selectResult < 0) {
				// An error occoured, select() failed.
				if (this->VERBOSE) printf("[%s] select() failed.\n", this->CLIENT_NAME.c_str());
				break;
			}
			else if (selectResult == 0 && !FD_ISSET(this->_socket, &filter)) {
				// An timeout occoured
				if (this->VERBOSE) printf("[%s] select() timed out.\n", this->CLIENT_NAME.c_str());
				break;
			}
			else if (!FD_ISSET(this->_socket, &filter)) {
				// No socket on the results!
				if (this->VERBOSE) printf("[%s] select() error: no timeout but socket not ready.\n", this->CLIENT_NAME.c_str());
				break;
			}

			if (this->VERBOSE) printf("[%s] select() succeded.\n", this->CLIENT_NAME.c_str());

			receivedBytes = recv(this->_socket, recvBuffer.get(), READ_SIZE, 0);

			if (receivedBytes == 0) {
				// If select() succeded but zero bytes are received, then exit / peer disconnected.
				if (this->VERBOSE) printf("[%s] select() succeded, but zero bytes received. Peer disconnected.\n", this->CLIENT_NAME.c_str());
				break;
			}

			if (receivedBytes > 0) {
				data->insert(data->end(), recvBuffer.get(), recvBuffer.get() + receivedBytes);
			}

			// Check if the remainingBytes were less  than or equal the receiving buffer size. If so, we finished.
			// Removed: truncated response risk!
			// if (remainingBytes > 0 && remainingBytes <= this->RECV_BUF_SIZE)
			// 	break;

		} while(receivedBytes > 0 && !oneChunk);

		if (this->VERBOSE) printf("[%s] Received %d bytes. %s\n", this->CLIENT_NAME.c_str(), data->size(), ((this->AvailableBytes() > 0) ? "More bytes are available." : "No more bytes available."));

		return std::move(data);
	}

	unique_ptr<vector<unsigned char>> BriandIDFSocketClient::ReadDataUntil(const unsigned char& stop, const size_t& limit, bool& found) {
		auto data = make_unique<vector<unsigned char>>();

		if (!this->CONNECTED) return std::move(data);

		// Read until bytes received or stop char is found, 1 byte per cycle.
		int receivedBytes;
		found = false;

		do {
			//size_t remainingBytes = this->AvailableBytes();

			// Before blocking socket, perform a select(), if timeout is not specified, a default 10 seconds will be used.
			fd_set filter;
			FD_ZERO(&filter);
			FD_SET(this->_socket, &filter);
			struct timeval timeout;
			bzero(&timeout, sizeof(timeout));
			timeout.tv_usec = 0;
			timeout.tv_sec = ( this->IO_TIMEOUT_S > 0 ? this->IO_TIMEOUT_S : this->poll_default_timeout_s);
			int selectResult = select(this->_socket+1, &filter, NULL, NULL, &timeout);

			if (selectResult < 0) {
				// An error occoured, select() failed.
				if (this->VERBOSE) printf("[%s] select() failed.\n", this->CLIENT_NAME.c_str());
				break;
			}
			else if (selectResult == 0 && !FD_ISSET(this->_socket, &filter)) {
				// An timeout occoured
				if (this->VERBOSE) printf("[%s] select() timed out.\n", this->CLIENT_NAME.c_str());
				break;
			}
			else if (!FD_ISSET(this->_socket, &filter)) {
				// No socket on the results!
				if (this->VERBOSE) printf("[%s] select() error: no timeout but socket not ready.\n", this->CLIENT_NAME.c_str());
				break;
			}

			if (this->VERBOSE) printf("[%s] select() succeded.\n", this->CLIENT_NAME.c_str());

			unsigned char buffer = 0x00;
			receivedBytes = recv(this->_socket, &buffer, 1, 0);

			if (receivedBytes == 0) {
				// If select() succeded but zero bytes are received, then exit / peer disconnected.
				if (this->VERBOSE) printf("[%s] select() succeded, but zero bytes received. Peer disconnected.\n", this->CLIENT_NAME.c_str());
				break;
			}

			// If stop char found, stop.
			if (buffer == stop) {
				found = true;
			}

			if (receivedBytes > 0) {
				data->push_back(buffer);
			}
			
		} while(receivedBytes > 0 && !found && data->size() < limit);

		return std::move(data);
	}

	size_t BriandIDFSocketClient::AvailableBytes() {
		size_t bytes_avail = 0;

		if (this->CONNECTED) {
			// Call a read without buffer (does not download data)
			char temp;
			recv(this->_socket, &temp, 0, 0);
			ioctl(this->_socket, FIONREAD, &bytes_avail);
		}
			
		return bytes_avail;
	}

	int BriandIDFSocketClient::GetSocketDescriptor() {
		return this->_socket;
	}
	
	size_t BriandIDFSocketClient::GetObjectSize() {
		size_t oSize = 0;

		oSize += sizeof(*this);
		oSize += sizeof(this->CLIENT_NAME) + sizeof(char)*this->CLIENT_NAME.size();

		return oSize;
	}

	void BriandIDFSocketClient::PrintObjectSizeInfo() {
		printf("sizeof(*this) = %zu\n", sizeof(*this));
		printf("sizeof(this->CLIENT_NAME) + sizeof(char)*this->CLIENT_NAME.size() = %zu\n", sizeof(this->CLIENT_NAME) + sizeof(char)*this->CLIENT_NAME.size());

		printf("TOTAL = %zu\n", this->GetObjectSize());
	}

}