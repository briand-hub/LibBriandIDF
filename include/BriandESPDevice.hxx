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

#if defined(ESP_PLATFORM)
    #include <sdkconfig.h>
    #include <esp_heap_caps.h>
    // Fixes header files between IDF versions and configuration file.
    #if __has_include(<esp32/spiram.h>)
        #include <esp32/spiram.h>
    #elif __has_include(<esp_spiram.h>)
        #include <esp_spiram.h>
    #else
        #error "NO REQUIRED HEADER FOR SPIRAM (either esp_spiram.h or esp32/spiram.h). CHECK CONFIG FILE TO ENABLE SUPPORT FOR SPIRAM!"
    #endif
    #include <freertos/task.h>
    #include <hal/cpu_hal.h>
    #include <soc/rtc.h>
    #include <soc/soc.h>
#elif defined(__linux__)
    #include "BriandEspLinuxPorting.hxx"
#else
    #error "UNSUPPORTED PLATFORM (ESP32 OR LINUX REQUIRED)"
#endif

using namespace std;

/* Most of the methods are similar or equal to Arduino's ESP framework */

namespace Briand {
    /* Utility class to get (or set) informations about device. */
    class BriandESPDevice {
        public:

        /**
         * Returns total SPIRAM size in bytes
        */
        static size_t GetSPIRAMSize() {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
            return info.total_free_bytes + info.total_allocated_bytes;
        }

        /**
         * Returns free SPIRAM size in bytes
        */
        static size_t GetFreeSPIRAM() {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
            return info.total_free_bytes;
        }

        /**
         * Returns total INTERNAL RAM size in bytes
        */
        static size_t GetBoardRAMSize() {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
            return info.total_free_bytes + info.total_allocated_bytes;
        }

        /**
         * Returns free INTERNAL size in bytes
        */
        static size_t GetFreeBoardRAM() {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
            return info.total_free_bytes;
        }

        /**
         * Returns true if SPIRAM detected
        */
        static bool HasSPIRAM() {
            return GetSPIRAMSize() > 0;
        }

        /**
         * Returns total heap size (includes SPI SRAM)
        */
        static size_t GetHeapSize() {
            size_t tot = 0;
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
            tot += info.total_free_bytes + info.total_allocated_bytes;
            heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
            tot += info.total_free_bytes + info.total_allocated_bytes;
            return tot;
        }

        /**
         * Returns free heap size (includes SPI SRAM)
        */
        static size_t GetFreeHeap() {
            size_t tot = 0;
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
            tot += info.total_free_bytes;
            heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
            tot += info.total_free_bytes;
            return tot;
        }

        /**
         * Returns CPU frequency in MHz
        */
        static unsigned long GetCpuFreqMHz()
        {
            rtc_cpu_freq_config_t cpuConf;
            rtc_clk_cpu_freq_get_config(&cpuConf);
            
            return cpuConf.freq_mhz;
        }

        /**
         * Set CPU frequency in MHz
         * @param MHz frequency in MHz
        */
        static void SetCpuFreqMHz(const unsigned short& MHz)
        {
            rtc_cpu_freq_config_t cpuConf;
            rtc_clk_cpu_freq_mhz_to_config(MHz, &cpuConf);
            rtc_clk_cpu_freq_set_config(&cpuConf);
        }
    
        /**
         * Returns informations about running tasks
         * @return string containing task informations
        */
       static unique_ptr<string> GetSystemTaskInfo() {
            auto info = make_unique<string>();
            auto nTask = uxTaskGetNumberOfTasks();
            auto tArray = make_unique<TaskStatus_t[]>(nTask);
            unsigned int ulTotalRunTime;
            nTask = uxTaskGetSystemState(tArray.get(), nTask, &ulTotalRunTime);
            info->append("#       Name        Min.Stack free    \n");
            for (unsigned int i=0; i<nTask; i++) {
                string temp = "";
                temp.append(std::to_string(tArray[i].xTaskNumber));
                while (temp.length() < 8) temp.append(" ");
                info->append(temp);
                temp.clear();
                if (tArray[i].pcTaskName != NULL) temp.append(tArray[i].pcTaskName);
                while (temp.length() < 12) temp.append(" ");
                info->append(temp);
                temp.clear();
                info->append(to_string(tArray[i].usStackHighWaterMark));
                info->append("\n");
            }

            return std::move(info);
        }
    
        /**
         * Prints out the memory status
        */
        static void PrintMemoryStatus() {
            cout << "Heap total size: " << Briand::BriandESPDevice::GetHeapSize() << endl;
            cout << "Heap total free: " << Briand::BriandESPDevice::GetFreeHeap() << endl;
            cout << "SPIRAM Size: " << Briand::BriandESPDevice::GetSPIRAMSize() << endl;
            cout << "SPIRAM Free: " << Briand::BriandESPDevice::GetFreeSPIRAM() << endl;
            cout << "Internal Size: " << Briand::BriandESPDevice::GetBoardRAMSize() << endl;
            cout << "Internal Free: " << Briand::BriandESPDevice::GetFreeBoardRAM() << endl;
            cout << "MAX SPI allocatable: " << heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) << endl;
            cout << "MAX Internal allocatable: " << heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) << endl;
       }
    };
}

