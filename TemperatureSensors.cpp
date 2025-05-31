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

#include <streambuf>
#include "TemperatureSensors.h"
#include <Arduino.h>
#include <cstddef>
#include <string.h>
#include <string>
#include <map>
#include <stdio.h>
#include <float.h>
#include "Thermostats.h"
#include "util.h"
#include "DS18B20.h"

using namespace AOS; 
using namespace std;

using AOS::TemperatureSensor; 
using AOS::TemperatureSensors; 

bool TemperatureSensor::readTemp(DS18B20 ds)
{
  char *sensorString = toString(); 
  if (!hasAddress())
  {
    Serial.printf("Sensor %s has not been discovered\r\n", sensorString); 
    free(sensorString); 
    return false; 
  }

  if (!ds.select(address))
  {
    Serial.printf("Sensor %s not found\r\n", sensorString); 
    free(sensorString);
    return false; 
  }

  float reading1C = ds.getTempC(); 
  float reading2C = ds.getTempC(); 
  float diff = fabs(reading1C - reading2C); 
  if (diff < 0.5 && isTempCValid(reading1C))
  {
    tempC = reading1C; 
    lastReadMs = millis(); 
    read = true; 
    return true;
  }
  else 
  {
    Serial.printf("Sensor %s: TEMP ERROR %f %f\r\n", sensorString, reading1C, reading2C); 
    tempErrors++; 
    free(sensorString);
    return false; 
  }
}

void TemperatureSensors::discoverSensors() 
{
  while (this->ds.selectNext())
  {
    uint8_t sensorAddress[8];
    ds.getAddress(sensorAddress);
    uint8_t shortAddress = sensorAddress[7]; 
    if (has(shortAddress))
    {
      TemperatureSensor& sensor = get(shortAddress); 
      char *sensorString = sensor.toString(); 
      if(!sensor.hasAddress())
      {
        sensor.setAddress(sensorAddress); 
        //Serial.printf("Discovered %s\r\n", sensorString); 
      }
      else if (!sensor.compareAddress(sensorAddress))
      {
        Serial.printf("ERROR: %s has conflicting address: %s vs %s\r\n", 
            sensorString, TemperatureSensor::addressToString(sensor.getAddress()).c_str(), TemperatureSensor::addressToString(sensorAddress).c_str()); 
      }
      free(sensorString);
    }
    else 
    {
      TemperatureSensor sensor = TemperatureSensor(sensorAddress);
      char *sensorString = sensor.toString(); 
      Serial.printf("Discovered unexpected sensor: %s\r\n", sensorString); 
      add(sensor); 
      free(sensorString);
    }
  }

  for (auto &[sA, s] : this->m)
  {
    if(!s.hasAddress())
    {
      char *sensorString = s.toString(); 
      Serial.printf("ERROR: sensor %s was not discovered.\r\n", sensorString); 
      free(sensorString);
    }
  }
}

/* Reads temperature sensors */
void TemperatureSensors::printSensors()
{
  for (auto &[sA, s] : this->m)
  {
    char *sensorString = s.toString(); 
    Serial.printf("%s\r\n", sensorString); 
    free(sensorString);
  }
}