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

double calculate_wet_bulb_temp(double dry_bulb_temp, double relative_humidity) {
    double rh = relative_humidity;
    double T = dry_bulb_temp;
    
    double Tw = T * atan(0.151977 * sqrt(rh + 8.313659)) + atan(T + rh) - atan(rh - 1.676331) + 0.00391838 * pow(rh, 1.5) * atan(0.023101 * rh) - 4.686035;
    
    return Tw;
}

// https://www.1728.org/relhum.htm
double calculate_relative_humidity(double td, double tw) 
{        
    double N = 0.6687451584; 
    double ed = 6.112 * exp((17.502 * td) / (240.97 + td)); 
    double ew = 6.112 * exp((17.502 * tw) / (240.97 + tw));

    if (ed <= 0)
      return 0; 

    double rh = (
      ( ew - N * (1+ 0.00115 * tw) * (td - tw) ) 
      / ed ) 
        * 100.0; 

    return rh; 
}

bool TemperatureSensor::readTemp(DS18B20 ds)
{
  if (!hasAddress())
  {
    Serial.printf("Sensor %s has not been discovered\r\n", toString().c_str()); 
    return false; 
  }

  if (!ds.select(address))
  {
    Serial.printf("Sensor %s not found\r\n", toString().c_str()); 
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
    Serial.printf("Sensor %s: TEMP ERROR %f %f\r\n", toString().c_str(), reading1C, reading2C); 
    tempErrors++; 
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
      if(!sensor.hasAddress())
      {
        sensor.setAddress(sensorAddress); 
        //Serial.printf("Discovered %s\r\n", sensor.toString().c_str()); 
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
      Serial.printf("Discovered unexpected sensor: %s\r\n", sensor.toString().c_str()); 
      add(sensor); 
    }
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
    Serial.printf("%s\r\n", s.toString().c_str()); 
  }
}