/*
 * This program is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the 
 * Free Software Foundation, either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/*
    Utilities for coding on the Pi Pico. 

    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */

#pragma once
#include <Arduino.h> 
#include <stdint.h> 

static volatile long            timeBaseMs             __attribute__((section(".uninitialized_data")));

void reboot(); 
String msToHumanReadableTime(long timeMs); 
uint32_t getTotalHeap();
uint32_t getFreeHeap(); 
