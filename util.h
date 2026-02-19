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
#include <iostream>
#include <string>
#include <initializer_list>
#include <algorithm>

#include <Arduino.h>

uint8_t extrapolatePWM(float gapC, float rangeC, float minGapC, float pwmMin, float pwmMax);
float extrapolateGradualPWM(float gapC, float rangeC, float minGapC, float pwmMin, float pwmMax, float lastPwm, float maxAdjustment);
float computeGradientC(float sourceTempC, float targetTempC, float toleranceC);
float clampf(float value, float min, float max);
int clampi(int value, int min, int max);

float fmaxv(std::initializer_list<float> values );

void pryApart(volatile float* pwm1, volatile float* pwm2, float minDifference, float minValue, float maxValue);

double calculate_wet_bulb_temp(double dry_bulb_temp, double relative_humidity);
double calculate_relative_humidity(double td, double tw);
String msToHumanReadableTime(long timeMs); 
String secondsToHumanReadableTime(double seconds);
String secondsToHMS(double timeSeconds);
void showbits( char x ); 
uint8_t computeParityByte(char * buffer, int length); 
