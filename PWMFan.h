#pragma once
#include <Arduino.h> 
#include <Arduino_JSON.h>
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
      static inline const float MIN_START_SPEED = 15.0; 
      static inline const float OFF_BELOW = 10.0; 

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

      void addTo(const char * key, JSONVar& document)
      {
        document[key][jsonName.c_str()]["command"] = command; 
        document[key][jsonName.c_str()]["state"] = state; 
        document[key][jsonName.c_str()]["onPin"] = onPin; 
        document[key][jsonName.c_str()]["pin"] = pin; 
      }; 
  };
}