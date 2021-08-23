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
		#include <map>
		#include <cstdio>
		#include <cstdlib>
		#include <cstring>
		#include <thread>
		#include <chrono>
		#include <algorithm>
		#include <unistd.h>
		#include <signal.h>

		// Resource usage
		#include <sys/resource.h>
		#include <sys/time.h>

		// Net
		#include <netinet/in.h>
		#include <netinet/tcp.h>
		#include <netdb.h>
		#include <sys/ioctl.h>
		#include <arpa/inet.h>
		#include <sys/select.h>

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

		// GPIOS and system basics

		typedef enum {
			GPIO_NUM_NC = -1,    /*!< Use to signal not connected to S/W */
			GPIO_NUM_0 = 0,     /*!< GPIO0, input and output */
			GPIO_NUM_1 = 1,     /*!< GPIO1, input and output */
			GPIO_NUM_2 = 2,     /*!< GPIO2, input and output */
			GPIO_NUM_3 = 3,     /*!< GPIO3, input and output */
			GPIO_NUM_4 = 4,     /*!< GPIO4, input and output */
			GPIO_NUM_5 = 5,     /*!< GPIO5, input and output */
			GPIO_NUM_6 = 6,     /*!< GPIO6, input and output */
			GPIO_NUM_7 = 7,     /*!< GPIO7, input and output */
			GPIO_NUM_8 = 8,     /*!< GPIO8, input and output */
			GPIO_NUM_9 = 9,     /*!< GPIO9, input and output */
			GPIO_NUM_10 = 10,   /*!< GPIO10, input and output */
			GPIO_NUM_11 = 11,   /*!< GPIO11, input and output */
			GPIO_NUM_12 = 12,   /*!< GPIO12, input and output */
			GPIO_NUM_13 = 13,   /*!< GPIO13, input and output */
			GPIO_NUM_14 = 14,   /*!< GPIO14, input and output */
			GPIO_NUM_15 = 15,   /*!< GPIO15, input and output */
			GPIO_NUM_16 = 16,   /*!< GPIO16, input and output */
			GPIO_NUM_17 = 17,   /*!< GPIO17, input and output */
			GPIO_NUM_18 = 18,   /*!< GPIO18, input and output */
			GPIO_NUM_19 = 19,   /*!< GPIO19, input and output */
			GPIO_NUM_20 = 20,   /*!< GPIO20, input and output */
			GPIO_NUM_21 = 21,   /*!< GPIO21, input and output */
			GPIO_NUM_26 = 26,   /*!< GPIO26, input and output */
			GPIO_NUM_27 = 27,   /*!< GPIO27, input and output */
			GPIO_NUM_28 = 28,   /*!< GPIO28, input and output */
			GPIO_NUM_29 = 29,   /*!< GPIO29, input and output */
			GPIO_NUM_30 = 30,   /*!< GPIO30, input and output */
			GPIO_NUM_31 = 31,   /*!< GPIO31, input and output */
			GPIO_NUM_32 = 32,   /*!< GPIO32, input and output */
			GPIO_NUM_33 = 33,   /*!< GPIO33, input and output */
			GPIO_NUM_34 = 34,   /*!< GPIO34, input and output */
			GPIO_NUM_35 = 35,   /*!< GPIO35, input and output */
			GPIO_NUM_36 = 36,   /*!< GPIO36, input and output */
			GPIO_NUM_37 = 37,   /*!< GPIO37, input and output */
			GPIO_NUM_38 = 38,   /*!< GPIO38, input and output */
			GPIO_NUM_39 = 39,   /*!< GPIO39, input and output */
			GPIO_NUM_40 = 40,   /*!< GPIO40, input and output */
			GPIO_NUM_41 = 41,   /*!< GPIO41, input and output */
			GPIO_NUM_42 = 42,   /*!< GPIO42, input and output */
			GPIO_NUM_43 = 43,   /*!< GPIO43, input and output */
			GPIO_NUM_44 = 44,   /*!< GPIO44, input and output */
			GPIO_NUM_45 = 45,   /*!< GPIO45, input and output */
			GPIO_NUM_46 = 46,   /*!< GPIO46, input mode only */
			GPIO_NUM_MAX,
		/** @endcond */
		} gpio_num_t;
		
		#define GPIO_MODE_DEF_DISABLE         (0b00000000)
		#define GPIO_MODE_DEF_INPUT           (0b00000001)    ///< bit mask for input
		#define GPIO_MODE_DEF_OUTPUT          (0b00000010)    ///< bit mask for output
		#define GPIO_MODE_DEF_OD              (0b00000100)    ///< bit mask for OD mode

		typedef enum {
			GPIO_MODE_DISABLE = GPIO_MODE_DEF_DISABLE,                                                         /*!< GPIO mode : disable input and output             */
			GPIO_MODE_INPUT = GPIO_MODE_DEF_INPUT,                                                             /*!< GPIO mode : input only                           */
			GPIO_MODE_OUTPUT = GPIO_MODE_DEF_OUTPUT,                                                           /*!< GPIO mode : output only mode                     */
			GPIO_MODE_OUTPUT_OD = ((GPIO_MODE_DEF_OUTPUT) | (GPIO_MODE_DEF_OD)),                               /*!< GPIO mode : output only with open-drain mode     */
			GPIO_MODE_INPUT_OUTPUT_OD = ((GPIO_MODE_DEF_INPUT) | (GPIO_MODE_DEF_OUTPUT) | (GPIO_MODE_DEF_OD)), /*!< GPIO mode : output and input with open-drain mode*/
			GPIO_MODE_INPUT_OUTPUT = ((GPIO_MODE_DEF_INPUT) | (GPIO_MODE_DEF_OUTPUT)),                         /*!< GPIO mode : output and input mode                */
		} gpio_mode_t;

		#define esp_restart() { printf("\n\n***** esp_restart() SYSTEM REBOOT. Hit Ctrl-C to restart main program.*****\n\n"); }
        #define gpio_set_level(n, l) { printf("\n\n***** GPIO %d => Level = %d*****\n\n", n, l); }
		#define gpio_get_level(n) GPIO_MODE_DISABLE
        #define gpio_set_direction(n, d) { printf("\n\n***** GPIO %d => Direction = %d*****\n\n", n, d); }


		// ERROR AND LOG FUNCTION

		#define ESP_OK 0
		#define ESP_FAIL -1
		#define ESP_ERR_NOT_FOUND -2
		#define ESP_ERR_NVS_NO_FREE_PAGES -3
		#define ESP_ERR_NVS_NEW_VERSION_FOUND -4

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

		extern map<string, esp_log_level_t> LOG_LEVELS_MAP;
		void esp_log_level_set(const char* tag, esp_log_level_t level);
		esp_log_level_t esp_log_level_get(const char* tag);
		#define ESP_LOGI(tag, _format, ...) { if(esp_log_level_get(tag) >= ESP_LOG_INFO) { printf("I "); printf(tag); printf(" "); printf(_format, ##__VA_ARGS__); } }
		#define ESP_LOGV(tag, _format, ...) { if(esp_log_level_get(tag) >= ESP_LOG_VERBOSE) { printf("V "); printf(tag); printf(" "); printf(_format, ##__VA_ARGS__); } }
		#define ESP_LOGD(tag, _format, ...) { if(esp_log_level_get(tag) >= ESP_LOG_DEBUG) { printf("D "); printf(tag); printf(" "); printf(_format, ##__VA_ARGS__); } }
		#define ESP_LOGE(tag, _format, ...) { if(esp_log_level_get(tag) >= ESP_LOG_ERROR) { printf("E "); printf(tag); printf(" "); printf(_format, ##__VA_ARGS__); } }
		#define ESP_LOGW(tag, _format, ...) { if(esp_log_level_get(tag) >= ESP_LOG_WARN) { printf("W "); printf(tag); printf(" "); printf(_format, ##__VA_ARGS__); } }

		void ESP_ERROR_CHECK(esp_err_t e);


		// FILESYSTEM FUNCTIONS

		// Only definition, not needed!

		typedef struct {
				const char* base_path;          /*!< File path prefix associated with the filesystem. */
				const char* partition_label;    /*!< Optional, label of SPIFFS partition to use. If set to NULL, first partition with subtype=spiffs will be used. */
				unsigned int max_files;         /*!< Maximum files that could be open at the same time. */
				bool format_if_mount_failed;    /*!< If true, it will format the file system if it fails to mount. */
		} esp_vfs_spiffs_conf_t;

		#define esp_vfs_spiffs_register(conf_ptr) ESP_OK
		#define esp_spiffs_info(l, t, u) ESP_OK
		#define esp_vfs_spiffs_unregister(ptr) ESP_OK

		
		// CPU/MEMORY FUNCTIONS

		/**
		 * @brief Flags to indicate the capabilities of the various memory systems
		 */
		#define MALLOC_CAP_EXEC             (1<<0)  ///< Memory must be able to run executable code
		#define MALLOC_CAP_32BIT            (1<<1)  ///< Memory must allow for aligned 32-bit data accesses
		#define MALLOC_CAP_8BIT             (1<<2)  ///< Memory must allow for 8/16/...-bit data accesses
		#define MALLOC_CAP_DMA              (1<<3)  ///< Memory must be able to accessed by DMA
		#define MALLOC_CAP_PID2             (1<<4)  ///< Memory must be mapped to PID2 memory space (PIDs are not currently used)
		#define MALLOC_CAP_PID3             (1<<5)  ///< Memory must be mapped to PID3 memory space (PIDs are not currently used)
		#define MALLOC_CAP_PID4             (1<<6)  ///< Memory must be mapped to PID4 memory space (PIDs are not currently used)
		#define MALLOC_CAP_PID5             (1<<7)  ///< Memory must be mapped to PID5 memory space (PIDs are not currently used)
		#define MALLOC_CAP_PID6             (1<<8)  ///< Memory must be mapped to PID6 memory space (PIDs are not currently used)
		#define MALLOC_CAP_PID7             (1<<9)  ///< Memory must be mapped to PID7 memory space (PIDs are not currently used)
		#define MALLOC_CAP_SPIRAM           (1<<10) ///< Memory must be in SPI RAM
		#define MALLOC_CAP_INTERNAL         (1<<11) ///< Memory must be internal; specifically it should not disappear when flash/spiram cache is switched off
		#define MALLOC_CAP_DEFAULT          (1<<12) ///< Memory can be returned in a non-capability-specific memory allocation (e.g. malloc(), calloc()) call
		#define MALLOC_CAP_IRAM_8BIT        (1<<13) ///< Memory must be in IRAM and allow unaligned access
		#define MALLOC_CAP_RETENTION        (1<<14)

		#define MALLOC_CAP_INVALID          (1<<31) ///< Memory can't be used / list end marker

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

		#define esp_get_free_heap_size() 320000
		size_t heap_caps_get_largest_free_block(uint32_t caps);


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

		#define WIFI_EVENT 0
		#define IP_EVENT 1
		#define IP_EVENT_STA_GOT_IP 2
		#define IP_EVENT_STA_LOST_IP 3
		#define WIFI_EVENT_STA_DISCONNECTED 4
		#define WIFI_EVENT_STA_START 5
		#define WIFI_EVENT_AP_STACONNECTED 6
		#define WIFI_EVENT_AP_STADISCONNECTED 7
		#define WIFI_EVENT_WIFI_READY 8

		#define WIFI_AUTH_OPEN 1
		#define WIFI_AUTH_WPA2_PSK 2
		#define WIFI_AUTH_WPA_WPA2_PSK 3

		typedef enum {
			WIFI_PS_NONE,        /**< No power save */
			WIFI_PS_MIN_MODEM,   /**< Minimum modem power saving. In this mode, station wakes up to receive beacon every DTIM period */
			WIFI_PS_MAX_MODEM,   /**< Maximum modem power saving. In this mode, interval to receive beacons is determined by the listen_interval parameter in wifi_sta_config_t */
		} wifi_ps_type_t;

		// should throw expected events.....

		esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t *instance);
		esp_err_t esp_event_handler_instance_unregister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_instance_t instance);
		esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
		esp_err_t esp_wifi_set_ps(wifi_ps_type_t type);

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

		#define ip4addr_ntoa(addr_ptr) inet_ntoa(addr_ptr)
        #define ip4addr_aton(n, addr_ptr) inet_aton(n, addr_ptr)
        typedef in_addr_t ip4_addr_t;

		#define SNTP_SYNC_STATUS_COMPLETED 0
		#define SNTP_OPMODE_POLL 0
		#define sntp_get_sync_status() SNTP_SYNC_STATUS_COMPLETED
		#define sntp_init() ESP_OK
		#define sntp_setservername(n, host_ptr) ESP_OK
		#define sntp_setoperatingmode(mode) ESP_OK


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
		typedef int BaseType_t;
		typedef uint16_t UBaseType_t;
		typedef void (*TaskFunction_t)( void * );

		/** Task states returned by eTaskGetState. */
		typedef enum
		{
			eRunning = 0,	/* A task is querying the state of itself, so must be running. */
			eReady,			/* The task being queried is in a read or pending ready list. */
			eBlocked,		/* The task being queried is in the Blocked state. */
			eSuspended,		/* The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out. */
			eDeleted,		/* The task being queried has been deleted, but its TCB has not yet been freed. */
			eInvalid		/* Used as an 'invalid state' value. */
		} eTaskState;

		typedef unsigned char StackType_t;
		#define configSTACK_DEPTH_TYPE uint16_t
		typedef BriandIDFPortingTaskHandle* TaskHandle_t;

		/*
		*  Used with the uxTaskGetSystemState() function to return the state of each task in the system.
		*/
		typedef struct xTASK_STATUS
		{
			TaskHandle_t xHandle;			/* The handle of the task to which the rest of the information in the structure relates. */
			const char *pcTaskName;			/* A pointer to the task's name.  This value will be invalid if the task was deleted since the structure was populated! */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
			UBaseType_t xTaskNumber;		/* A number unique to the task. */
			eTaskState eCurrentState;		/* The state in which the task existed when the structure was populated. */
			UBaseType_t uxCurrentPriority;	/* The priority at which the task was running (may be inherited) when the structure was populated. */
			UBaseType_t uxBasePriority;		/* The priority to which the task will return if the task's current priority has been inherited to avoid unbounded priority inversion when obtaining a mutex.  Only valid if configUSE_MUTEXES is defined as 1 in FreeRTOSConfig.h. */
			uint32_t ulRunTimeCounter;		/* The total run time allocated to the task so far, as defined by the run time stats clock.  See http://www.freertos.org/rtos-run-time-stats.html.  Only valid when configGENERATE_RUN_TIME_STATS is defined as 1 in FreeRTOSConfig.h. */
			StackType_t *pxStackBase;		/* Points to the lowest address of the task's stack area. */
			configSTACK_DEPTH_TYPE usStackHighWaterMark;	/* The minimum amount of stack space that has remained for the task since the task was created.  The closer this value is to zero the closer the task has come to overflowing its stack. */
		#if configTASKLIST_INCLUDE_COREID
			BaseType_t xCoreID;				/*!< Core this task is pinned to (0, 1, or -1 for tskNO_AFFINITY). This field is present if CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID is set. */
		#endif
		} TaskStatus_t;
		
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

		void vTaskDelete(TaskHandle_t handle);

		UBaseType_t uxTaskGetNumberOfTasks();
		UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray, const UBaseType_t uxArraySize, uint32_t * const pulTotalRunTime );


		// ESP PTHREADS

		/** pthread configuration structure that influences pthread creation */
		typedef struct {
			size_t stack_size;  ///< The stack size of the pthread
			size_t prio;        ///< The thread's priority
			bool inherit_cfg;   ///< Inherit this configuration further
			const char* thread_name;  ///< The thread name.
			int pin_to_core;    ///< The core id to pin the thread to. Has the same value range as xCoreId argument of xTaskCreatePinnedToCore.
		} esp_pthread_cfg_t;

		esp_pthread_cfg_t esp_pthread_get_default_config(void);
		esp_err_t esp_pthread_set_cfg(const esp_pthread_cfg_t *cfg);
		esp_err_t esp_pthread_get_cfg(esp_pthread_cfg_t *p);
		esp_err_t esp_pthread_init(void);
		

		// MISC

		esp_err_t nvs_flash_init(void);
		esp_err_t nvs_flash_erase(void);
		unsigned int esp_random();

	#endif /* BRIAND_LINUX_PORTING_H */

#endif /* defined(__linux__) */