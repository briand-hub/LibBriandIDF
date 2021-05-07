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

#include <iostream>
#include <memory>

// Esp specific
#include <esp_wifi.h>

using namespace std;

namespace Briand
{
	/**
	 * This class is a simplified management for ESP IDF wifi interfaces
	*/
	class BriandIDFWifiManager {
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
	};
}
