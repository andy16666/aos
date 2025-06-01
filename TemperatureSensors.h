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
    A container for DS18B20 temperature sensors on a given GPIO pin

    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */

#pragma once
#include "SerialUSB.h"
#include <Arduino.h>
#include <DS18B20.h>
#include <sys/_types.h>
#include <sys/_stdint.h>
#include <sys/_intsup.h>
#include <cstdlib>
#include <map>
#include <string>
#include <float.h>
#include <stdint.h>

// #define NDEBUG
#include <cassert>

using namespace std;

namespace AOS
{
  const unsigned int ADDRESS_LENGTH = 8; 
  const float MAX_TEMP_C = 120; 
  const float MIN_TEMP_C = -50;  
  const float INVALID_TEMP  = FLT_MIN;
  const unsigned long TEMP_EXPIRY_TIME_MS = 15000; 
  const unsigned long TEMP_VALID_TIME_MS = 60000; 

  class TemperatureSensors; 
  class TemperatureSensor; 

  class TemperatureSensor
  {
    private:
      uint8_t shortAddress;
      uint8_t address[ADDRESS_LENGTH];
      std::string name; 
      float tempC; 
      bool read; 
      unsigned long lastReadMs;
      unsigned long tempErrors;  
      
    public:
      TemperatureSensor() {}; 
      TemperatureSensor(std::string name, uint8_t shortAddress)
      {
        this->name = std::string(name); 
        this->shortAddress = shortAddress; 
        for (int i = 0; i < ADDRESS_LENGTH; i++) { this->address[i] = 0; }
        read = false; 
        lastReadMs = 0; 
      }; 
      
      TemperatureSensor(uint8_t address[ADDRESS_LENGTH])
      {
        setAddress(address); 
        this->name = "Discovered"; 
        read = false; 
        lastReadMs = 0; 
      }; 

      std::string getName()
      {
        return name;
      };

      bool readTemp(DS18B20 ds); 
      bool isRead() { return read; };
      bool hasAddress() { return address[ADDRESS_LENGTH-1] == shortAddress; };
      uint8_t* getAddress() { return address; };
      uint8_t getShortAddress() { return shortAddress; }
      
      bool isTempExpired() 
      {
        return !read || lastReadMs < (millis() - TEMP_EXPIRY_TIME_MS); 
      };

      float getTempC() { return tempC; }; 
      
      bool isTempValid()
      {
        return read && isTempCValid(tempC) && lastReadMs >= (millis() - TEMP_VALID_TIME_MS);
      };

      void setName(std::string name)
      {
        this->name = name; 
      };

      void setAddress(uint8_t address[ADDRESS_LENGTH]) 
      {  
        shortAddress = address[ADDRESS_LENGTH-1]; 
        for (int i = 0; i < ADDRESS_LENGTH; i++) { this->address[i] = address[i]; }
      };

      bool compareAddress(uint8_t address[ADDRESS_LENGTH])
      {
        if (!hasAddress())
        {
          return false; 
        }

        for (int i = 0; i < ADDRESS_LENGTH; i++) 
        { 
          if (this->address[i] != address[i])
            return false; 
        }

        return true; 
      };

      String toString()
      {
        char buffer[1024];
        sprintf(buffer, "{ \"name\":\"%s\", \"shortAddress\":\"%d\" }", name.c_str(), shortAddress);
        return String(buffer); 
      };

      static String addressToString(uint8_t *address)
      {
        String addressString = ""; 
        for (int i = 0; i < ADDRESS_LENGTH; i++)
        {
          if (i > 0) addressString += ":";
          addressString += address[i];
        }
        return addressString;
      };

      static bool isTempCValid(float tempC) 
      {
        return tempC != INVALID_TEMP
          && tempC == tempC 
          && tempC < MAX_TEMP_C 
          && tempC > MIN_TEMP_C; 
      };
  };

  class TemperatureSensors
  {
    private:
      std::map<uint8_t, TemperatureSensor> m;
      uint8_t pin; 
      DS18B20 ds; 
      bool discovered; 

    public: 
      TemperatureSensors(uint8_t pin) : ds(pin) 
      { 
        this->pin = pin; 
        discovered = false;
      }; 

      void discoverSensors(); 

      void readSensors() 
      {  
        if (!this->discovered)
        {
          discoverSensors(); 
        }

        bool discovered = true; 
        for (auto &[sA, s] : this->m)
        {
          if (!s.hasAddress())
          {
            discovered = false; 
          }
          else if (!s.isTempValid() || s.isTempExpired())
          {
            s.readTemp(ds); 
          }
        }

        this->discovered = discovered; 
      }; 

      float getTempC(uint8_t a) { return has(a) && get(a).isTempValid() ? get(a).getTempC() : 0; };
      String formatTempC(uint8_t a) { return has(a) && get(a).isTempValid() ? String(get(a).getTempC()) : String("-"); };
      TemperatureSensor& get(uint8_t shortAddress) { return m[shortAddress]; };
      bool has(uint8_t shortAddress) { return m.count(shortAddress); };
      void add(std::string name, uint8_t shortAddress) 
      {  
        TemperatureSensor sensor(name, shortAddress);
        add(sensor);
      }; 
      void add(TemperatureSensor& sensor) 
      {  
        m.insert({sensor.getShortAddress(), sensor}); 
        if (!sensor.hasAddress())
        {
          discovered = false; 
        }
      }; 

      bool ready() 
      { 
        for (auto &[sA, s] : this->m)
        {
          if (!s.isTempValid())
          {
            return false; 
          }
        }

        return true; 
      }; 

      void printSensors(); 
  };
}