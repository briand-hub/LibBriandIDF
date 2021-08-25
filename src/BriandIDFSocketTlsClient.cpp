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

#include "BriandIDFSocketTlsClient.hxx"

#include <iostream>
#include <memory>

using namespace std;

namespace Briand {

	BriandIDFSocketTlsClient::BriandIDFSocketTlsClient() : BriandIDFSocketClient() {
		this->CLIENT_NAME = string("BriandIDFSocketTlsClient");
		this->caChainLoaded = false;
		this->caChainFailed = true;
		this->min_rsa_key_size = 2048;

		// Setup resources
		this->SetupResources();
	}

	BriandIDFSocketTlsClient::~BriandIDFSocketTlsClient() {
		this->ReleaseResources();
	}

	void BriandIDFSocketTlsClient::SetDefaultSocketOptions() {
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

	void BriandIDFSocketTlsClient::SetID(const int& id) {
		this->CLIENT_NAME = "BriandIDFSocketTlsClient#" + std::to_string(id);
	}

	void BriandIDFSocketTlsClient::SetupResources() {
		this->resourcesReady = false;

		// Error checking
		int ret;

		// Initialize resources needed
		mbedtls_net_init( &this->tls_socket );
		mbedtls_ssl_init( &this->ssl );
		mbedtls_ssl_config_init( &this->conf );
		mbedtls_x509_crt_init( &this->cacert );
		mbedtls_ctr_drbg_init( &this->ctr_drbg );

		// Set a random personalization string
		for (unsigned char i=0; i<16; i++)
			personalization_string[i] = static_cast<unsigned char>( esp_random() % 0x100 );

		if (this->VERBOSE) printf("[%s] Initializaing RNG.\n", this->CLIENT_NAME.c_str());

		// Initialize the RNG
		mbedtls_entropy_init( &entropy );
		ret = mbedtls_ctr_drbg_seed( &this->ctr_drbg, mbedtls_entropy_func, &this->entropy, const_cast<const unsigned char*>(this->personalization_string), 16);
		if( ret != 0 )
		{
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Error, failed RNG init: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			this->ReleaseResources();
			return;
		}

		// Certificates will be set by the other methods.

		this->resourcesReady = true;
	}

	void BriandIDFSocketTlsClient::ReleaseResources() {
		if (this->CONNECTED) this->Disconnect();
		this->caChainLoaded = false;
		this->caChainFailed = true;
		if (this->resourcesReady) {
			this->resourcesReady = false;
			mbedtls_net_free(&this->tls_socket);
			mbedtls_ssl_free(&this->ssl);
			mbedtls_ssl_config_free(&this->conf);
			mbedtls_x509_crt_free(&this->cacert);
			mbedtls_ctr_drbg_free(&this->ctr_drbg);
			mbedtls_entropy_free(&this->entropy);
		}	
		this->_socket = -1;
	}

	void BriandIDFSocketTlsClient::SetTimeout(const unsigned short& connectTimeout_s, const unsigned short& ioTimeout_s) {
		this->CONNECT_TIMEOUT_S = connectTimeout_s;
		this->IO_TIMEOUT_S = ioTimeout_s;

		// Better implementation with SetDefaultSocketOptions()
		// if (this->resourcesReady && this-IO_TIMEOUT_S > 0) {
		// 	mbedtls_ssl_conf_read_timeout(&this->conf, this->IO_TIMEOUT_S*1000);
		// }
	}

	void BriandIDFSocketTlsClient::SetMinRsaKeySize(const unsigned short& keySize) {
		this->min_rsa_key_size = keySize;
	}

	void BriandIDFSocketTlsClient::SetCACertificateChainPEM(const string& pemCAcertificate) {
		if (!this->resourcesReady) SetupResources();
		this->caChainLoaded = true;

		// PEM: must be null terminated, could be multiple at once
		// The size passed must include the null-terminating char
		unsigned long int CERT_SIZE = pemCAcertificate.length() + 1;
		int ret = mbedtls_x509_crt_parse(&this->cacert, reinterpret_cast<const unsigned char*>(pemCAcertificate.c_str()), CERT_SIZE);
		if( ret < 0 )
		{
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Warning! Failed to parse CA chain PEM certificate: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			//if (this->VERBOSE) printf("[%s] Warning! INSECURE mode (no verify) will be used.\n", this->CLIENT_NAME.c_str());
			this->caChainFailed = true;
		}
		else {
			this->caChainFailed = false;
		}
	}

	void BriandIDFSocketTlsClient::AddCACertificateToChainDER(const vector<unsigned char>& derCAcertificate) {
		if (!this->resourcesReady) SetupResources();
		this->caChainLoaded = true;

		// DER: must be JUST ONE!

		int ret = mbedtls_x509_crt_parse(&this->cacert, reinterpret_cast<const unsigned char*>(derCAcertificate.data()), derCAcertificate.size());
		if( ret < 0 )
		{
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Warning! Failed to parse CA chain DER certificate: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			//if (this->VERBOSE) printf("[%s] Warning! INSECURE mode (no verify) will be used.\n", this->CLIENT_NAME.c_str());
			this->caChainFailed = true;
		}
		else {
			this->caChainFailed = false;
		}
	}

	bool BriandIDFSocketTlsClient::Connect(const struct addrinfo& address, const short& port) {
		string hostIp("");

		// Using mbedtls_net_connect ip or address does not matter. So translate the struct to IP and go.
		unique_ptr<char[]> ipBuf = nullptr;
		switch (address.ai_addr->sa_family) {
			case AF_INET:
				ipBuf = make_unique<char[]>(INET_ADDRSTRLEN);
				inet_ntop(AF_INET, &(((struct sockaddr_in *)address.ai_addr)->sin_addr), ipBuf.get(), INET_ADDRSTRLEN);
				hostIp = string(ipBuf.get());
				ipBuf.reset();
				break;
			case AF_INET6:
				ipBuf = make_unique<char[]>(INET6_ADDRSTRLEN);
				inet_ntop(AF_INET, &(((struct sockaddr_in *)address.ai_addr)->sin_addr), ipBuf.get(), INET6_ADDRSTRLEN);
				hostIp = string(ipBuf.get());
				ipBuf.reset();
				break;
			default:
				break;
		}

		if (hostIp.length() == 0) {
			if (this->VERBOSE) printf("[%s] Failed to convert IP to string (invalid INET?).\n", this->CLIENT_NAME.c_str());
			if (this->resourcesReady) this->ReleaseResources();
			return false;
		}

		return this->Connect(hostIp, port);
	}

	bool BriandIDFSocketTlsClient::Connect(const string& host, const short& port) {
		// If previous connection is in progress, close it.
		if (this->CONNECTED) {
			this->Disconnect();
		}

		// If CA chain loaded but failed, return false.
		if (this->caChainLoaded && this->caChainFailed) {
			if (this->VERBOSE) printf("[%s] SSL certificate chain loaded but FAILED.\n", this->CLIENT_NAME.c_str());
			this->ReleaseResources();
			return false;
		}

		if (!this->resourcesReady) SetupResources();

		// Error management
		int ret;

		if (this->VERBOSE) printf("[%s] Opening connection.\n", this->CLIENT_NAME.c_str());

		// Open socket connection
		ret = mbedtls_net_connect(&this->tls_socket, host.c_str(), std::to_string(port).c_str(), MBEDTLS_NET_PROTO_TCP); 
		if (ret != 0) {
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Failed to allocate socket: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			this->ReleaseResources();
			return false;
		}

		if (this->VERBOSE) printf("[%s] Socket ready, configuring SSL.\n", this->CLIENT_NAME.c_str());

		// Default configuration
		ret = mbedtls_ssl_config_defaults(&this->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
		if (ret != 0) {
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Failed to setup SSL socket: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			this->ReleaseResources();
			return false;
		}

		// Handshake timeout
		// Not working, gives linker error!
		// mbedtls_ssl_conf_handshake_timeout(&this->conf, 1000, this->CONNECT_TIMEOUT_S*1000);

		// Set the security mode
		if (this->caChainLoaded && !this->caChainFailed) {
			mbedtls_ssl_conf_authmode(&this->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
			mbedtls_ssl_conf_ca_chain(&this->conf, &this->cacert, NULL);
			mbedtls_ssl_conf_rng(&this->conf, mbedtls_ctr_drbg_random, &this->ctr_drbg );
			if (this->VERBOSE) printf("[%s] SSL certificate chain loaded.\n", this->CLIENT_NAME.c_str());
		}
		else {
			mbedtls_ssl_conf_authmode(&this->conf, MBEDTLS_SSL_VERIFY_NONE);
			//mbedtls_ssl_conf_ca_chain(&this->conf, &this->cacert, NULL);
			mbedtls_ssl_conf_rng(&this->conf, mbedtls_ctr_drbg_random, &this->ctr_drbg );
			if (this->VERBOSE) printf("[%s] SSL with INSECURE mode set.\n", this->CLIENT_NAME.c_str());
		}

		// Setup 
		ret = mbedtls_ssl_setup(&this->ssl, &this->conf);
		if (ret != 0) {
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Failed to setup SSL configuration: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			this->ReleaseResources();
			return false;
		}
		ret = mbedtls_ssl_set_hostname(&this->ssl, host.c_str());
		if (ret != 0) {
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Failed to setup SSL hostname: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
			errBuf.reset();
			this->ReleaseResources();
			return false;
		}

		if (this->VERBOSE) printf("[%s] SSL setup done.\n", this->CLIENT_NAME.c_str());

		// Setup the functions that will be used for data read/write. 
		// Added also timeout with mbedtls_net_recv_timeout
		if (this->IO_TIMEOUT_S > 0)
			mbedtls_ssl_set_bio(&this->ssl, &this->tls_socket, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
		else 
			mbedtls_ssl_set_bio(&this->ssl, &this->tls_socket, mbedtls_net_send, mbedtls_net_recv, NULL);

		// Set timeout (works only with mbedtls_net_recv_timeout on BIO setup!)
		if (this->IO_TIMEOUT_S > 0)
			mbedtls_ssl_conf_read_timeout(&this->conf, this->IO_TIMEOUT_S*1000);

		if (this->VERBOSE) printf("[%s] Performing handshake.\n", this->CLIENT_NAME.c_str());

		// Handshake
		ret = mbedtls_ssl_handshake(&this->ssl);
		//if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
		if (ret != 0) {
			auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
			mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
			if (this->VERBOSE) printf("[%s] Failed SSL handshake, returned %d: %s\n", this->CLIENT_NAME.c_str(), ret, errBuf.get());
			errBuf.reset();
			this->ReleaseResources();
			return false;
		}

		if (this->VERBOSE) printf("[%s] SSL handshake done.\n", this->CLIENT_NAME.c_str());
		
		// Verify certificates, if loaded
		if (this->caChainLoaded && !this->caChainFailed) {
			unsigned long int flags = mbedtls_ssl_get_verify_result(&this->ssl);
			if (flags != 0) {
				auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
				mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
				mbedtls_x509_crt_verify_info(errBuf.get(), this->ERR_BUF_SIZE - 1, "", flags);
				if (this->VERBOSE) printf("[%s] Certificate validation failed: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
				errBuf.reset();
				this->ReleaseResources();
				return false;
			}
			if (this->VERBOSE) printf("[%s] Certificate validation success.\n", this->CLIENT_NAME.c_str());
		}
		else {
			if (this->VERBOSE) printf("[%s] Certificate are not validated (INSECURE MODE).\n", this->CLIENT_NAME.c_str());
		}

		if (this->VERBOSE) printf("[%s] SSL connection ready.\n", this->CLIENT_NAME.c_str());

		this->CONNECTED = true;

		// Save underlying socket if needed
		this->_socket = this->tls_socket.fd;

		// Now connected!
		this->CONNECTED = true;

		// Set socket options
		this->SetDefaultSocketOptions();

		return true;
	}

	void BriandIDFSocketTlsClient::Disconnect() {
		if (this->CONNECTED) {
			this->CONNECTED = false;
			mbedtls_ssl_close_notify(&this->ssl);
			this->_socket = -1;
			if (this->VERBOSE) printf("[%s] Disconnected.\n", this->CLIENT_NAME.c_str());
		}
		// in each case...
		this->ReleaseResources();
	}

	bool BriandIDFSocketTlsClient::WriteData(const unique_ptr<vector<unsigned char>>& data) {
		if (!this->CONNECTED) return false;

		// Error management
		int ret;

		if (data == nullptr || data->size() == 0) {
			if (this->VERBOSE) printf("[%s] WriteData: no bytes! (nullptr or zero size!).\n", this->CLIENT_NAME.c_str());
			return false;
		}

		// Poll the connection for writing (NOT NECESSARY)
		// if (this->VERBOSE) printf("[%s] Polling for write\n", this->CLIENT_NAME.c_str()); 
		// Linker error: undefined reference to `mbedtls_net_poll' see ReadData() for details/implementation
		// ret = mbedtls_net_poll(&this->tls_socket, MBEDTLS_NET_POLL_WRITE, this->IO_TIMEOUT_S);
		// if (this->VERBOSE) printf("[%s] Poll result: %d\n", this->CLIENT_NAME.c_str(), ret);

		do {
			ret = mbedtls_ssl_write(&this->ssl, data->data(), data->size());

			if(ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
				auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
				mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
				if (this->VERBOSE) printf("[%s] Failed to write: %d %x %s\n", this->CLIENT_NAME.c_str(), ret, ret, errBuf.get());
				errBuf.reset();
				// Connection must be closed!
				this->Disconnect();
				return false;
			}
		}
		while (ret <= 0);
			
		if (this->VERBOSE) printf("[%s] %d bytes written.\n", this->CLIENT_NAME.c_str(), ret);

		return true;
	}

	unique_ptr<vector<unsigned char>> BriandIDFSocketTlsClient::ReadData(bool oneChunk /* = false*/) {
		auto data = make_unique<vector<unsigned char>>();

		if (!this->CONNECTED) return std::move(data);

		// Error management
		int ret;

		// Poll the connection for reading - NOT SOLVING PROBLEM
		//if (this->VERBOSE) printf("[%s] Polling for read\n", this->CLIENT_NAME.c_str()); 
		// gives linker undefined reference error
		//ret = mbedtls_net_poll(&this->tls_socket, MBEDTLS_NET_POLL_READ, this->IO_TIMEOUT_S*1000);

		// polling not solving problem for long delays
		// pollfd fds;
		// memset(&fds, 0, sizeof(fds));
		// fds.fd = this->_socket;
		// fds.events = POLLRDNORM;
		// poll(&fds, 1, this->poll_timeout_s*1000);
		// if (this->VERBOSE) printf("[%s] Poll result: %d flags: %d Bytes avail: %d\n", this->CLIENT_NAME.c_str(), ret, fds.revents, this->AvailableBytes());

		// Read until bytes received or jsut one chunk requested
		do {
			// The following seems to resolve the long delay. 
			// The blocking request for always RECV_BUF_SIZE seems to keep socket blocked until exactly recv_buf_size at most is read.
			size_t remainingBytes = this->AvailableBytes();
			int READ_SIZE = this->RECV_BUF_SIZE;
			if (remainingBytes > 0 && remainingBytes < READ_SIZE)
				READ_SIZE = remainingBytes;

			auto recvBuffer = make_unique<unsigned char[]>(READ_SIZE);
			ret = mbedtls_ssl_read(&this->ssl, recvBuffer.get(), READ_SIZE);

			// DEBUG if (this->VERBOSE) printf("[%s] Called ret = %d size to read=%d\n", this->CLIENT_NAME.c_str(), ret, READ_SIZE); 
			
			if(ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
				// Wait
				continue;
			}
			else if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == MBEDTLS_ERR_SSL_WANT_READ) {
				// Finish because server wants to close or pass to the read (has finished writing us)
				break;
			}
			else if (ret < 0) {
				// Error
				auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
				mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
				if (this->VERBOSE) printf("[%s] Failed to read: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
				errBuf.reset();
				// Connection must be closed!
				this->Disconnect();
				return std::move(data);
			}

			// if ret is zero is EOF
			// if > 0 then bytes!
			if (ret > 0) data->insert(data->end(), recvBuffer.get(), recvBuffer.get() + ret);

			// Check if the remainingBytes were less  than or equal the receiving buffer size. If so, we finished.
			// The condition is good there because MbedTLS has internal buffer!
			if (remainingBytes > 0 && remainingBytes <= this->RECV_BUF_SIZE)
				break;

		} while(ret != 0 && !oneChunk);

		if (this->VERBOSE) printf("[%s] Received %d bytes. %s\n", this->CLIENT_NAME.c_str(), data->size(), ((this->AvailableBytes() > 0) ? "More bytes are available." : "No more bytes available."));

		return std::move(data);
	}

	unique_ptr<vector<unsigned char>> BriandIDFSocketTlsClient::ReadDataUntil(const unsigned char& stop, const size_t& limit, bool& found) {
		auto data = make_unique<vector<unsigned char>>();

		if (!this->CONNECTED) return std::move(data);

		// Read until bytes received or stop char is found, 1 byte per cycle.
		int ret;
		found = false;

		do {
			size_t remainingBytes = this->AvailableBytes();

			if (remainingBytes <= 0) {
				// Error, stop char not found before EOF. 
				return std::move(data);
			}

			unsigned char buffer = 0x00;
			ret = mbedtls_ssl_read(&this->ssl, &buffer, 1);

			if(ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
				// Wait
				continue;
			}
			else if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == MBEDTLS_ERR_SSL_WANT_READ) {
				// Finish because server wants to close or pass to the read (has finished writing us)
				break;
			}
			else if (ret < 0) {
				// Error
				auto errBuf = make_unique<char[]>(this->ERR_BUF_SIZE);
				mbedtls_strerror(ret, errBuf.get(), this->ERR_BUF_SIZE - 1);
				if (this->VERBOSE) printf("[%s] Failed to read: %s\n", this->CLIENT_NAME.c_str(), errBuf.get());
				errBuf.reset();
				// Connection must be closed!
				this->Disconnect();
				return std::move(data);
			}

			// If stop char found, stop.
			if (buffer == stop) {
				found = true;
			}

			if (ret > 0) {
				data->push_back(buffer);
			}

		} while(ret > 0 && !found && data->size() < limit);

		return std::move(data);
	}

	size_t BriandIDFSocketTlsClient::AvailableBytes() {
		size_t bytes_avail = 0;

		if (this->CONNECTED) {
			/* select() here with mbedtls is a BAD idea: results show a really long delay
			// Perform a select() 
			fd_set filter;
			FD_ZERO(&filter);
			FD_SET(this->_socket, &filter);
			struct timeval timeout;
			bzero(&timeout, sizeof(timeout));
			timeout.tv_usec = 0;
			timeout.tv_sec = ( this->IO_TIMEOUT_S > 0 ? this->IO_TIMEOUT_S : this->poll_default_timeout_s);
			int selectResult = select(this->_socket+1, &filter, NULL, NULL, &timeout);

			if (selectResult < 0) {
				// Error on select
				if (this->VERBOSE)  printf("[%s] select() failed.\n", this->CLIENT_NAME.c_str());
				return bytes_avail;
			}
			*/

			// Call a zero-size read
			mbedtls_ssl_read(&this->ssl, NULL, 0);
			bytes_avail = mbedtls_ssl_get_bytes_avail(&this->ssl);
		}
		
		return bytes_avail;
	}

	size_t BriandIDFSocketTlsClient::GetObjectSize() {
		size_t oSize = 0;

		oSize += sizeof(*this);
		oSize += sizeof(this->CLIENT_NAME) + sizeof(char)*this->CLIENT_NAME.size();
		oSize += sizeof(this->personalization_string) + sizeof(unsigned char)*16;

		return oSize;
	}

	void BriandIDFSocketTlsClient::PrintObjectSizeInfo() {
		printf("sizeof(*this) = %zu\n", sizeof(*this));
		printf("sizeof(this->CLIENT_NAME) + sizeof(char)*this->CLIENT_NAME.size() = %zu\n", sizeof(this->CLIENT_NAME) + sizeof(char)*this->CLIENT_NAME.size());
		printf("sizeof(this->personalization_string) + sizeof(unsigned char)*16 = %zu\n", sizeof(this->personalization_string) + sizeof(unsigned char)*16);

		printf("TOTAL = %zu\n", this->GetObjectSize());
	}

}