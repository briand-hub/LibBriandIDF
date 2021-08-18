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

#include "BriandIDFWifiManager.hxx"

#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>
#include <string.h>

/* Framework libraries */
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

using namespace std;

namespace Briand {

	// Define so it can be initialized with first call to GetInstance()
	BriandIDFWifiManager* BriandIDFWifiManager::Instance = NULL;

	BriandIDFWifiManager* BriandIDFWifiManager::GetInstance() {
		// Singleton pattern
		if (BriandIDFWifiManager::Instance == NULL) {
			BriandIDFWifiManager::Instance = new BriandIDFWifiManager();
		}

		return Instance;
	}

	BriandIDFWifiManager::BriandIDFWifiManager() {
		this->VERBOSE = false;
		this->INITIALIZED = false;
		this->STA_CONNECTED = false;
		this->AP_READY = false;
		this->STA_IF_READY = false;
		this->interfaceAP = NULL;
		this->interfaceSTA = NULL;

		// Init interfaces
		this->InitInterfaces();

		// Set default mode AP+STA
		this->SetWifiMode(WIFI_MODE_APSTA);

		// Output
		if (this->VERBOSE) cout << endl << endl << "[WIFI MANAGER] Constructor set wifi mode to: " << this->GetWifiMode() << endl << endl;
	}

	BriandIDFWifiManager::~BriandIDFWifiManager() {
		// Stop wifi
		this->StopWIFI();
		// Clean
		delete Instance;
	}
	
	void BriandIDFWifiManager::SetVerbose(const bool& verbose, const bool& disableEspWifiLog) {
		this->VERBOSE = verbose;
		if (disableEspWifiLog) {
			esp_log_level_set("wifi", ESP_LOG_NONE);
			esp_log_level_set("wifi_init", ESP_LOG_NONE);
			esp_log_level_set("esp_netif_handler", ESP_LOG_NONE);
			esp_log_level_set("phy", ESP_LOG_NONE);
			esp_log_level_set("system_api", ESP_LOG_NONE);
			esp_log_level_set("tcpip_adapter", ESP_LOG_NONE);
			esp_log_level_set("esp_netif_lwip", ESP_LOG_NONE);
			esp_log_level_set("device", ESP_LOG_NONE);
			esp_log_level_set("dhcpc", ESP_LOG_NONE);
		}
		else {
			esp_log_level_set("wifi", ESP_LOG_INFO);
			esp_log_level_set("wifi_init", ESP_LOG_INFO);
			esp_log_level_set("esp_netif_handler", ESP_LOG_INFO);
			esp_log_level_set("phy", ESP_LOG_INFO);
			esp_log_level_set("system_api", ESP_LOG_INFO);
			esp_log_level_set("tcpip_adapter", ESP_LOG_INFO);
			esp_log_level_set("esp_netif_lwip", ESP_LOG_INFO);
			esp_log_level_set("device", ESP_LOG_INFO);
			esp_log_level_set("dhcpc", ESP_LOG_INFO);
		}
	}

	wifi_mode_t BriandIDFWifiManager::GetWifiMode() {
		// Temp for error management
		esp_err_t err;

		wifi_mode_t mode = WIFI_MODE_NULL;
		err = esp_wifi_get_mode(&mode);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on get wifi mode: " << esp_err_to_name(err) << endl;
		}

		return mode;
	}

	void BriandIDFWifiManager::SetWifiMode(wifi_mode_t mode) {
		// Temp for error management
		esp_err_t err;

		err = esp_wifi_set_mode(mode);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on set wifi mode: " << esp_err_to_name(err) << endl;
		}
	}

	bool BriandIDFWifiManager::IsConnected() {
		return this->STA_CONNECTED;
	}

	bool BriandIDFWifiManager::IsAPReady() {
		return this->AP_READY;
	}

	void BriandIDFWifiManager::InitInterfaces() {
		// Temp for error management
		esp_err_t err;
		
		// Initialize the interface

		err = esp_netif_init();
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured during esp_netif_init: " << esp_err_to_name(err) << endl;
			return;
		}

		// Setup the event loop
		err = esp_event_loop_create_default();
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured during esp_event_loop_create_default: " << esp_err_to_name(err) << endl;
			return;
		}

		// Get default parameters for wireless settings
		this->initConfig = WIFI_INIT_CONFIG_DEFAULT();

		// Eventually set different parameters

		// Initialize wifi interface configuration
		err = esp_wifi_init(&this->initConfig);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured during esp_wifi_init: " << esp_err_to_name(err) << endl;
			return;
		}

		// Create the interfaces
		this->interfaceSTA = esp_netif_create_default_wifi_sta();
		this->interfaceAP = esp_netif_create_default_wifi_ap();

		// Set zeros to the current wifi ap/sta configuration
		memset(&this->currentConfig, 0, sizeof(this->currentConfig));

		this->INITIALIZED = true;
	}

	void BriandIDFWifiManager::WiFiEventHandler(void* evtArg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
		// First argument passed to handler is the BriandIDFWifiManager instance (this)
		if (event_id == IP_EVENT_STA_GOT_IP) {
			// Set success on connection
			auto wifiManagerInstance = ((BriandIDFWifiManager*)evtArg);
			if (wifiManagerInstance != nullptr) {
				wifiManagerInstance->STA_CONNECTED = true;
			}
		}
		if (event_id == WIFI_EVENT_STA_START) {
			// Set interface ready (ex. for setting hostname)
			auto wifiManagerInstance = ((BriandIDFWifiManager*)evtArg);
			if (wifiManagerInstance != nullptr) {
				wifiManagerInstance->STA_IF_READY = true;
			}
		}
		if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
			// Set interface ready (ex. for setting hostname)
			auto wifiManagerInstance = ((BriandIDFWifiManager*)evtArg);
			if (wifiManagerInstance != nullptr) {
				wifiManagerInstance->STA_CONNECTED = false;
				printf("[WIFI MANAGER] STA DISCONNECTED event.\n");
			}
		}
		if (event_id == IP_EVENT_STA_LOST_IP) {
			// Set interface ready (ex. for setting hostname)
			auto wifiManagerInstance = ((BriandIDFWifiManager*)evtArg);
			if (wifiManagerInstance != nullptr) {
				wifiManagerInstance->STA_CONNECTED = false;
				printf("[WIFI MANAGER] STA LOST IP event.\n");
			}
		}
		if (event_id == WIFI_EVENT_AP_STACONNECTED) {
			auto wifiManagerInstance = ((BriandIDFWifiManager*)evtArg);
			auto event = (wifi_event_ap_staconnected_t*) event_data;
			if (wifiManagerInstance != nullptr && wifiManagerInstance->VERBOSE) {
				printf("[WIFI MANAGER] station with mac " MACSTR " connected to AP.", MAC2STR(event->mac));
			}
		}
		if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
			auto wifiManagerInstance = ((BriandIDFWifiManager*)evtArg);
			auto event = (wifi_event_ap_stadisconnected_t*) event_data;
			if (wifiManagerInstance != nullptr && wifiManagerInstance->VERBOSE) {
				printf("[WIFI MANAGER] station with mac " MACSTR " disconnected from AP.", MAC2STR(event->mac));
			}
		}
	}

	void BriandIDFWifiManager::SetRandomMAC(const wifi_interface_t& interface) {
		// Temp for error management
		esp_err_t err;

		// Set MAC address
		auto macAddress = make_unique<unsigned char[]>(6);

		esp_wifi_get_mac(interface, macAddress.get());
		
		if (this->VERBOSE) {
			cout << "[WIFI MANAGER] CURRENT MAC: ";
			for(char i=0; i<6; i++) {
				printf("%02X", macAddress[i]);
				if (i<5) printf(":");
			}
			cout << endl;
		} 
		
		// Shuffle but leave unchanged the first bit to zero!
		// This API can only be called when the interface is disabled
		// ESP32 soft-AP and station have different MAC addresses, do not set them to be the same.
		// The bit 0 of the first byte of ESP32 MAC address can not be 1. For example, the MAC address can set to be “1a:XX:XX:XX:XX:XX”, but can not be “15:XX:XX:XX:XX:XX”.		
		macAddress[0] = static_cast<unsigned char>( (esp_random() % 0x100) & 0xFE ); // avoid bit 0 of first byte being 1
		macAddress[1] = static_cast<unsigned char>( esp_random() % 0x100 );
		macAddress[2] = static_cast<unsigned char>( esp_random() % 0x100 );
		macAddress[3] = static_cast<unsigned char>( esp_random() % 0x100 );
		macAddress[4] = static_cast<unsigned char>( esp_random() % 0x100 );
		macAddress[5] = static_cast<unsigned char>( esp_random() % 0x100 );

		err = esp_wifi_set_mac(interface, macAddress.get());
		
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured during esp_wifi_set_mac: " << esp_err_to_name(err) << endl;
		}

		if (this->VERBOSE) {
			cout << "[WIFI MANAGER] NEW MAC: ";
			for(char i=0; i<6; i++) {
				printf("%02X", macAddress[i]);
				if (i<5) printf(":");
			}
			cout << endl;
		}
	}

	void BriandIDFWifiManager::SetHostname(const string& hostname) {
		// Temp for error management
		esp_err_t err;

		// Set hostname
		
		const char* curHostname;
		err = esp_netif_get_hostname(this->interfaceSTA, &curHostname);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured during esp_netif_get_hostname: " << esp_err_to_name(err) << endl;
		}
		
		if (this->VERBOSE) cout << "[WIFI MANAGER] Original STA hostname: " << string(curHostname) << endl;
		
		err = esp_netif_set_hostname(this->interfaceSTA, hostname.c_str());

		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured during esp_netif_set_hostname: " << esp_err_to_name(err) << endl;
		}

		err = esp_netif_get_hostname(this->interfaceSTA, &curHostname);
		if (this->VERBOSE) cout << "[WIFI MANAGER] New STA hostname: " << string(curHostname) << endl;
	}

	bool BriandIDFWifiManager::ConnectStation(const string& essid, const string& password, const int& timeoutSeconds, const string& ovverrideHostname /*= ""*/, const bool& changeMacToRandom/*= true*/) {
		// Temp for error management
		esp_err_t err;
		
		if (this->VERBOSE) cout << "[WIFI MANAGER] Wifi mode is: " << this->GetWifiMode() << endl;

		// Check current mode
		if (this->GetWifiMode() == WIFI_MODE_AP)
			this->SetWifiMode(WIFI_MODE_APSTA);
		else if (this->GetWifiMode() != WIFI_MODE_APSTA)
			this->SetWifiMode(WIFI_MODE_STA);
		
		if (this->VERBOSE) cout << "[WIFI MANAGER] Wifi mode set to: " << this->GetWifiMode() << endl;

		if (!this->INITIALIZED) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (STA) Error, interfaces not initalized enable verbose/logging for details.." << endl;
			return false;
		}

		// Change mac if required
		if (changeMacToRandom) {
			this->SetRandomMAC(WIFI_IF_STA);
		}

		// Configure the connection
		strcpy((char*)this->currentConfig.sta.ssid, essid.c_str());
		strcpy((char*)this->currentConfig.sta.password, password.c_str());

		// Require minimum WPA2-PSK
		this->currentConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		this->currentConfig.sta.pmf_cfg.capable = true;
		this->currentConfig.sta.pmf_cfg.required = false;

		err = esp_wifi_set_config(WIFI_IF_STA, &this->currentConfig);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (STA) Error occoured during esp_wifi_set_config: " << esp_err_to_name(err) << endl;
			return false;
		}

		// Connect and wait for failure or timeout

		long int timeout, now;

		esp_event_handler_instance_t got_ip_event;

/* LINUX PORTING REQUIRES LITTLE MOD HERE */
#if defined(ESP_PLATFORM)

		// Pass this object as argument to event manager
		esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &BriandIDFWifiManager::WiFiEventHandler, this, &got_ip_event);

		// Start interface, attach event handler for interface ready.
		esp_event_handler_instance_t if_ready_change_hostname_event;
		this->STA_IF_READY = false;
		// Pass this object as argument to event manager
		esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_START, &BriandIDFWifiManager::WiFiEventHandler, this, &if_ready_change_hostname_event);

#elif defined(__linux__)

		this->STA_IF_READY = true;
		this->STA_CONNECTED = true;

#endif

		// Always stop & restart
		err = esp_wifi_start();		
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (STA) Error occoured during esp_wifi_start: " << esp_err_to_name(err) << endl;
			return false;
		}

		// Disable powersave
		esp_wifi_set_ps(WIFI_PS_NONE); 

		// Set hostname if needed

		if (ovverrideHostname.length() > 0) {
			if (ovverrideHostname.length() > 32) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] (STA) Hostname too long! (max 32 chars)." << endl;
				return false;
			}

			// Wait for event
			
			while (!this->STA_IF_READY) vTaskDelay(10/portTICK_PERIOD_MS);

			this->SetHostname(ovverrideHostname);
		}

/* LINUX PORTING REQUIRES LITTLE MOD HERE */
#if defined(ESP_PLATFORM)

		// Delete the associated event
		esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_WIFI_READY, &if_ready_change_hostname_event);

#endif

		time(&timeout);
		time(&now);
		timeout += timeoutSeconds;
		
		// Connect
		esp_wifi_connect();

		while ( !this->STA_CONNECTED && (now < timeout) ) {
			if (this->VERBOSE) cout << ".";
			time(&now);
			vTaskDelay(1000/portTICK_PERIOD_MS);
		}
		if (this->VERBOSE) cout << endl;

		if (!this->STA_CONNECTED) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] STA Connect timed out" << endl;
			return false;
		}

/* LINUX PORTING REQUIRES LITTLE MOD HERE */
#if defined(ESP_PLATFORM)

		// Delete the associated event
		esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event);

#endif

		// Get IP info
		esp_netif_ip_info_t ipInfo;
		err = esp_netif_get_ip_info(this->interfaceSTA, &ipInfo);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (STA) Error occoured during esp_netif_get_ip_info: " << esp_err_to_name(err) << endl;
			return false;
		}

		auto buf = make_unique<char[]>(16);
		esp_ip4addr_ntoa(&ipInfo.ip, buf.get(), 15);
		if (this->VERBOSE) cout << "[WIFI MANAGER] (STA) Connected! Your IP: " << buf.get() << endl;

		return this->STA_CONNECTED;
	}

	void BriandIDFWifiManager::DisconnectStation() {
		// Temp for error management
		esp_err_t err;

		err = esp_wifi_disconnect();
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Station disconnect failed: " << esp_err_to_name(err) << endl;
		}

		// Configuration reset, otherwise will not reconnect another time!
		memset(&this->currentConfig.sta, 0, sizeof(this->currentConfig.sta));

		this->STA_CONNECTED = false;
	}

	bool BriandIDFWifiManager::StartAP(const string& essid, const string& password, const unsigned char& channel, const unsigned char& maxConnections, const bool& changeMacToRandom /* = true*/) {
		// Temp for error management
		esp_err_t err;

		if (this->VERBOSE) cout << "[WIFI MANAGER] Wifi mode is: " << this->GetWifiMode() << endl;

		// Check current mode
		if (this->GetWifiMode() == WIFI_MODE_STA)
			this->SetWifiMode(WIFI_MODE_APSTA);
		else if (this->GetWifiMode() != WIFI_MODE_APSTA)
			this->SetWifiMode(WIFI_MODE_AP);

		if (this->VERBOSE) cout << "[WIFI MANAGER] Wifi mode set to: " << this->GetWifiMode() << endl;
		
		this->AP_READY = false;

		if (!this->INITIALIZED) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (AP) Error, interfaces not initalized enable verbose/logging for details.." << endl;
			return false;
		}

		// Change mac if required
		if (changeMacToRandom) {
			this->SetRandomMAC(WIFI_IF_AP);
		}

		// Configure the AP
		strcpy((char*)this->currentConfig.ap.ssid, essid.c_str());
		this->currentConfig.ap.ssid_len = essid.length();
		this->currentConfig.ap.channel = channel;
		strcpy((char*)this->currentConfig.ap.password, password.c_str());
		this->currentConfig.ap.max_connection = maxConnections;
		this->currentConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
		
		if (password.length() == 0) {
			this->currentConfig.ap.authmode = WIFI_AUTH_OPEN;
		}

		err = esp_wifi_set_config(WIFI_IF_AP, &this->currentConfig);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (AP) Error occoured during esp_wifi_set_config: " << esp_err_to_name(err) << endl;
			return false;
		}

		// Start AP
		err = esp_wifi_start();
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] (AP) Error occoured during esp_wifi_start: " << esp_err_to_name(err) << endl;
			return false;
		}

		this->AP_READY = true;

		if (this->VERBOSE) cout << "[WIFI MANAGER] (AP) Started." << endl;

		return true;
	}

	void BriandIDFWifiManager::StopAP() {
		// No method to stop AP, just reset mode.
		if (this->GetWifiMode() == WIFI_MODE_APSTA)
			this->SetWifiMode(WIFI_MODE_STA);
		else if (this->GetWifiMode() == WIFI_MODE_AP)
			this->SetWifiMode(WIFI_MODE_NULL);

		// Configuration reset, otherwise will not reconnect another time!
		memset(&this->currentConfig.ap, 0, sizeof(this->currentConfig.ap));

		this->AP_READY = false;
	}

	void BriandIDFWifiManager::StopWIFI() { 
		esp_wifi_stop();
		// Configuration reset, otherwise will not reconnect another time!
		memset(&this->currentConfig, 0, sizeof(this->currentConfig));
	}

	string BriandIDFWifiManager::GetApIP() {
		if (!this->INITIALIZED)
			return "";

		// Temp for error management
		esp_err_t err;

		esp_netif_ip_info_t info;

		err = esp_netif_get_ip_info(this->interfaceAP, &info);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting info from interface: " << esp_err_to_name(err) << endl;
			return "";
		}
		
		auto buf = make_unique<char[]>(16);
		esp_ip4addr_ntoa(&info.ip, buf.get(), 15);

		return string(buf.get());
	}

	string BriandIDFWifiManager::GetStaIP() {
		if (!this->INITIALIZED)
			return "";

		// Temp for error management
		esp_err_t err;

		esp_netif_ip_info_t info;

		err = esp_netif_get_ip_info(this->interfaceSTA, &info);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting info from interface: " << esp_err_to_name(err) << endl;
			return "";
		}
		
		auto buf = make_unique<char[]>(16);
		esp_ip4addr_ntoa(&info.ip, buf.get(), 15);

		return string(buf.get());
	}

	string BriandIDFWifiManager::GetApMAC() {
		if (!this->INITIALIZED)
			return "";

		// Temp for error management
		esp_err_t err;

		auto mac = make_unique<unsigned char[]>(6);

		err = esp_wifi_get_mac(WIFI_IF_AP, mac.get());
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting mac from interface: " << esp_err_to_name(err) << endl;
			return "";
		}

		ostringstream sbuf("");
		for (char i=0; i<6; i++) {
			sbuf << std::hex << std::setfill('0') << std::setw(2) << static_cast<short>(mac[i]);
			if (i<5) sbuf << ":";
		}

		return sbuf.str();
	}

	string BriandIDFWifiManager::GetStaMAC() {
		if (!this->INITIALIZED)
			return "";

		// Temp for error management
		esp_err_t err;

		auto mac = make_unique<unsigned char[]>(6);

		err = esp_wifi_get_mac(WIFI_IF_STA, mac.get());
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting mac from interface: " << esp_err_to_name(err) << endl;
			return "";
		}

		ostringstream sbuf("");
		for (char i=0; i<6; i++) {
			sbuf << std::hex << std::setfill('0') << std::setw(2) << static_cast<short>(mac[i]);
			if (i<5) sbuf << ":";
		}

		return sbuf.str();
	}

	bool BriandIDFWifiManager::SetApIPv4(const unsigned char& a, const unsigned char& b,const unsigned char& c, const unsigned char& d) {
		esp_err_t err;
		esp_netif_ip_info_t info;
		esp_netif_dhcp_status_t dhcp;

		// Check if DHCP server is enabled
		
		err = esp_netif_dhcps_get_status(this->interfaceAP, &dhcp);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting dhcp server status from interface: " << esp_err_to_name(err) << endl;
			return false;
		}

		if (dhcp != ESP_NETIF_DHCP_STOPPED) {
			err = esp_netif_dhcps_stop(this->interfaceAP);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on stopping dhcp server from interface: " << esp_err_to_name(err) << endl;
				return false;
			}
		}
		
		// Set new IP

		err = esp_netif_get_ip_info(this->interfaceAP, &info);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting info from interface: " << esp_err_to_name(err) << endl;
			return false;
		}

		esp_netif_set_ip4_addr(&info.ip, a, b, c, d);

		err = esp_netif_set_ip_info(this->interfaceAP, &info);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on set IP: " << esp_err_to_name(err) << endl;
			return false;
		}

		// If the DHCP server was enabled re-enable.
		if (dhcp != ESP_NETIF_DHCP_STOPPED) {
			err = esp_netif_dhcps_start(this->interfaceAP);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on restarting dhcp server from interface: " << esp_err_to_name(err) << endl;
				return false;
			}
		}
		
		return true;
	}

	void BriandIDFWifiManager::SetApDHCPServer(const bool& enabled) {
		esp_err_t err;

		if (enabled) {
			err = esp_netif_dhcps_start(this->interfaceAP);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on starting dhcp server from interface: " << esp_err_to_name(err) << endl;
			}
		}
		else {
			err = esp_netif_dhcps_stop(this->interfaceAP);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on starting dhcp server from interface: " << esp_err_to_name(err) << endl;
			}
		}
	}

	bool BriandIDFWifiManager::SetStaIPv4(const unsigned char& a, const unsigned char& b,const unsigned char& c, const unsigned char& d) {
		esp_err_t err;
		esp_netif_ip_info_t info;
		esp_netif_dhcp_status_t dhcp;

		// Check if DHCP enabled
		
		err = esp_netif_dhcps_get_status(this->interfaceSTA, &dhcp);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting dhcp status from interface: " << esp_err_to_name(err) << endl;
			return false;
		}

		if (dhcp != ESP_NETIF_DHCP_STOPPED) {
			err = esp_netif_dhcpc_stop(this->interfaceSTA);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on stopping dhcp from interface: " << esp_err_to_name(err) << endl;
				return false;
			}
		}
		
		err = esp_netif_get_ip_info(this->interfaceSTA, &info);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on getting info from interface: " << esp_err_to_name(err) << endl;
			return false;
		}

		esp_netif_set_ip4_addr(&info.ip, a, b, c, d);

		err = esp_netif_set_ip_info(this->interfaceSTA, &info);
		if (err != ESP_OK) {
			if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on set IP: " << esp_err_to_name(err) << endl;
			return false;
		}

		// DHCP client should not be restored if you called a IP change
		
		return true;
	}

	void BriandIDFWifiManager::SetStaIPv4DHCPClient(const bool& enabled) {
		esp_err_t err;
		
		if (enabled) {
			err = esp_netif_dhcpc_start(this->interfaceSTA);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on starting dhcp client from interface: " << esp_err_to_name(err) << endl;
			}
		}
		else {
			err = esp_netif_dhcpc_stop(this->interfaceSTA);
			if (err != ESP_OK) {
				if (this->VERBOSE) cout << "[WIFI MANAGER] Error occoured on starting dhcp client from interface: " << esp_err_to_name(err) << endl;
			}
		}
	}

	size_t BriandIDFWifiManager::GetObjectSize() {
		size_t oSize = 0;

		oSize += sizeof(*this);
		oSize += sizeof(this->Instance) + (this->Instance != NULL ? sizeof(BriandIDFWifiManager) : 0);

		return oSize;
	}

	void BriandIDFWifiManager::PrintObjectSizeInfo() {
		printf("sizeof(*this) = %zu\n", sizeof(*this));
		printf("sizeof(this->Instance) + (this->Instance != NULL ? sizeof(BriandIDFWifiManager) : 0) = %zu\n", sizeof(this->Instance) + (this->Instance != NULL ? sizeof(BriandIDFWifiManager) : 0));

		printf("TOTAL = %zu\n", this->GetObjectSize());
	}

}