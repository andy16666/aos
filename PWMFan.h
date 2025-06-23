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
  class PWMFan
  {
    private: 
      pin_size_t pin; 
      pin_size_t onPin; 
      float state; 
      float command; 
      String jsonName; 

    public: 
      static inline const float MAX_SPEED = 100.0; 
      static inline const float MIN_START_SPEED = 20.0; 
      static inline const float OFF_BELOW = 14.0; 

      PWMFan(const char *jsonName, pin_size_t onPin, pin_size_t pin)
      {
        this->pin = pin; 
        this->onPin = onPin; 
        this->jsonName = String(jsonName); 
        this->state = 0; 
        this->command = 0; 
        pinMode(pin, OUTPUT); 
        pinMode(onPin, OUTPUT);
      }; 

      float getState() { return state; }; 
      float getCommand() { return command; }; 

      void setCommand(float command) 
      {
        if (this->command == command)
          return; 

        this->command = command < OFF_BELOW 
          ? 0.0 
          : (command > MAX_SPEED
            ? MAX_SPEED 
            : command); 
      };
      
      bool isFanOff()
      {
          return state < OFF_BELOW; 
      };

      void execute()
      {  
        if (command == state)
        {
          return; 
        }
        else if (command < OFF_BELOW)
        {
          state = 0.0; 
        }
        else if (isFanOff())
        {
          state = command >= MIN_START_SPEED ? command : MIN_START_SPEED; 
        }
        else 
        {
          state = command; 
        }
        
        digitalWrite(onPin, state >= OFF_BELOW ? HIGH : LOW); 
        analogWrite(pin, state >= OFF_BELOW ? state : 0.0); 
      };

      void addTo(const char * key, JsonDocument& document)
      {
        document[key][jsonName.c_str()]["command"] = command; 
        document[key][jsonName.c_str()]["state"] = state; 
        document[key][jsonName.c_str()]["onPin"] = onPin; 
        document[key][jsonName.c_str()]["pin"] = pin; 
      }; 
  };
}