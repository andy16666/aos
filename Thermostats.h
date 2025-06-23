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
    A container for thermostat data from homekit

    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */

#pragma once
#include <sys/_intsup.h>
#include <cstdlib>
#include <map>
#include <string>
#include <cstddef>
#include <string.h>
#include <stdio.h>
#include <float.h>

#include <Arduino.h>
#include <ArduinoJson.h>

#include "util.h"

#define COMMAND_EXPIRY_TIME_MS 600000

using namespace std;

namespace AOS
{
  class Thermostats; 
  class Thermostat;

  class Thermostat 
  {
    private: 
      std::string name; 
      float setPointC; 
      float currentTemperatureC; 
      bool command; 
      bool heatOn; 
      unsigned long lastUpdatedSetpointMs; 
      unsigned long lastUpdatedCurrentTempMs; 
      unsigned long lastUpdatedCommandMs; 
      unsigned long lastUpdatedHeatOnMs; 

    public: 
      Thermostat() {}; 
      Thermostat(std::string name);
      void setSetPointC(float setPointC);
      void setCurrentTemperatureC(float currentTemperatureC);
      void setCommand(bool command);
      void setHeatOn(bool heatOn); 
      
      std::string getName() const;
      float getSetPointC() const;
      float getCurrentTemperatureC() const;
      bool getCommand() const;
      bool getHeatOn() const; 

      bool isCurrent(); 
      bool heatCalledFor();
      bool coolingCalledFor();
      float getMagnitude(); 

      void addTo(const char* key, JsonDocument& document) 
      {
        document[key][name.c_str()]["setPointC"] = setPointC; 
        document[key][name.c_str()]["setPointAge"] = msToHumanReadableTime(millis() - lastUpdatedSetpointMs); 
        document[key][name.c_str()]["currentTempC"] = currentTemperatureC; 
        document[key][name.c_str()]["currentAge"] = msToHumanReadableTime(millis() - lastUpdatedCurrentTempMs); 
        document[key][name.c_str()]["command"] = command; 
        document[key][name.c_str()]["commandAge"] = msToHumanReadableTime(millis() - lastUpdatedCommandMs); 
        document[key][name.c_str()]["heatOn"] = heatOn; 
        document[key][name.c_str()]["heatOnAge"] = msToHumanReadableTime(millis() - lastUpdatedHeatOnMs); 
      }

      unsigned long getLastUpdatedSetpointMs() const; 
      unsigned long getLastUpdatedCurrentTempMs() const; 
      unsigned long getLastUpdatedCommandMs() const; 
      unsigned long getLastUpdatedHeatOnMs() const; 
      
  };

  class Thermostats 
  {
    private:
      std::map<string, Thermostat> m; 

    public: 
      Thermostats() {}; 

      void add(std::string name);
      bool contains(std::string name);
      bool contains(char *name);
      Thermostat& get(std::string name);
      Thermostat& get(char * name);

      bool isCurrent() 
      { 
        for (auto &[sA, s] : this->m)
        {
          if (!s.isCurrent())
          {
            return false; 
          }
        }

        return true; 
      }; 

      bool coolingCalledFor() 
      { 
        for (auto &[sA, s] : this->m)
        {
          if (s.coolingCalledFor())
          {
            return true; 
          }
        }

        return false; 
      }; 

      bool heatCalledFor() 
      { 
        for (auto &[sA, s] : this->m)
        {
          if (s.heatCalledFor())
          {
            return true; 
          }
        }

        return false; 
      }; 

      double getMaxCoolingMagnitude() 
      { 
        if (!coolingCalledFor())
          return 0; 
        
        float maxMagnitude = 0; 
        for (auto &[sA, s] : this->m)
        {
          if (s.coolingCalledFor() && s.getMagnitude() > maxMagnitude)
          {
            maxMagnitude = s.getMagnitude(); 
          }
        }

        return maxMagnitude; 
      }; 

      void addTo(const char* key, JsonDocument& document) 
      {
        for (auto &[sA, t] : this->m)
        {
          t.addTo(key, document); 
        }
      }
  };
}