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
#include "TemperatureSensors.h"
#include <math.h>

using namespace AOS; 
using namespace std;

using AOS::TemperatureSensor; 
using AOS::TemperatureSensors; 

bool TemperatureSensor::readTemp(DS18B20& ds)
{
  if (!hasAddress())
  {
    return false; 
  }

  if (!ds.select(address))
  {
    return false; 
  }

  float reading1C = ds.getTempC(); 
  tempC = reading1C; 
  lastReadMs = millis(); 
  read = true; 
  return true;
}

void TemperatureSensors::discoverSensors() 
{
  while (this->ds.selectNext())
  {
    uint8_t sensorAddress[8];
    ds.getAddress(sensorAddress);
    uint8_t shortAddress = sensorAddress[7]; 
    Serial.printf("Found %d::%s\r\n", shortAddress, TemperatureSensor::addressToString(sensorAddress).c_str()); 
    if (has(shortAddress))
    {
      TemperatureSensor& sensor = get(shortAddress); 
      if(!sensor.hasAddress())
      {
        sensor.setAddress(sensorAddress); 
      }
      else if (!sensor.compareAddress(sensorAddress))
      {
        Serial.printf("ERROR: %s has conflicting address: %s vs %s\r\n", 
            sensor.toString().c_str(), TemperatureSensor::addressToString(sensor.getAddress()).c_str(), TemperatureSensor::addressToString(sensorAddress).c_str()); 
      }
    }
    else 
    {
      TemperatureSensor sensor = TemperatureSensor(sensorAddress);
      add(sensor); 
    }
    delay(100); 
  }

  for (auto &[sA, s] : this->m)
  {
    if(!s.hasAddress())
    {
      Serial.printf("ERROR: sensor %s was not discovered.\r\n", s.toString().c_str()); 
    }
  }
}

/* Reads temperature sensors */
void TemperatureSensors::printSensors()
{
  for (auto &[sA, s] : this->m)
  {
    if (s.hasAddress() && s.isRead())
      Serial.printf("%s: %s\r\n", s.toString().c_str(), s.formatTempC().c_str()); 
  }
}