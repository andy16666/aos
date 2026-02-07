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
#pragma once
#include <Arduino.h> 
#include <ArduinoJson.h>
#include <map>

namespace AOS
{
  class PWMFans; 
  class PWMFan; 

  class PWMFan
  {
    private: 
      pin_size_t pin; 
      bool  activeLow; 
      float state; 
      float command; 
      String jsonName; 
      float offBelow; 
      float minStart; 
      float maxSpeed; 

    public: 
      PWMFan() {}; 
      PWMFan(const char *jsonName, pin_size_t pin, float offBelow, float minStart, float maxSpeed) 
        : PWMFan(jsonName, pin, false, offBelow, minStart, maxSpeed) { }; 

      PWMFan(const char *jsonName, pin_size_t pin, bool  activeLow, float offBelow, float minStart, float maxSpeed)
      {
        this->offBelow = offBelow; 
        this->minStart = minStart; 
        this->maxSpeed = maxSpeed; 
        this->pin = pin; 
        this->activeLow = activeLow; 
        this->jsonName = String(jsonName); 
        this->state = 0; 
        this->command = 0; 
        pinMode(pin, OUTPUT); 
        analogWrite(pin, activeLow ? 255 : 0); 
      }; 

      pin_size_t getPin() { return pin; }; 
      float getState() { return state; }; 
      float getCommand() { return command; }; 

      void setCommand(float command) 
      {
        if (this->command == command)
          return; 

        this->command = command < offBelow 
          ? 0.0 
          : (command > maxSpeed
            ? maxSpeed 
            : command); 
      };
      
      bool isFanOff()
      {
        return state < offBelow; 
      };

      void execute()
      {  
        if (command == state)
        {
          return; 
        }
        else if (command < offBelow)
        {
          state = 0.0; 
        }
        else if (isFanOff())
        {
          state = command >= minStart ? command : minStart; 
        }
        else 
        {
          state = command; 
        }

        int pinValue = (int)(((state >= offBelow ? state : 0.0)/100.0) * 255.0); 
        
        analogWrite(pin, activeLow ? 255-pinValue : pinValue); 
      };

      void addTo(const char * key, JsonDocument& document)
      {
        document[key][jsonName.c_str()]["command"] = command; 
        document[key][jsonName.c_str()]["state"] = state; 
        document[key][jsonName.c_str()]["pin"] = pin; 
      }; 
  };

  class PWMFans 
  {
    private:
      std::map<pin_size_t, PWMFan> m;
      float offBelow;
      float minStart;

    public: 
      PWMFans() {}; 
      PWMFans(float offBelow, float minStart)
      {
        this->offBelow = offBelow;
        this->minStart = minStart;
      }; 

      void add(PWMFan& fan) { m.insert({fan.getPin(), fan}); };
      
      void add(const char *jsonName, pin_size_t pin); 

      void add(const char *jsonName, pin_size_t pin, bool activeLow); 

      bool has(pin_size_t pin) { return m.count(pin); };
      
      PWMFan& get(pin_size_t pin) { return m[pin]; };

      PWMFan& operator[](pin_size_t pin) 
      {
        return get(pin); 
      }; 
      
      float getCommand(pin_size_t pin) { return has(pin) ? get(pin).getCommand() : 0; }; 

      void setCommand(pin_size_t pin, float command) 
      { 
        if (has(pin))
        {
           get(pin).setCommand(command); 
        }
      }; 

      void execute() 
      {
        for (auto &[k, f] : this->m)
        {
          f.execute(); 
        }
      }; 

      void addTo(const char* key, JsonDocument& document) 
      {
        for (auto &[k, f] : this->m)
        {
          f.addTo(key, document); 
        }
      }; 
  }; 
}