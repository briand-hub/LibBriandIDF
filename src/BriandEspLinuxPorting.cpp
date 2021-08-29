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

#if defined(__linux__)

	#include "BriandEspLinuxPorting.hxx"

	const char *esp_err_to_name(esp_err_t code) {
		return "UNDEFINED ON LINUX PLATFORM";
	}

	map<string, esp_log_level_t> LOG_LEVELS_MAP;
	
	void esp_log_level_set(const char* tag, esp_log_level_t level) {
		// If wildcard, all to level.
		if (strcmp(tag, "*") == 0) {
			for (auto it = LOG_LEVELS_MAP.begin(); it != LOG_LEVELS_MAP.end(); ++it) {
				it->second = level;
			}
		}
		else {
			LOG_LEVELS_MAP[string(tag)] = level;
		}
	}

	esp_log_level_t esp_log_level_get(const char* tag) {
		auto it = LOG_LEVELS_MAP.find(string(tag));
		
		if (it == LOG_LEVELS_MAP.end()) {
			// Create
			LOG_LEVELS_MAP[string(tag)] = ESP_LOG_NONE;
		}
		
		return LOG_LEVELS_MAP[string(tag)];
	}

	void ESP_ERROR_CHECK(esp_err_t e) { /* do nothing */ }

	void heap_caps_get_info(multi_heap_info_t* info, uint32_t caps) {
		bzero(info, sizeof(info));
		// Standard ESP 320KB
		// default return 0 for free bytes
		info->total_free_bytes = 0;
		info->total_allocated_bytes = 0;
	}

	void rtc_clk_cpu_freq_get_config(rtc_cpu_freq_config_t* info) { info->freq_mhz = 240; }
	void rtc_clk_cpu_freq_mhz_to_config(uint32_t mhz, rtc_cpu_freq_config_t* out) { out->freq_mhz = mhz; }
	void rtc_clk_cpu_freq_set_config(rtc_cpu_freq_config_t* info) { /* do nothing */ }

	size_t heap_caps_get_largest_free_block(uint32_t caps) { return 0; }

	wifi_mode_t BRIAND_CURRENT_WIFIMODE = WIFI_MODE_NULL;
	const char* BRIAND_HOST = "localhost";

	wifi_init_config_t WIFI_INIT_CONFIG_DEFAULT() { return 0; }
	esp_err_t esp_wifi_get_mode(wifi_mode_t *mode) { *mode = BRIAND_CURRENT_WIFIMODE; return ESP_OK; }
	esp_err_t esp_wifi_set_mode(wifi_mode_t mode) { BRIAND_CURRENT_WIFIMODE = mode; return ESP_OK; }
	esp_err_t esp_netif_init() { return ESP_OK; } 
	esp_err_t esp_event_loop_create_default() { return ESP_OK; } 
	esp_err_t esp_wifi_init(const wifi_init_config_t *config) { return ESP_OK; }

	esp_netif_t BRIAND_STA;
	esp_netif_t BRIAND_AP;

	esp_netif_t* esp_netif_create_default_wifi_sta(void) { return &BRIAND_STA; }
	esp_netif_t* esp_netif_create_default_wifi_ap(void) { return &BRIAND_AP; }

	esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]) {
		// Return always a null mac
		for (char i=0; i<6; i++) mac[i] = 0x00;
		return ESP_OK;
	}

	esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6]) { return ESP_OK; }

	esp_err_t esp_netif_get_hostname(esp_netif_t *esp_netif, const char **hostname) { 
		*hostname = BRIAND_HOST;
		return ESP_OK; 
	}

	esp_err_t esp_netif_set_hostname(esp_netif_t *esp_netif, const char *hostname) { return ESP_OK; }
	esp_err_t esp_wifi_start() { return ESP_OK; }
	esp_err_t esp_wifi_stop() { return ESP_OK; }
	esp_err_t esp_wifi_connect() { return ESP_OK; }
	esp_err_t esp_wifi_disconnect() { return ESP_OK; }
	esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) { return ESP_OK; }

	// should throw expected events.....

	esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t *instance) { 
		return ESP_OK;
	}

	esp_err_t esp_event_handler_instance_unregister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_instance_t instance) {
		return ESP_OK;
	}

	esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
		return ESP_OK;
	}

	esp_netif_dhcp_status_t BRIAND_CURRENT_DHCPC_STATUS = ESP_NETIF_DHCP_STARTED;
	esp_netif_dhcp_status_t BRIAND_CURRENT_DHCPS_STATUS = ESP_NETIF_DHCP_STARTED;
	esp_netif_ip_info_t BRIAND_CURRENT_IP;

	esp_err_t esp_netif_dhcpc_get_status(esp_netif_t *esp_netif, esp_netif_dhcp_status_t *status) {
		*status = BRIAND_CURRENT_DHCPC_STATUS;
		return ESP_OK;
	}

	esp_err_t esp_netif_dhcps_get_status(esp_netif_t *esp_netif, esp_netif_dhcp_status_t *status) {
		*status = BRIAND_CURRENT_DHCPS_STATUS;
		return ESP_OK;
	}

	esp_err_t esp_netif_dhcpc_stop(esp_netif_t *esp_netif) {
		BRIAND_CURRENT_DHCPC_STATUS = ESP_NETIF_DHCP_STOPPED;
		return ESP_OK;
	}

	esp_err_t esp_netif_dhcpc_start(esp_netif_t *esp_netif) {
		BRIAND_CURRENT_DHCPC_STATUS = ESP_NETIF_DHCP_STARTED;
		return ESP_OK;
	}

	esp_err_t esp_netif_dhcps_stop(esp_netif_t *esp_netif) {
		BRIAND_CURRENT_DHCPS_STATUS = ESP_NETIF_DHCP_STOPPED;
		return ESP_OK;
	}

	esp_err_t esp_netif_dhcps_start(esp_netif_t *esp_netif) {
		BRIAND_CURRENT_DHCPS_STATUS = ESP_NETIF_DHCP_STARTED;
		return ESP_OK;
	}

	esp_err_t esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info) {
		return ESP_OK;
	}

	esp_err_t esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info) { return ESP_OK; }

	void esp_netif_set_ip4_addr(esp_ip4_addr_t *addr, uint8_t a, uint8_t b, uint8_t c, uint8_t d) { /* do nothing */ }

	char *esp_ip4addr_ntoa(const esp_ip4_addr_t *addr, char *buf, int buflen) { if (buflen < 9) return NULL; strcpy(buf, "0.0.0.0"); return buf; }

	BriandIDFPortingTaskHandle::BriandIDFPortingTaskHandle(const std::thread::native_handle_type& h, const char* name, const std::thread::id& tid) {
		this->handle = h;
		this->name = string(name);
		this->thread_id = tid;
		this->toBeKilled = false;
	}

	BriandIDFPortingTaskHandle::~BriandIDFPortingTaskHandle() {
		if (esp_log_level_get("ESPLinuxPorting") != ESP_LOG_NONE) cout << "BriandIDFPortingTaskHandle: " << this->name << " destroyed." << endl;
	}

	unique_ptr<vector<TaskHandle_t>> BRIAND_TASK_POOL = nullptr;

	TickType_t CTRL_C_MAX_WAIT = 0; // this is useful max waiting time before killing thread (see main()) 

	void vTaskDelay(TickType_t delay) { 
		if (CTRL_C_MAX_WAIT < delay) CTRL_C_MAX_WAIT = delay;
		std::this_thread::sleep_for( std::chrono::milliseconds(delay) ); 
	}

	uint64_t esp_timer_get_time() { 
		// Should return microseconds!
		auto clockPrecision = std::chrono::system_clock::now().time_since_epoch();
		auto micros = std::chrono::duration_cast<std::chrono::microseconds>(clockPrecision);
		return micros.count(); 
	}

	BaseType_t xTaskCreate(
			TaskFunction_t pvTaskCode,
			const char * const pcName,
			const uint32_t usStackDepth,
			void * const pvParameters,
			UBaseType_t uxPriority,
			TaskHandle_t * const pvCreatedTask)
	{
		// do not worry for prioriry and task depth now...

		std::thread t(pvTaskCode, pvParameters);
		TaskHandle_t tHandle = new BriandIDFPortingTaskHandle(t.native_handle(), pcName, t.get_id());

		if (pvCreatedTask != NULL) {
			*pvCreatedTask = tHandle;
		}

		// Add the task to pool BEFORE detach() otherwise native id is lost
		BRIAND_TASK_POOL->push_back( tHandle );

		t.detach(); // this will create daemon-like threads

		return static_cast<BaseType_t>(BRIAND_TASK_POOL->size()-1); // task index
	}

	void vTaskDelete(TaskHandle_t handle) {
		std::thread::id idToKill;

		if (handle == NULL || handle == nullptr) {
			// Terminate this
			idToKill = std::this_thread::get_id();
		}
		else {
			idToKill = handle->thread_id;
		}

		if (BRIAND_TASK_POOL != nullptr) {
			for (int i = 0; i<BRIAND_TASK_POOL->size(); i++) {
				if (BRIAND_TASK_POOL->at(i)->thread_id == idToKill) {
					BRIAND_TASK_POOL->at(i)->toBeKilled = true;
					break;
				}
			}
		}
	}

	UBaseType_t uxTaskGetNumberOfTasks() {
		if (BRIAND_TASK_POOL == nullptr) return 0;
		return static_cast<UBaseType_t>(BRIAND_TASK_POOL->size());
	}

	UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray, const UBaseType_t uxArraySize, uint32_t * const pulTotalRunTime ) {
		UBaseType_t max = 0;
		if (uxArraySize == 0) return 0;
		if (pulTotalRunTime != NULL) *pulTotalRunTime = 0;
		if (BRIAND_TASK_POOL != nullptr && pxTaskStatusArray != NULL) {
			max = (uxArraySize < static_cast<UBaseType_t>(BRIAND_TASK_POOL->size()) ? uxArraySize :  static_cast<UBaseType_t>(BRIAND_TASK_POOL->size()));
			for (unsigned short i=0; i<max; i++) {
				bzero(&pxTaskStatusArray[i], sizeof(TaskStatus_t));
				pxTaskStatusArray[i].xTaskNumber = i;
				pxTaskStatusArray[i].pcTaskName = BRIAND_TASK_POOL->at(i)->name.c_str();
				//
				// TODO : calculate phtread stack size
				//
				pxTaskStatusArray[i].usStackHighWaterMark = 0;
			}
		}
		return max;
	}

	esp_err_t nvs_flash_init(void) { return ESP_OK; }
	esp_err_t nvs_flash_erase(void) { return ESP_OK; }
	unsigned int esp_random() {
		return static_cast<unsigned int>(rand());
	}

	esp_pthread_cfg_t esp_pthread_get_default_config(void) {
		// Like the default configuration
		esp_pthread_cfg_t defaults;
		defaults.stack_size = 2048;
		defaults.inherit_cfg = false;
		defaults.pin_to_core = 0;
		defaults.prio = 5;
		defaults.thread_name = "pthread";
		return defaults;
	}

	esp_err_t esp_pthread_set_cfg(const esp_pthread_cfg_t *cfg) {
		// do nothing
		return ESP_OK;
	}

	esp_err_t esp_pthread_get_cfg(esp_pthread_cfg_t *p) {
		if (p != NULL) *p = esp_pthread_get_default_config();
		return ESP_OK;
	}

	esp_err_t esp_pthread_init(void) {
		// do nothing
		return ESP_OK;
	}

	// app_main() early declaration with extern keyword so will be found
	extern "C" { void app_main(); }

	// Ctrl-C event handler
	bool CTRL_C_EVENT_SET = false;
	void sig_hnd_Ctrl_C(int s) { CTRL_C_EVENT_SET = true; } 

	// main() method required

	int main(int argc, char** argv) {
		// srand for esp_random()
		srand(time(NULL));

		// Add this to the logging utils in order to deactivate output if necessary
		esp_log_level_set("ESPLinuxPorting", ESP_LOG_NONE);

		// Save this thread id
		cout << "MAIN THREAD ID: " << std::this_thread::get_id() << endl;

		// Attach Ctrl-C event handler
		sighandler_t oldHandler = signal(SIGINT, sig_hnd_Ctrl_C);

		// Will create the app_main() method and then remains waiting like esp
		// Will also do the task scheduler work to check if any thread should be killed
		cout << "main(): Starting. Creating task pool simulation..." << endl;

		BRIAND_TASK_POOL = make_unique<vector<TaskHandle_t>>();
		
		cout << "main() Pool started. Use Ctrl-C to terminate" << endl;
		
		cout << "Starting app_main()" << endl;

		app_main(); // This must not be a thread because it terminates!

		cout << "app_main() started." << endl;

		while(!CTRL_C_EVENT_SET) { 
			// Check if any instanced thread should be terminated
			for (int i=0; i<BRIAND_TASK_POOL->size(); i++) {
				if (BRIAND_TASK_POOL->at(i)->toBeKilled) {
					string tname = BRIAND_TASK_POOL->at(i)->name;
					pthread_cancel(BRIAND_TASK_POOL->at(i)->handle);
					delete BRIAND_TASK_POOL->at(i);
					BRIAND_TASK_POOL->erase(BRIAND_TASK_POOL->begin() + i);
					if (esp_log_level_get("ESPLinuxPorting") != ESP_LOG_NONE) cout << "Thread #" << i << "(" << tname << ") killed" << endl;
				}
			}
				
			std::this_thread::sleep_for( std::chrono::milliseconds(500) ); 
		}

		cout << endl << endl << "*** Ctrl-C event caught! ***" << endl << endl;

		// Reset the original signal handler
		signal(SIGINT, oldHandler);

		// Kill all processes (from newer to older)
		for (int i=BRIAND_TASK_POOL->size() - 1; i>=0; i--) {
			if (BRIAND_TASK_POOL->at(i) != NULL) {
				string tname = BRIAND_TASK_POOL->at(i)->name;
				pthread_cancel(BRIAND_TASK_POOL->at(i)->handle);
				delete BRIAND_TASK_POOL->at(i);
				cout << "Thread #" << i << "(" << tname << ") killed" << endl;
			} 
		}
				
		cout << endl << endl << "*** All threads killed! Exiting. ***" << endl << endl;
		raise(SIGINT);

		return 0;
	}

#endif