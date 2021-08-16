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

/**
 * This class contains just two abstract methods needed to collect informations about heap object size.
*/

#pragma once

#include <cstdlib>

namespace Briand {
    class BriandESPHeapOptimize {
        public:

        /**
         * Method returns the size of this object
         * @return size of the object
        */
        virtual size_t GetObjectSize() = 0;
        
        /**
         * Method to print in stdout the detailed size of object
        */
        virtual void PrintObjectSizeInfo() = 0;

        /**
         * Virtual destructor to avoid error
         * error: deleting object of polymorphic class type 'Briand::BriandIDFWifiManager' which has non-virtual destructor might cause undefined behavior [-Werror=delete-non-virtual-dtor]
        */
        virtual ~BriandESPHeapOptimize() { /* nothing to destroy here */ }
    };
}