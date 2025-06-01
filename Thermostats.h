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
      unsigned long lastUpdatedSetpointMs; 
      unsigned long lastUpdatedCurrentTempMs; 
      unsigned long lastUpdatedCommandMs; 

    public: 
      Thermostat() {}; 
      Thermostat(std::string name);
      void setSetPointC(float setPointC);
      void setCurrentTemperatureC(float currentTemperatureC);
      void setCommand(bool command);
      
      std::string getName() const;
      float getSetPointC() const;
      float getCurrentTemperatureC() const;
      bool getCommand() const;

      bool isCurrent(); 
      bool heatCalledFor();
      bool coolingCalledFor();
      float getMagnitude(); 

      unsigned long getLastUpdatedSetpointMs() const; 
      unsigned long getLastUpdatedCurrentTempMs() const; 
      unsigned long getLastUpdatedCommandMs() const; 
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
      String toString();
  };
}