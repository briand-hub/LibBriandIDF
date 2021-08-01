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

/** This file is a redefinition of ESP IDF functions for Linux Porting */

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

/** Makefile for LINUX (see instructions below)
 
	# Variables to control Makefile operation

	SRCPATH = src/
	INCLUDEPATH = include/
	OUTNAME = main_linux_exe

	CC = g++
	CFLAGS = -g -fpermissive -pthread -lmbedtls -lmbedcrypto -lmbedx509 -lsodium -std=gnu++17


	main:
		$(CC) -o $(OUTNAME) $(SRCPATH)*.cpp $(CFLAGS) -I$(INCLUDEPATH)
 
*/

/** LINUX COMPILATION
 *  
 * 	USE THE INCLUDED Makefile!
 * 
 *  g++ -o main main.cpp -I../include/ -lmbedtls -llibsodium -lmbedcrypto -lmbedx509 -pthread -fpermissive
 *  
 * 	-pthread is MANDATORY
 *  -fpermissive is MANDATORY
 * 	-l (MBEDTLS, MBEDCRYPTO, LIBSODIUM etc. only if required)
 */

#if defined(__linux__)

	#ifndef BRIAND_LINUX_PORTING_H
		#define BRIAND_LINUX_PORTING_H

		#include <iostream>
		#include <memory>
		#include <vector>
		#include <cstdio>
		#include <cstdlib>
		#include <cstring>
		#include <thread>
		#include <chrono>
		#include <unistd.h>

		// Resource usage
		#include <sys/resource.h>
		#include <sys/time.h>

		// Net
		#include <netinet/in.h>
		#include <netinet/tcp.h>
		#include <netdb.h>
		#include <sys/ioctl.h>
		#include <arpa/inet.h>

		// Sockets
		#include <sys/socket.h>

		// Mbedtls
		#include <mbedtls/entropy.h>
		#include <mbedtls/ctr_drbg.h>
		#include <mbedtls/net_sockets.h>
		#include <mbedtls/ssl.h>
		#include <mbedtls/x509.h>
		#include <mbedtls/debug.h>

		// Libsodium
		#include <sodium.h>

		using namespace std;

		// ERROR AND LOG FUNCTION

		#define ESP_OK 0
		#define ESP_ERR_NVS_NO_FREE_PAGES 1
		#define ESP_ERR_NVS_NEW_VERSION_FOUND 2

		typedef int esp_err_t;

		const char *esp_err_to_name(esp_err_t code);

		// LOGGING FUNCTIONS

		typedef enum esp_log_level {
			ESP_LOG_NONE,       /*!< No log output */
			ESP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
			ESP_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
			ESP_LOG_INFO,       /*!< Information messages which describe normal flow of events */
			ESP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
			ESP_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
		} esp_log_level_t;

		extern esp_log_level_t BRIAND_CURRENT_LOG_LEVEL;

		void esp_log_level_set(const char* tag, esp_log_level_t level);

		esp_log_level_t esp_log_level_get(const char* tag);

		void ESP_ERROR_CHECK(esp_err_t e);

		#define ESP_LOGI(tag, _format, ...) { printf("D "); printf(tag); printf(" "); printf(_format, __VA_ARGS__); }
		#define ESP_LOGV(tag, _format, ...) { printf("D "); printf(tag); printf(" "); printf(_format, __VA_ARGS__); }
		#define ESP_LOGD(tag, _format, ...) { printf("D "); printf(tag); printf(" "); printf(_format, __VA_ARGS__); }
		#define ESP_LOGE(tag, _format, ...) { printf("D "); printf(tag); printf(" "); printf(_format, __VA_ARGS__); }
		#define ESP_LOGW(tag, _format, ...) { printf("D "); printf(tag); printf(" "); printf(_format, __VA_ARGS__); }

		// CPU/MEMORY FUNCTIONS

		#define MALLOC_CAP_INTERNAL 0

		typedef struct multi_heap_info {
			unsigned long total_free_bytes;
			unsigned long total_allocated_bytes;
		} multi_heap_info_t;

		typedef struct rtc_cpu_freq_config {
			unsigned long freq_mhz;
		} rtc_cpu_freq_config_t;

		void heap_caps_get_info(multi_heap_info_t* info, uint32_t caps);

		void rtc_clk_cpu_freq_get_config(rtc_cpu_freq_config_t* info);
		void rtc_clk_cpu_freq_mhz_to_config(uint32_t mhz, rtc_cpu_freq_config_t* out);
		void rtc_clk_cpu_freq_set_config(rtc_cpu_freq_config_t* info);

		// WIFI FUNCTIONS

		typedef enum wifi_mode {
			WIFI_MODE_NULL = 0,  /**< null mode */
			WIFI_MODE_STA,       /**< WiFi station mode */
			WIFI_MODE_AP,        /**< WiFi soft-AP mode */
			WIFI_MODE_APSTA,     /**< WiFi station + soft-AP mode */
			WIFI_MODE_MAX
		} wifi_mode_t;

		typedef enum esp_interface {
			ESP_IF_WIFI_STA = 0,     /**< ESP32 station interface */
			ESP_IF_WIFI_AP,          /**< ESP32 soft-AP interface */
			ESP_IF_ETH,              /**< ESP32 ethernet interface */
			ESP_IF_MAX
		} esp_interface_t;

		typedef enum wifi_interface {
			WIFI_IF_STA = 0,
			WIFI_IF_AP  = 1,
		} wifi_interface_t;

		typedef struct {
			uint8_t authmode;
			bool required;
			bool capable;
		} briand_min_required_wifi_config;

		typedef struct {
			uint8_t ssid[32];           /**< SSID of ESP32 soft-AP. If ssid_len field is 0, this must be a Null terminated string. Otherwise, length is set according to ssid_len. */
			uint8_t password[64];       /**< Password of ESP32 soft-AP. */
			uint8_t ssid_len;           /**< Optional length of SSID field. */
			uint8_t channel;            /**< Channel of ESP32 soft-AP */
			uint8_t authmode;  /**< Auth mode of ESP32 soft-AP. Do not support AUTH_WEP in soft-AP mode */
			uint8_t ssid_hidden;        /**< Broadcast SSID or not, default 0, broadcast the SSID */
			uint8_t max_connection;     /**< Max number of stations allowed to connect in, default 4, max 10 */
			uint16_t beacon_interval;   /**< Beacon interval which should be multiples of 100. Unit: TU(time unit, 1 TU = 1024 us). Range: 100 ~ 60000. Default value: 100 */
			uint8_t pairwise_cipher;   /**< pairwise cipher of SoftAP, group cipher will be derived using this. cipher values are valid starting from WIFI_CIPHER_TYPE_TKIP, enum values before that will be considered as invalid and default cipher suites(TKIP+CCMP) will be used. Valid cipher suites in softAP mode are WIFI_CIPHER_TYPE_TKIP, WIFI_CIPHER_TYPE_CCMP and WIFI_CIPHER_TYPE_TKIP_CCMP. */
			bool ftm_responder;         /**< Enable FTM Responder mode */
		} wifi_ap_config_t;

		/** @brief STA configuration settings for the ESP32 */
		typedef struct {
			uint8_t ssid[32];      /**< SSID of target AP. */
			uint8_t password[64];  /**< Password of target AP. */
			uint8_t scan_method;    /**< do all channel scan or fast scan */
			bool bssid_set;        /**< whether set MAC address of target AP or not. Generally, station_config.bssid_set needs to be 0; and it needs to be 1 only when users need to check the MAC address of the AP.*/
			uint8_t bssid[6];     /**< MAC address of target AP*/
			uint8_t channel;       /**< channel of target AP. Set to 1~13 to scan starting from the specified channel before connecting to AP. If the channel of AP is unknown, set it to 0.*/
			uint16_t listen_interval;   /**< Listen interval for ESP32 station to receive beacon when WIFI_PS_MAX_MODEM is set. Units: AP beacon intervals. Defaults to 3 if set to 0. */
			uint8_t sort_method;    /**< sort the connect AP in the list by rssi or security mode */
			briand_min_required_wifi_config  threshold;     /**< When sort_method is set, only APs which have an auth mode that is more secure than the selected auth mode and a signal stronger than the minimum RSSI will be used. */
			briand_min_required_wifi_config pmf_cfg;    /**< Configuration for Protected Management Frame. Will be advertized in RSN Capabilities in RSN IE. */
			uint32_t rm_enabled:1;        /**< Whether Radio Measurements are enabled for the connection */
			uint32_t btm_enabled:1;       /**< Whether BSS Transition Management is enabled for the connection */
			uint32_t reserved:30;         /**< Reserved for future feature set */
		} wifi_sta_config_t;

		/** @brief Configuration data for ESP32 AP or STA.
		 *
		 * The usage of this union (for ap or sta configuration) is determined by the accompanying
		 * interface argument passed to esp_wifi_set_config() or esp_wifi_get_config()
		 *
		 */
		typedef union {
			wifi_ap_config_t  ap;  /**< configuration of AP */
			wifi_sta_config_t sta; /**< configuration of STA */
		} wifi_config_t;

		extern wifi_mode_t BRIAND_CURRENT_WIFIMODE;
		
		extern const char* BRIAND_HOST;

		typedef int wifi_init_config_t;
		wifi_init_config_t WIFI_INIT_CONFIG_DEFAULT();
		esp_err_t esp_wifi_get_mode(wifi_mode_t *mode);
		esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
		esp_err_t esp_netif_init();
		esp_err_t esp_event_loop_create_default();
		esp_err_t esp_wifi_init(const wifi_init_config_t *config);

		struct esp_netif_obj {};
		typedef struct esp_netif_obj esp_netif_t;

		typedef const char*  esp_event_base_t; 

		extern esp_netif_t BRIAND_STA;
		extern esp_netif_t BRIAND_AP;

		esp_netif_t* esp_netif_create_default_wifi_sta(void);
		esp_netif_t* esp_netif_create_default_wifi_ap(void);

		esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]);

		esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6]);

		esp_err_t esp_netif_get_hostname(esp_netif_t *esp_netif, const char **hostname);

		esp_err_t esp_netif_set_hostname(esp_netif_t *esp_netif, const char *hostname);
		esp_err_t esp_wifi_start();
		esp_err_t esp_wifi_stop();
		esp_err_t esp_wifi_connect();
		esp_err_t esp_wifi_disconnect();

		typedef void* esp_event_handler_instance_t;
		typedef void* esp_event_handler_t;
		typedef void wifi_event_ap_staconnected_t;
		typedef void wifi_event_ap_stadisconnected_t;

		#define WIFI_EVENT NULL
		#define IP_EVENT NULL
		#define IP_EVENT_STA_GOT_IP NULL
		#define WIFI_EVENT_STA_START NULL
		#define WIFI_EVENT_AP_STACONNECTED NULL
		#define WIFI_EVENT_AP_STADISCONNECTED NULL
		#define WIFI_EVENT_WIFI_READY NULL

		#define WIFI_AUTH_OPEN 1
		#define WIFI_AUTH_WPA2_PSK 2
		#define WIFI_AUTH_WPA_WPA2_PSK 3

		// should throw expected events.....

		esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t *instance);
		esp_err_t esp_event_handler_instance_unregister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_instance_t instance);
		esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);


		// NETWORKING

		typedef unsigned int esp_ip4_addr_t;

		typedef struct {
			unsigned int ip;      /**< Interface IPV4 address */
			unsigned int netmask; /**< Interface IPV4 netmask */
			unsigned int gw;      /**< Interface IPV4 gateway address */
		} esp_netif_ip_info_t;

		typedef enum esp_netif_dhcp_status {
			ESP_NETIF_DHCP_INIT = 0,    /**< DHCP client/server is in initial state (not yet started) */
			ESP_NETIF_DHCP_STARTED,     /**< DHCP client/server has been started */
			ESP_NETIF_DHCP_STOPPED,     /**< DHCP client/server has been stopped */
			ESP_NETIF_DHCP_STATUS_MAX
		} esp_netif_dhcp_status_t;

		extern esp_netif_dhcp_status_t BRIAND_CURRENT_DHCPC_STATUS;
		extern esp_netif_dhcp_status_t BRIAND_CURRENT_DHCPS_STATUS;
		extern esp_netif_ip_info_t BRIAND_CURRENT_IP;

		esp_err_t esp_netif_dhcpc_get_status(esp_netif_t *esp_netif, esp_netif_dhcp_status_t *status);
		esp_err_t esp_netif_dhcps_get_status(esp_netif_t *esp_netif, esp_netif_dhcp_status_t *status);
		esp_err_t esp_netif_dhcpc_stop(esp_netif_t *esp_netif);
		esp_err_t esp_netif_dhcpc_start(esp_netif_t *esp_netif);
		esp_err_t esp_netif_dhcps_stop(esp_netif_t *esp_netif);
		esp_err_t esp_netif_dhcps_start(esp_netif_t *esp_netif);
		esp_err_t esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info);
		void esp_netif_set_ip4_addr(esp_ip4_addr_t *addr, uint8_t a, uint8_t b, uint8_t c, uint8_t d);

		esp_err_t esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info);

		char *esp_ip4addr_ntoa(const esp_ip4_addr_t *addr, char *buf, int buflen);

		#define MACSTR "%02x:%02x:%02x:02x:%02x:02x"
		#define MAC2STR(x) "00:00:00:00:00:00"


		// TASKS AND TIME

		/** Class to handle the Thread Pool */
		class BriandIDFPortingTaskHandle {
			public:
			bool toBeKilled;
			std::thread::native_handle_type handle;
			std::thread::id thread_id;
			string name;

			BriandIDFPortingTaskHandle(const std::thread::native_handle_type& h, const char* name, const std::thread::id& tid);
			~BriandIDFPortingTaskHandle();
		};

		#define portTICK_PERIOD_MS 1

		typedef uint64_t TickType_t;
		typedef void BaseType_t;
		typedef uint16_t UBaseType_t;
		typedef void (*TaskFunction_t)( void * );
		
		typedef BriandIDFPortingTaskHandle* TaskHandle_t;
		extern unique_ptr<vector<TaskHandle_t>> BRIAND_TASK_POOL;

		void vTaskDelay(TickType_t delay);

		uint64_t esp_timer_get_time();

		BaseType_t xTaskCreate(
				TaskFunction_t pvTaskCode,
				const char * const pcName,
				const uint32_t usStackDepth,
				void * const pvParameters,
				UBaseType_t uxPriority,
				TaskHandle_t * const pvCreatedTask);

		void xTaskDelete(TaskHandle_t handle);

		// MISC

		esp_err_t nvs_flash_init(void);
		esp_err_t nvs_flash_erase(void);
		#define esp_random() rand()

	#endif /* BRIAND_LINUX_PORTING_H */

#endif /* defined(__linux__) */