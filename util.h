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
#include <sys/_intsup.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> 
#include <math.h> 

#include <Arduino.h>

double calculate_wet_bulb_temp(double dry_bulb_temp, double relative_humidity);
double calculate_relative_humidity(double td, double tw);
String msToHumanReadableTime(long timeMs); 
void showbits( char x ); 
uint8_t computeParityByte(char * buffer, int length); 
