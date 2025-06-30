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
    Simplifies operations controlling inductive loads using GPIO. 

    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */
#pragma once
#include <sys/_intsup.h>
#include <cstdlib>
#include <string>
#include <map>
#include <cstddef>

#include <Arduino.h>
#include <ArduinoJson.h>

using namespace std; 

namespace AOS
{
  class GPIOOutputs; 
  class GPIOOutput; 

  class GPIOOutput 
  { 
    private: 
      std::string name;
      bool activeHigh; 
      pin_size_t pin; 
      bool command; 
      bool state; 
      
    public: 
      GPIOOutput() {}; 
      GPIOOutput(std::string name, pin_size_t pin, bool activeHigh);

      void init() {
        pinMode(pin, OUTPUT); 
        execute(); 
      }

      void setCommand(bool command);
      void execute();
      void setAndExecute(bool command) { setCommand(command); execute(); };
      
      std::string getName() const;
      bool isSet() const;
      pin_size_t getPin() const;
      bool getCommand() const;
      bool getState() const;

      void addTo(const char* key, JsonDocument& document) 
      {
        document[key][name.c_str()] = command; 
      }

      void addTo(JsonDocument& document) 
      {
        document[name.c_str()] = command; 
      }
  };

  class GPIOOutputs
  {
    private: 
      std::string name;
      std::map<pin_size_t, GPIOOutput> m; 
      
    public: 
      GPIOOutputs() {}; 
      GPIOOutputs(std::string name) { this->name = name; };
      void add(GPIOOutput& outputPin);
      void add(std::string name, pin_size_t pin, bool activeHigh) 
      {
        GPIOOutput output(name, pin, activeHigh); 
        add(output); 
      };
      void setAll();
      bool setNext();

      void setCommand(const pin_size_t pin, bool command) 
      {
        if (has(pin))
          get(pin).setCommand(command); 
      };

      void setAndExecute(const pin_size_t pin, bool command) 
      {
        if (has(pin))
        {
          get(pin).setCommand(command); 
          get(pin).execute(); 
        }
      };

      bool isSet()
      {
        for (auto &[pN, p] : this->m)
        {
          if(!p.isSet())
          {
            return false; 
          }
        }

        return true; 
      }; 

      void init()
      {
        for (auto &[pN, p] : this->m)
        {
          if(!p.isSet())
          {
            p.init(); 
            delay(500);
          }
        }
      };

      void addTo(JsonDocument& document) 
      {
        for (auto &[sA, s] : this->m)
        {
          s.addTo(document); 
        }
      };

      void addTo(const char* key, JsonDocument& document) 
      {
        for (auto &[sA, s] : this->m)
        {
          s.addTo(key, document); 
        }
      };

      bool has(const pin_size_t p);
      GPIOOutput& get(const pin_size_t pin);
      GPIOOutput& operator[](const pin_size_t pin)
      {
        return get(pin); 
      }; 

      std::string getName() const;
      
  };
}