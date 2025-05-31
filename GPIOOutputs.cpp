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
#include <map> 
#include <string>
#include "GPIOOutputs.h"

using namespace std;
using namespace AOS; 

using AOS::GPIOOutput; 
using AOS::GPIOOutputs; 

GPIOOutput::GPIOOutput(std::string name, pin_size_t pin, bool activeHigh)
{
  this->name = name; 
  this->pin = pin; 
  this->activeHigh = activeHigh; 
  this->command = false; 
  this->state = false; 
  pinMode(pin, OUTPUT); 
}

pin_size_t GPIOOutput::getPin() const
{ 
  return this->pin; 
}

void GPIOOutput::setCommand(bool command) 
{ 
  this->command = command; 
}

bool GPIOOutput::getCommand() const
{ 
  return this->command; 
}

bool GPIOOutput::getState() const
{ 
  return this->state; 
}

void GPIOOutput::execute() 
{ 
  if (!this->isSet())
  {
    digitalWrite(this->getPin(), (this->activeHigh ? this->getCommand() : !this->getCommand()) ? HIGH : LOW); 
    this->state = this->getCommand();  
  }
}

bool GPIOOutput::isSet() const
{ 
  return this->getCommand() == this->getState(); 
}

std::string GPIOOutput::getName() const
{ 
  return this->name; 
} 

bool GPIOOutputs::setNext()
{
  for (auto& [pN, p] : this->m)
  {
    if(!p.isSet())
    {
      p.execute(); 
      return true; 
    }
  }

  return false; 
}

void GPIOOutputs::setAll()
{
  for (auto &[pN, p] : this->m)
  {
    if(!p.isSet())
    {
      p.execute(); 
    }
  }
}

void GPIOOutputs::add(GPIOOutput& o)
{
  this->m.insert({o.getPin(), o}); 
}

GPIOOutput& GPIOOutputs::get(const pin_size_t p)
{
  return this->m[p];
}

bool GPIOOutputs::has(const pin_size_t p)
{
  return this->m.count(p);
}

std::string GPIOOutputs::getName() const 
{ 
  return this->name; 
}
