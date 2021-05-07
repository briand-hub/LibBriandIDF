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

#include <esp_heap_caps.h>
#include <esp_spiram.h>
#include <hal/cpu_hal.h>
#include <soc/rtc.h>
#include <soc/soc.h>

using namespace std;

/* Most of the methods are similar or equal to Arduino's ESP framework */

namespace Briand {
    /* Utility class to get (or set) informations about device. */
    class BriandESPDevice {

        static bool HasPSram() {
            esp_spiram_init_cache();
            if (!esp_spiram_test()) {
                return false;
            }
            if (esp_spiram_add_to_heapalloc() != ESP_OK) {
                return false;
            }

            return true;
        }

        /**
         * Returns total heap size
        */
        static unsigned long GetHeapSize() {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
            return info.total_free_bytes + info.total_allocated_bytes;
        }

        /**
         * Returns free heap size
        */
        static unsigned long GetFreeHep() {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
            return info.total_free_bytes;
        }

        /**
         * Returns total PSram if available
        */
        static unsigned long GetPsramSize()
        {
            if(HasPSram()){
                multi_heap_info_t info;
                heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
                return info.total_free_bytes + info.total_allocated_bytes;
            }

            return 0;
        }

        /**
         * Returns free PSram if available
        */
        static unsigned long GetFreePsram()
        {
            if(HasPSram()){
                return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            }

            return 0;
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
    };
}
