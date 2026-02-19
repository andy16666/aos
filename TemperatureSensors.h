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
#include <sys/_types.h>
#include <sys/_stdint.h>
#include <sys/_intsup.h>
#include <cstdlib>
#include <map>
#include <string>
#include <streambuf>
#include <cstddef>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// #define NDEBUG
#include <cassert>

#include <DS18B20.h>
#include <ArduinoJson.h>

using namespace std;

namespace AOS
{
  const unsigned int ADDRESS_LENGTH = 8; 
  const float MAX_TEMP_C = 120; 
  const float MIN_TEMP_C = -50;  
  const float INVALID_TEMP  = FLT_MIN;
  const unsigned long TEMP_EXPIRY_TIME_MS = 30000; 
  const unsigned long TEMP_VALID_TIME_MS = 300 * 1000; 

  class TemperatureSensors;
  class TemperatureSensor;
  class TemperatureSensor
  {
    private:
      uint8_t shortAddress;
      uint8_t address[ADDRESS_LENGTH];
      String name;
      String jsonName;
      float tempC;
      float previousTempC; 
      bool read;
      unsigned long lastReadMs;
      unsigned long previousReadMs; 
      unsigned int tempErrors; 

    public:
      static inline const unsigned long READ_INTERVAL_MS = 1000; 

      TemperatureSensor() 
      {
        tempErrors = 0; 
        previousTempC = 0; 
        read = false; 
        lastReadMs = 0; 
        previousReadMs = 0; 
      }; 
      
      TemperatureSensor(const char* name, uint8_t shortAddress) : TemperatureSensor()
      {
        this->name = String(name); 
        this->jsonName = String(name) + String(shortAddress); 
        this->shortAddress = shortAddress; 
        for (int i = 0; i < ADDRESS_LENGTH; i++) { this->address[i] = 0; }
      }; 

      TemperatureSensor(const char* name, const char* jsonName, uint8_t shortAddress) : TemperatureSensor()
      {
        this->name = String(name); 
        this->jsonName = String(jsonName); 
        this->shortAddress = shortAddress; 
        for (int i = 0; i < ADDRESS_LENGTH; i++) { this->address[i] = 0; }
      }; 
      
      TemperatureSensor(uint8_t address[ADDRESS_LENGTH]) : TemperatureSensor()
      {
        setAddress(address); 
        this->name = "Discovered"; 
        this->jsonName = String(this->name) + String(shortAddress); 
      }; 

      String getName()
      {
        return name;
      };

      unsigned int getTempErrors() { return tempErrors; }; 
      bool readTemp(DS18B20& ds); 
      bool isRead() { return read; };
      bool hasAddress() { return address[ADDRESS_LENGTH-1] == shortAddress; };
      uint8_t* getAddress() { return address; };
      uint8_t getShortAddress() { return shortAddress; }
      
      bool isTempExpired() 
      {
        return !read || lastReadMs < (millis() - TEMP_EXPIRY_TIME_MS);  
      };

      double getAgeSeconds()
      {
        return (millis() - lastReadMs) / 1E3;
      }

      double getRateOfChangeDegreesPerSecond()
      {
        // Rollover or init
        if (lastReadMs <= previousReadMs) 
          return 0; 

        double intervalSeconds = (lastReadMs - previousReadMs) / 1E3; 
        double deltat = tempC - previousTempC; 

        return deltat / intervalSeconds; 
      }

      float getTempC() { return tempC; }; 
      
      bool isTempValid()
      {
        return read && isTempCValid(tempC);
      };

      void setName(String name)
      {
        this->name = name; 
      };

      void setAddress(uint8_t address[ADDRESS_LENGTH]) 
      {  
        Serial.printf("Setting address of %d to %s\r\n", shortAddress, addressToString(address).c_str()); 
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

      String formatTempC()
      {
        return isTempValid() ? String(getTempC()) : String("-"); 
      }

      void addTo(const char* key, JsonDocument& document) 
      {
        document[key][jsonName] = formatTempC(); 
      }

      void addTo(JsonDocument& document) 
      {
        document[jsonName] = formatTempC(); 
      }

      static String addressToString(uint8_t *address)
      {
        String addressString = ""; 
        for (int i = 0; i < ADDRESS_LENGTH; i++)
        {
          if (i > 0) addressString += ":";
          addressString += '0'+address[i];
        }
        return addressString;
      };

      static bool isTempCValid(float tempC) 
      {
        return tempC < MAX_TEMP_C 
          && tempC > MIN_TEMP_C; 
      };
  };

  class TemperatureSensors
  {
    private:
      unsigned int tempErrors; 
      std::map<uint8_t, TemperatureSensor> m;
      uint8_t pin; 
      DS18B20 ds; 
      bool discovered; 

      void discoverSensors();
      bool areDiscovered()
      {
        if (!this->discovered)
        {
          for (auto &[sA, s] : this->m)
          {
            if (!s.hasAddress())
            {
              return false; 
            }
          }

          // If there are no sensors registered, report false to force discoverty
          return m.size() > 0; 
        }
        else 
        {
          return true; 
        }
      }

    public: 
      TemperatureSensors(uint8_t pin) : ds(pin) 
      { 
        this->pin = pin; 
        discovered = false;
        ds.setResolution(RES_12_BIT); 
      }; 

      void forceDiscovery()
      {  
        discoverSensors(); 
      }; 

      void readSensors() 
      {  
        if (!areDiscovered())
        {
          forceDiscovery(); 
        }
        else 
        {
          this->discovered = true; 
        }

        for (auto &[sA, s] : this->m)
        {
          if (s.hasAddress())
          {
            s.readTemp(ds); 
          }
        }

        ds.doConversion(); 
      }; 

      unsigned int getTempErrors()
      {
        unsigned int tempErrors = 0; 
        for (auto &[sA, s] : this->m)
        {
          tempErrors += s.getTempErrors(); 
        }

        return tempErrors; 
      }; 
      
      bool isTempValid(uint8_t a) { return has(a) && get(a).isTempValid(); };
      
      float getTempC(uint8_t a) { return has(a) && get(a).isTempValid() ? get(a).getTempC() : 0; };
      
      String formatTempC(uint8_t a) { return has(a) ? get(a).formatTempC() : String("-"); };
      
      TemperatureSensor& get(uint8_t shortAddress) { return m[shortAddress]; };
      
      float operator[](uint8_t a) 
      {
        return getTempC(a); 
      }; 

      bool has(uint8_t shortAddress) { return m.count(shortAddress); };
      
      void add(const char * name, const char * jsonName, uint8_t shortAddress) 
      {  
        TemperatureSensor sensor(name, jsonName, shortAddress);
        add(sensor);
      }; 

      void add(const char * name, uint8_t shortAddress) 
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
          if (!s.hasAddress())
          {
            return false; 
          }
        }

        return true; 
      }; 

      void printSensors(); 

      void addTo(JsonDocument& document) 
      {
        for (auto &[sA, s] : this->m)
        {
          s.addTo(document); 
        }
      }

      void addTo(const char* key, JsonDocument& document) 
      {
        for (auto &[sA, s] : this->m)
        {
          s.addTo(key, document); 
        }
      }
  };
}