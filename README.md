# LibBriandIDF

C++17 Utility library for ESP32 IDF Framework. Built with PlatformIO and IDF v4.2 (latest stable).

## NEW RELEASE WITH LINUX PORTING

A new release (1.1) has a Makefile and could be compiled under Linux environment for any testing.

Required libraries: mbedtls and libsodium (ESP32/IDF defaults). Could be installed with (Debian):

```bash
sudo apt-get install libsodium-dev libmbedtls-dev
```

**See Makefile and header/cpp files BriandEspLinuxPorting for more details**

*WARNING*: the IDF porting is not **full** but will be added with functions needed for other projects. 

## Install

In your platformio.ini file add:
```ini
lib_deps = https://github.com/briand-hub/LibBriandIDF
```

Or, for specific version:

```ini
lib_deps = https://github.com/briand-hub/LibBriandIDF@1.0.0
```

Rember to enable C++17 support as follows:

1. Add build/unbuild flags 

```ini
[env:lolin_d32]
platform = espressif32
board = lolin_d32
framework = espidf

build_unflags = -fno-exceptions -std=gnu++11
build_flags = -fexceptions -std=gnu++17
```

2. Edit your .vscode/c_cpp_properties.json

Change:

```json
  "cStandard": "c99",
  "cppStandard": "c++11",
```

to:

```json
  "cStandard": "c99",
  "cppStandard": "c++17",
```

## Usage 

Library is well-documented so few examples are provided.

### Prerequisites

If compiled with C++17 rename main.c to main.cpp and add:

```C
// Required for C++ use WITH IDF!
extern "C" {
	void app_main();
}
```

Remember in your app_main() to load NVS (required to use the Wifi management object):


```C
void app_main() {
	// Initialize the NVS
	cout << "Initializing NVS...";
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	ESP_ERROR_CHECK(nvs_flash_erase());
	ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	cout << "done." << endl;
}
```

### General

All objects has a SetVerbose method that accept at least one bool parameter. If set to true debug output will be printed to stdout. Example:

```C
obj->SetVerbose(false);  // no stdout output
```

### ESP Device

Is a simple class with static methods to get (or set) informations about ESP32, for example heap size, cpu frequency and so on.

### Wi-Fi management object

A singleton-pattern object is used, called BriandIDFWifiManager. You can refer to instance using:

```C
auto mgr = Briand::BriandIDFWifiManager::GetInstance();
```

**Start wifi as station**

```C
mgr->ConnectStation("<ESSID>", "<Password>");
```

Full parameters (change hostname, change MAC address, set a timeout of 60 seconds):

```C
mgr->ConnectStation("<ESSID>", "<Password>", 60, "new-hostname", true);
```

**Start wifi as AP**

Example to start an AP with given password (if empty will be open) on channel 1 accepting 5 connections and changing the MAC address.

```C
mgr->StartAP("<ESSID>", "<password>", 1, 5, true); 
```

Please refer to code docs for more informations.

### Clients

Library includes two clients, based on sockets. One is a classic BSD socket client, the other uses TLS connection provided with MbedTLS framework library.
If used inside *xTaskCreate* let the stack size be at least 2048 for the non-tls client, and 4096 for the tls client. Prefer always heap-object allocation.

**Clear-text client example**

```C
void taskNonSSL(void* arg) {
	string data =
			"GET /all.json HTTP/1.1\r\n" \
            "Host: ifconfig.io\r\n" \
            "User-Agent: esp-idf/1.0 esp32\r\n" \
			"Connection: close\r\n" \
            "\r\n\r\n";
	
	auto dataV = make_unique<vector<unsigned char>>();
	for (char& c: data) dataV->push_back(c);

	auto client = make_unique<Briand::BriandIDFSocketClient>();
	client->SetVerbose(false);
	client->Connect("ifconfig.io", 80);
	client->WriteData(dataV);
	auto rec = client->ReadData();
	client->Disconnect();
	rec.reset();
	client.reset();
}
```

**TLS client example**

```C

const char* caRoot = "-----BEGIN CERTIFICATE-----\n" \
"MIIEZTCCA02gAwIBAgIQQAF1BIMUpMghjISpDBbN3zANBgkqhkiG9w0BAQsFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTIwMTAwNzE5MjE0MFoXDTIxMDkyOTE5MjE0MFow\n" \
"MjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxCzAJBgNVBAMT\n" \
"AlIzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuwIVKMz2oJTTDxLs\n" \
"jVWSw/iC8ZmmekKIp10mqrUrucVMsa+Oa/l1yKPXD0eUFFU1V4yeqKI5GfWCPEKp\n" \
"Tm71O8Mu243AsFzzWTjn7c9p8FoLG77AlCQlh/o3cbMT5xys4Zvv2+Q7RVJFlqnB\n" \
"U840yFLuta7tj95gcOKlVKu2bQ6XpUA0ayvTvGbrZjR8+muLj1cpmfgwF126cm/7\n" \
"gcWt0oZYPRfH5wm78Sv3htzB2nFd1EbjzK0lwYi8YGd1ZrPxGPeiXOZT/zqItkel\n" \
"/xMY6pgJdz+dU/nPAeX1pnAXFK9jpP+Zs5Od3FOnBv5IhR2haa4ldbsTzFID9e1R\n" \
"oYvbFQIDAQABo4IBaDCCAWQwEgYDVR0TAQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8E\n" \
"BAMCAYYwSwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5p\n" \
"ZGVudHJ1c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTE\n" \
"p7Gkeyxx+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEE\n" \
"AYLfEwEBATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2Vu\n" \
"Y3J5cHQub3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0\n" \
"LmNvbS9EU1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYf\n" \
"r52LFMLGMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjANBgkqhkiG9w0B\n" \
"AQsFAAOCAQEA2UzgyfWEiDcx27sT4rP8i2tiEmxYt0l+PAK3qB8oYevO4C5z70kH\n" \
"ejWEHx2taPDY/laBL21/WKZuNTYQHHPD5b1tXgHXbnL7KqC401dk5VvCadTQsvd8\n" \
"S8MXjohyc9z9/G2948kLjmE6Flh9dDYrVYA9x2O+hEPGOaEOa1eePynBgPayvUfL\n" \
"qjBstzLhWVQLGAkXXmNs+5ZnPBxzDJOLxhF2JIbeQAcH5H0tZrUlo5ZYyOqA7s9p\n" \
"O5b85o3AM/OJ+CktFBQtfvBhcJVd9wvlwPsk+uyOy2HI7mNxKKgsBTt375teA2Tw\n" \
"UdHkhVNcsAKX1H7GNNLOEADksd86wuoXvg==\n" \
"-----END CERTIFICATE-----\n";

string caRootString = string(caRoot);

void taskSSL(void* arg) {
	string data =
			"GET /a/check HTTP/1.1\r\n" \
            "Host: www.howsmyssl.com\r\n" \
            "User-Agent: esp-idf/1.0 esp32\r\n" \
			"Connection: close\r\n" \
            "\r\n\r\n";
	
	auto dataV = make_unique<vector<unsigned char>>();
	for (char& c: data) dataV->push_back(c);

	auto client = make_unique<Briand::BriandIDFSocketTlsClient>();
	client->SetVerbose(true);
	// Provide a certificate (if provided will be always verified)
	client->SetCACertificateChainPEM(caRootString);
	client->Connect("www.howsmyssl.com", 443);
	fflush(stdout);
	client->WriteData(dataV);
	auto rec = client->ReadData();
	cout << "DATA: ";
	for (int i=0; i<rec->size(); i++) cout << rec->at(i);
	cout << endl;
	client->Disconnect();
	rec.reset();
	client.reset();
}
```

**TLS client (with no verification) example**

```C
void taskSSL(void* arg) {
	string data =
			"GET /a/check HTTP/1.1\r\n" \
            "Host: www.howsmyssl.com\r\n" \
            "User-Agent: esp-idf/1.0 esp32\r\n" \
			"Connection: close\r\n" \
            "\r\n\r\n";
	
	auto dataV = make_unique<vector<unsigned char>>();
	for (char& c: data) dataV->push_back(c);

	auto client = make_unique<Briand::BriandIDFSocketTlsClient>();
	client->SetVerbose(true);
	
	// Do not provide a certificate, no verifications will be done!
	
	client->Connect("www.howsmyssl.com", 443);
	fflush(stdout);
	client->WriteData(dataV);
	auto rec = client->ReadData();
	cout << "DATA: ";
	for (int i=0; i<rec->size(); i++) cout << rec->at(i);
	cout << endl;
	client->Disconnect();
	rec.reset();
	client.reset();
}
```

**TLS: remember to sync time with NTP**

In order to successful certificate validation, consider to sync time with NTP. Example:

```C
static void SetTimeWithNTP() {
	// MUST BE CONNECTED TO INTERNET!
	
	// Set timezone to UTC and no daylight saving
	// details: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
	
	setenv("TZ", "UTC", 1);
	tzset();
	
	// Perform sync (use preferred mode)
	//sntp_setoperatingmode(SNTP_SYNC_MODE_SMOOTH);

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");

	sntp_init();
	// Wait until timeout or success
	int maxTentatives = 60; // = 30 seconds
	while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && maxTentatives > 0) {
		maxTentatives--;
		vTaskDelay(500/portTICK_PERIOD_MS);
	}
}
```

Please refer to code docs for more informations.
