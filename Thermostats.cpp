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


#include "Thermostats.h"

using namespace AOS; 
using namespace std;

using AOS::Thermostat; 
using AOS::Thermostats; 

Thermostat::Thermostat(std::string name)
{ 
  this->name = name; 
  command = false; 
  heatOn = false; 
  setPointC = 20.0; 
  currentTemperatureC = 20.0; 
  lastUpdatedCommandMs = millis(); 
  lastUpdatedCurrentTempMs = millis(); 
  lastUpdatedSetpointMs = millis(); 
  lastUpdatedHeatOnMs = millis(); 
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

unsigned long Thermostat::getLastUpdatedHeatOnMs() const
{
  return this->lastUpdatedHeatOnMs; 
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

void Thermostat::setHeatOn(bool heatOn) 
{
  this->heatOn = heatOn;
  this->lastUpdatedHeatOnMs = millis(); 
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

bool Thermostat::getHeatOn() const
{
  return this->heatOn;
}

std::string Thermostat::getName() const
{
  return this->name;
}

bool Thermostat::heatCalledFor() 
{
  return isCurrent() && getCommand() && (getSetPointC() > getCurrentTemperatureC() || getHeatOn());
}

bool Thermostat::coolingCalledFor() 
{
  return isCurrent() 
    && getCommand() 
    && !getHeatOn() 
    && getSetPointC() < getCurrentTemperatureC();
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