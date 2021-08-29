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

/** EXAMPLE MAIN FILE */

/** USING IN HEADERS AND .cpp FILES

	#if defined(ESP_PLATFORM)
		#include <esp_wifi.h>
		#include <esp_netif.h>
		#include <esp_log.h>
	#elif defined(__linux__)
		#include "BriandEspLinuxPorting.hxx"
		#include <mbedtls/ssl.h>
	#else 
		#error "UNSUPPORTED PLATFORM (ESP32 OR LINUX REQUIRED)"
	#endif
  
*/

/** Platformio and ESP32 compatible boards
 
	The included sdkconfig could be modified.
	The sample platformio.ini file should be like the one in the sources:

	[env] 
	; Commmon settings
	framework = espidf
	platform = espressif32
	;monitor and upload speed
	monitor_speed = 115200
	;Monitor flags
	monitor_filters = colorize, direct, esp32_exception_decoder
	upload_speed = 921600
	;Partitions (must be set also in menuconfig!)
	board_build.partitions = partitions.csv	
	;Enable C++17 (must be enabled also in .vscode/c_cpp_properties.json by setting "cppStandard": "c++17")
	build_unflags = -fno-exceptions -std=gnu++11
	build_flags = -fexceptions -std=gnu++17

	[env:lolin_d32]
	board = lolin_d32			; Use this for the classic ESP32 module
	board_build.mcu = esp32		; WARNING: use the right chip there!

	[env:esp-wrover-kit]
	;board = esp-wrover-kit		; Use this for the WRover ESP32 (contains 8MB PSram)
	board = lolin_d32
	board_build.mcu = esp32s2	; WARNING: use the right chip there!

	[env:esp32-s2-saola-1]
	board = esp32-s2-saola-1
 
*/

/** Makefile for LINUX (see instructions below)
 
	# Variables to control Makefile operation

	SRCPATH = src/
	INCLUDEPATH = include/
	OUTNAME = main_linux_exe

	CC = g++
	CFLAGS = -g -pthread -lmbedtls -lmbedcrypto -lsodium -std=gnu++17


	main:
		$(CC) -o $(OUTNAME) $(SRCPATH)*.cpp $(CFLAGS) -I$(INCLUDEPATH)
 
*/

/** LINUX COMPILATION
 *  
 * 	USE THE INCLUDED Makefile!
 * 
 *  g++ -o main main.cpp -I../include/ -lmbedtls -llibsodium -lmbedcrypto -pthread
 *  
 * 	-pthread is MANDATORY
 * 	-l (MBEDTLS, MBEDCRYPTO, LIBSODIUM etc. only if required)
 */


#include <iostream>
#include <memory>
#include <vector>

/* Framework libraries */
#if defined(ESP_PLATFORM)
	#include <freertos/FreeRTOS.h>
	#include <freertos/task.h>
	#include <esp_system.h>
	#include <esp_wifi.h>
	#include <esp_event.h>
	#include <esp_log.h>
	#include <esp_int_wdt.h>
	#include <esp_task_wdt.h>
	#include <nvs_flash.h>
	#include <esp_tls.h>
#elif defined(__linux__)
	#include "BriandEspLinuxPorting.hxx"
	#include <mbedtls/ssl.h>
#else 
	#error "UNSUPPORTED PLATFORM (ESP32 OR LINUX REQUIRED)"
#endif

/* Library */
#include "BriandESPDevice.hxx"
#include "BriandIDFWifiManager.hxx"
#include "BriandIDFSocketClient.hxx"
#include "BriandIDFSocketTlsClient.hxx"

using namespace std;

// Required for C++ use WITH IDF!
extern "C" {
	void app_main();
}

void sysinfo(); 		// Early declaration, displays system information
void netinfo(); 		// Early declaration, displays netowrk information
void clienttest();		// Early declaration, test NON-SSL client
void sslclienttest();	// Early declaration, test SSL client

void app_main() {
	// Initialize the NVS 
	esp_err_t ret;
	printf("[INFO] Initializing NVS...");
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	printf("done.\n");

	// Print system informations
    sysinfo();

	// Set cpu frequency to 240 MHz, check with sysinfo()
	Briand::BriandESPDevice::SetCpuFreqMHz(240);
	sysinfo();

	// Get the WiFi manager instance
	auto mgr = Briand::BriandIDFWifiManager::GetInstance();

	// Full log output

	mgr->SetVerbose(true, true);

	// Start & Stop AP

	cout << "****** AP START with default ip and MAC address change" << endl;
	mgr->StartAP("HelloEsp", "0123456789", 13, 1, true);
	netinfo();

	mgr->SetApIPv4(10,0,0,1);
	netinfo();
	
	cout << "****** AP STOP" << endl;
	mgr->StopAP();
	netinfo();

	// Connect station

	cout << "****** STA START and connect, change hostname and MAC address." << endl;
	mgr->ConnectStation("<ESSID>", "<PASSWORD>", 60, "", true);
	netinfo();

	// Use some clients

	clienttest();

	sslclienttest();

	// Disconnect station

	mgr->DisconnectStation();

	// Reboot system
	esp_restart();
}


void sysinfo() {
	cout << "SYS INFO ----------------" << endl;
	cout << "CPU Frequency MHz: " << Briand::BriandESPDevice::GetCpuFreqMHz() << endl;
	Briand::BriandESPDevice::PrintMemoryStatus();
	cout << "-------------------------" << endl;
}

void netinfo() {
	auto mgr = Briand::BriandIDFWifiManager::GetInstance();

	cout << "NET INFO ----------------" << endl;

	cout << "AP IP: " << mgr->GetApIP() << endl;
	cout << "AP MAC: " << mgr->GetApMAC() << endl;
	cout << "STA IP: " << mgr->GetStaIP() << endl;
	cout << "STA MAC: " << mgr->GetStaMAC() << endl;

	cout << "-------------------------" << endl;
}

void sslclienttest() {
	cout << "SSL CLIENT ----------------" << endl;

	string data = "GET /all.json HTTP/1.1\r\n" \
            "Host: ifconfig.io\r\n" \
            "User-Agent: esp-idf/1.0 esp32\r\n" \
			"Connection: close\r\n" \
            "\r\n\r\n";
	
	auto dataV = make_unique<vector<unsigned char>>();
	
	unique_ptr<vector<unsigned char>> resp = nullptr;
	for (char& c: data) dataV->push_back(static_cast<unsigned char>(c));
	
	unsigned short timeout = 5;

	auto cli = make_unique<Briand::BriandIDFSocketTlsClient>();
	cli->SetTimeout(timeout, timeout);
	
	if (!cli->Connect("ifconfig.io", 443)) {
		cout << "SSL-CLIENT CONNECTION ERROR" << endl;
		return;
	}
	
	if (!cli->WriteData(dataV)) {
		cout << "SSL-CLIENT DATA WRITE ERROR" << endl;
		return;
	}
	
	resp = cli->ReadData(false);

	if (resp == nullptr) {
		cout << "SSL-CLIENT DATA READ ERROR" << endl;
		return;
	}

	cout << "SSL-CLIENT DATA RECEIVED:" << resp->size() << " bytes" << endl;

	cout << "---------------------------" << endl;
}

void clienttest() {
	cout << "SOCKET CLIENT ----------------" << endl;

	string data = "GET /all.json HTTP/1.1\r\n" \
            "Host: ifconfig.io\r\n" \
            "User-Agent: esp-idf/1.0 esp32\r\n" \
			"Connection: close\r\n" \
            "\r\n\r\n";
	
	auto dataV = make_unique<vector<unsigned char>>();
	
	unique_ptr<vector<unsigned char>> resp = nullptr;
	for (char& c: data) dataV->push_back(static_cast<unsigned char>(c));
	
	unsigned short timeout = 5;

	auto cli = make_unique<Briand::BriandIDFSocketClient>();
	cli->SetTimeout(timeout, timeout);
	
	if (!cli->Connect("ifconfig.io", 80)) {
		cout << "CLIENT CONNECTION ERROR" << endl;
		return;
	}
	
	if (!cli->WriteData(dataV)) {
		cout << "CLIENT DATA WRITE ERROR" << endl;
		return;
	}
	
	resp = cli->ReadData(false);

	if (resp == nullptr) {
		cout << "CLIENT DATA READ ERROR" << endl;
		return;
	}

	cout << "CLIENT DATA RECEIVED:" << resp->size() << " bytes" << endl;

	cout << "------------------------------" << endl;
}