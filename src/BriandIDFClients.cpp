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

#include "BriandIDFClients.hxx"

#include <iostream>
#include <memory>

using namespace std;

namespace Briand {

	/*
	*
	*
	*	BriandIDFSocketClient
	*
	*
	*/

	BriandIDFSocketClient::BriandIDFSocketClient() {
		this->CLIENT_NAME = string("BriandIDFSocketClient");
		this->CONNECTED = false;
		this->VERBOSE = false;
		this->CONNECT_TIMEOUT_S = 30;
		this->IO_TIMEOUT_S = 5;
		this->RECV_BUF_SIZE = 64;
		this->_socket = -1;
	}

	BriandIDFSocketClient::BriandIDFSocketClient(const int& id) {
		BriandIDFSocketClient();
		this->CLIENT_NAME.append(" #") + std::to_string(id);
	}

	BriandIDFSocketClient::~BriandIDFSocketClient() {
		if (this->CONNECTED) this->Disconnect();
	}

	void BriandIDFSocketClient::SetVerbose(const bool& verbose) {
		this->VERBOSE = verbose;
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

		// Save status
		this->CONNECTED = connected;

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

		//Set timeout for socket
		struct timeval receiving_timeout;
		receiving_timeout.tv_sec = this->IO_TIMEOUT_S;
		receiving_timeout.tv_usec = 0;
		if (setsockopt(this->_socket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
			if (this->VERBOSE) printf("[%s] Error on setting socket timeout.\n", this->CLIENT_NAME.c_str());
			this->Disconnect();
			return std::move(data);
		}

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




	/*
	*
	*
	*	BriandIDFSocketTlsClient
	*
	*
	*/

	BriandIDFSocketTlsClient::BriandIDFSocketTlsClient() : BriandIDFSocketClient() {
		this->CLIENT_NAME = string("BriandIDFSocketTlsClient");
		this->caChainLoaded = false;
		this->caChainFailed = true;
		this->min_rsa_key_size = 2048;

		// Setup resources
		this->SetupResources();
	}

	BriandIDFSocketTlsClient::BriandIDFSocketTlsClient(const int& id) {
		BriandIDFSocketTlsClient();
		this->CLIENT_NAME.append(" #") + std::to_string(id);
	}

	BriandIDFSocketTlsClient::~BriandIDFSocketTlsClient() {
		this->ReleaseResources();
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
		this->_socket = -1;
		if (this->resourcesReady) {
			mbedtls_net_free(&this->tls_socket);
			mbedtls_ssl_free(&this->ssl);
			mbedtls_ssl_config_free(&this->conf);
			mbedtls_x509_crt_free(&this->cacert);
			mbedtls_ctr_drbg_free(&this->ctr_drbg);
			mbedtls_entropy_free(&this->entropy);
		}	
		this->resourcesReady = false;
	}

	void BriandIDFSocketTlsClient::SetTimeout(const unsigned short& connectTimeout_s, const unsigned short& ioTimeout_s) {
		this->CONNECT_TIMEOUT_S = connectTimeout_s;
		this->IO_TIMEOUT_S = ioTimeout_s;
		if (this->resourcesReady) {
			mbedtls_ssl_conf_read_timeout(&this->conf, this->IO_TIMEOUT_S*1000);
		}
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
		mbedtls_ssl_set_bio(&this->ssl, &this->tls_socket, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

		// Set timeout (works only with mbedtls_net_recv_timeout on BIO setup!)
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

		return true;
	}

	void BriandIDFSocketTlsClient::Disconnect() {
		if (this->CONNECTED) {
			mbedtls_ssl_close_notify(&this->ssl);
			this->CONNECTED = false;
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

		// Read until bytes received or jsut one chunk requested
		int ret;
		do {
			auto recvBuffer = make_unique<unsigned char[]>(this->RECV_BUF_SIZE);
			ret = mbedtls_ssl_read(&this->ssl, recvBuffer.get(), this->RECV_BUF_SIZE);
			
			if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
				// Wait
				continue;
			}
			else if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
				// Finish
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

		} while(ret != 0 && !oneChunk);

		if (this->VERBOSE) printf("[%s] Received %d bytes. %s\n", this->CLIENT_NAME.c_str(), data->size(), ((this->AvailableBytes() > 0) ? "More bytes are available." : "No more bytes available."));

		return std::move(data);
	}

	int BriandIDFSocketTlsClient::AvailableBytes() {
		int bytes_avail = 0;

		if (this->CONNECTED) 
			bytes_avail = mbedtls_ssl_get_bytes_avail(&this->ssl);

		return bytes_avail;
	}
}