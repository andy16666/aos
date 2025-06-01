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

#include <Arduino.h>
#include <cstddef>
#include <string.h>
#include <string>
#include <map>
#include <stdio.h>
#include <float.h>
#include "Thermostats.h"
#include "util.h"

#define THERMOSTAT_FORMAT_STRING "{ " \
    "\"name\":\"%s\", " \
    "\"setTempC\":\"%f\", " \
    "\"setTempAge\":\"%s\", " \
    "\"currentTempC\":\"%f\", " \
    "\"currentTempAge\":\"%s\", " \
    "\"command\":\"%d\", " \
    "\"commandAge\":\"%s\" " \
    "}"

#define COMMAND_EXPIRY_TIME_MS 600000

using namespace AOS; 
using namespace std;

using AOS::Thermostat; 
using AOS::Thermostats; 

Thermostat::Thermostat(std::string name)
{ 
  this->name = name; 
  command = false; 
  setPointC = 20.0; 
  currentTemperatureC = 20.0; 
  lastUpdatedCommandMs = millis(); 
  lastUpdatedCurrentTempMs = millis(); 
  lastUpdatedSetpointMs = millis(); 
} 

unsigned long Thermostat::getLastUpdatedCurrentTempMs() const
{
  return this->lastUpdatedCurrentTempMs; 
}

unsigned long Thermostat::getLastUpdatedSetpointMs() const
{
  return this->lastUpdatedSetpointMs;  
}

unsigned long Thermostat::getLastUpdatedCommandMs() const
{
  return this->lastUpdatedCommandMs; 
}

void Thermostat::setSetPointC(float setPointC) 
{
  this->setPointC = setPointC;
  this->lastUpdatedSetpointMs = millis(); 
}

void Thermostat::setCurrentTemperatureC(float currentTemperatureC) 
{
  this->currentTemperatureC = currentTemperatureC;
  this->lastUpdatedCurrentTempMs = millis(); 
}

void Thermostat::setCommand(bool command) 
{
  this->command = command;
  this->lastUpdatedCommandMs = millis(); 
}

float Thermostat::getSetPointC() const
{
  return this->setPointC;
}

float Thermostat::getCurrentTemperatureC() const
{
  return this->currentTemperatureC;
}

bool Thermostat::getCommand() const
{
  return this->command;
}

std::string Thermostat::getName() const
{
  return this->name;
}

bool Thermostat::heatCalledFor() 
{
  return this->command && getSetPointC() > getCurrentTemperatureC();
}

bool Thermostat::coolingCalledFor() 
{
  return isCurrent() && getCommand() && getSetPointC() < getCurrentTemperatureC() && getMagnitude() > 0.25;
}

float Thermostat::getMagnitude()
{
  return isCurrent() ? fabs(getCurrentTemperatureC() - getSetPointC()) : 0; 
}

bool Thermostat::isCurrent() 
{
  unsigned long timeMs = millis(); 
  unsigned long commandAge = timeMs - getLastUpdatedCommandMs(); 
  unsigned long setpointAge = timeMs - getLastUpdatedSetpointMs(); 
  unsigned long currentTempAge = timeMs - getLastUpdatedCurrentTempMs(); 

  return commandAge < COMMAND_EXPIRY_TIME_MS && setpointAge < COMMAND_EXPIRY_TIME_MS && currentTempAge < COMMAND_EXPIRY_TIME_MS; 
}

void Thermostats::add(std::string name)
{
  Thermostat thermostat(name);
  m.insert({name, thermostat});
}

bool Thermostats::contains(std::string name) 
{
  return this->m.count(name);
}

bool Thermostats::contains(char *name)
{
  std::string nameStr(name);
  return this->m.count(nameStr);
}

Thermostat& Thermostats::get(std::string name) 
{
  return this->m[name];
}

Thermostat& Thermostats::get(char * name) 
{ 
  std::string nameStr(name);
  return this->m[nameStr];
}

String Thermostats::toString() 
{
  char buffer[1024];

  sprintf(buffer, "\n   [\n");
  int i = 0;
  for (const auto &[n, t] : this->m) 
  {
    if (i++ != 0)
      sprintf(buffer + strlen(buffer), ",\n");
    
    sprintf(buffer + strlen(buffer), "    ");

    unsigned long timeMs = millis(); 
    sprintf(
        buffer + strlen(buffer), 
        THERMOSTAT_FORMAT_STRING,
        t.getName().c_str(),
        t.getSetPointC(),
        msToHumanReadableTime(timeMs - t.getLastUpdatedSetpointMs()).c_str(),
        t.getCurrentTemperatureC(),
        msToHumanReadableTime(timeMs - t.getLastUpdatedCurrentTempMs()).c_str(),
        t.getCommand() ? 1 : 0,
        msToHumanReadableTime(timeMs - t.getLastUpdatedCommandMs()).c_str()
      );
  }
  sprintf(buffer + strlen(buffer), "\n   ]");

  return String(buffer);
}