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

#include "BriandESPHeapOptimize.hxx"

// Esp specific
#if defined(ESP_PLATFORM)
    #include <esp_wifi.h>
#elif defined(__linux__)
	#include "BriandEspLinuxPorting.hxx"
#else
    #error "UNSUPPORTED PLATFORM (ESP32 OR LINUX REQUIRED)"
#endif

using namespace std;

namespace Briand
{
	/**
	 * This class is a simplified management for ESP IDF wifi interfaces
	*/
	class BriandIDFWifiManager : public BriandESPHeapOptimize {
		private:

		static BriandIDFWifiManager* Instance;

		/**
		 * PRIVATE CONSTRUCTOR (Singleton PATTERN!)
		*/
		BriandIDFWifiManager();
		~BriandIDFWifiManager();

		protected:

		bool VERBOSE;
		bool INITIALIZED;
		bool STA_IF_READY;
		bool STA_CONNECTED;
		bool AP_READY;

		/** The returned initialized STA interface */
		esp_netif_obj* interfaceSTA;
		/** The returned initialized AP interface */
		esp_netif_obj* interfaceAP;
		/** The current interface configuration */
		wifi_init_config_t initConfig;
		/** The current WIFI configuration */
		wifi_config_t currentConfig { };

		/**
		 * Event handler
		*/
		static void WiFiEventHandler(void* evtArg, esp_event_base_t event_base, int32_t event_id, void* event_data);

		/**
		 * Set a random MAC address. MUST be called after esp_wifi_init and before any connection
		 * @param if the interface (WIFI_IF_STA or WIFI_IF_AP)
		*/
		void SetRandomMAC(const wifi_interface_t& interface);

		/**
		 * Set hostname method (must be called after EVENT)
		 * @param hostname the hostname to set
		*/
		void SetHostname(const string& hostname);

		/**
		 * Initialize the interfaces, method called by constructor/GetInstance
		*/
		void InitInterfaces();

		public:

		/**
		 * Return the instance (SINGLETON!)
		*/
		static BriandIDFWifiManager* GetInstance();

		/**
		 * Return current wifi mode.
		 * @return one of WIFI_MODE_NULL, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA
		*/
		wifi_mode_t GetWifiMode();

		/**
		 * Sets the wifi mode.
		 * @param mode one of WIFI_MODE_NULL, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA
		*/
		void SetWifiMode(wifi_mode_t mode);

		/**
		 * Set output to console (true) or not.
		 * @param verbose (true/false)
		 * @param disableEspWifiLog disable the ESP wifi log if true, re-enables if false.
		*/
		void SetVerbose(const bool& verbose, const bool& disableEspWifiLog);

		/**
		 * Connects to a WiFi in STA mode, using DHCP.
		 * @param essid the essid
		 * @param password the password
		 * @param timeoutSeconds connection timeout
		 * @param ovverrideHostname populate to change the hostname (max 32 chars), empty for default.
		 * @param changeMacToRandom set to true to change MAC to a random one
		 * @return true if success, false if fails or timeout
		*/
		bool ConnectStation(const string& essid, const string& password, const int& timeoutSeconds, const string& ovverrideHostname = "", const bool& changeMacToRandom = true);

		/**
		 * Method disconnects station
		*/
		void DisconnectStation();

		/**
		 * Method starts AP interface with DHCP enabled
		 * @param essid the essid
		 * @param password the password
		 * @param changeMacToRandom set to true to change MAC to a random one
		 * @return true if success, false if fails
		*/
		bool StartAP(const string& essid, const string& password, const unsigned char& channel, const unsigned char& maxConnections, const bool& changeMacToRandom = true);

		/**
		 * Method stops AP
		*/
		void StopAP();

		/**
		 * Method stops all wifi activity (AP/STA)
		*/
		void StopWIFI();

		/**
		 * Method returns if connected as STA
		 * @returns true if STA connected
		*/
		bool IsConnected();

		/**
		 * Method returns if AP interface is ready
		 * @returns true if AP ready
		*/
		bool IsAPReady();

		/**
		 * Method returns STA ip address, string format
		 * @return ip address
		*/
		string GetStaIP();

		/**
		 * Method returns AP ip address, string format
		 * @return ip address
		*/
		string GetApIP();

		/**
		 * Method returns STA MAC address, string format
		 * @return MAC address
		*/
		string GetStaMAC();

		/**
		 * Method returns AP MAC address, string format
		 * @return MAC address
		*/
		string GetApMAC();

		/**
		 * Method to set AP ipv4 address. Dhcp server will be stopped and restarted
		 * @return true on success
		*/
		bool SetApIPv4(const unsigned char& a, const unsigned char& b,const unsigned char& c, const unsigned char& d);

		/**
		 * Method enables/disables dhcp server on AP
		 * @return true on success
		*/
		void SetApDHCPServer(const bool& enabled);

		/**
		 * Method to set STA ipv4 address. Will disable any dhcp client!
		 * @return true on success
		*/
		bool SetStaIPv4(const unsigned char& a, const unsigned char& b,const unsigned char& c, const unsigned char& d);

		/**
		 * Method enables/disables dhcp client on STA
		 * @return true on success
		*/
		void SetStaIPv4DHCPClient(const bool& enabled);

		/** Inherited from BriandESPHeapOptimize */
		virtual void PrintObjectSizeInfo();
		/** Inherited from BriandESPHeapOptimize */
		virtual size_t GetObjectSize();

	};
}
