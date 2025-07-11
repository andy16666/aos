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
    Interfaces to a heavily modified air conditioner running 
    https://github.com/andy16666/simplicity-ac-firmware

    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */


#pragma once
#include <sys/_intsup.h>
#include <cstdlib>
#include <map>
#include <string>

#include <Arduino.h> 
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "util.h"

#include "TemperatureSensors.h"

#define AC_DATA_EXPIRY_TIME_MS 30 * 1000
#define AC_OUTLET_IDLE_MIN_TEMP_C 15
#define AC_EVAP_IDLE_MIN_TEMP_C 15

// External command to the unit
typedef enum {
  // Off does a soft power down, switching to fan high->medium->low
  CMD_AC_OFF = '-',
  // Each cool mode has different temperature ranges and fan speeds
  CMD_AC_COOL_HIGH = 'C',
  CMD_AC_COOL_MED  = 'M',
  CMD_AC_COOL_LOW  = 'L',
  // Kill stops the AC unit completely, disaling the compressure and powering down the fan
  CMD_AC_KILL = 'K',
  CMD_AC_FAN_HIGH = 'F',
  CMD_AC_FAN_MED = 'm',
  CMD_AC_FAN_LOW = 'l'
} ac_cmd_t;

// A/C state is changed based on the external command and the current state.
typedef enum {
  AC_POWER_OFF = '-',
  AC_COOL_HIGH = 'H',
  AC_COOL_MED  = 'M',
  AC_COOL_LOW  = 'L',
  AC_STATE_FAN_HIGH = 'h',
  AC_STATE_FAN_MED  = 'm',
  AC_STATE_FAN_LOW  = 'l'
} ac_state_t;

typedef enum {
  AC_FAN_OFF = '-',
  AC_FAN_LOW = 'L',
  AC_FAN_MED = 'M',
  AC_FAN_HIGH = 'H'
} fan_state_t;

typedef enum {
  AC_COMPRESSOR_OFF,
  AC_COMPRESSOR_ON
} compressor_state_t;



namespace AOS 
{
  class SimplicityAC
  {
    public: 
      float evapTempC; 
      float outletTempC; 
      ac_cmd_t command; 
      ac_state_t state; 
      fan_state_t fanState;
      compressor_state_t compressorState;  
      unsigned long updateTimeMs; 
      String hostname;
      bool set;
      
      SimplicityAC(String hostname)
      {
        this->evapTempC = 0; 
        this->outletTempC = 0; 
        this->updateTimeMs = 0; 
        this->command = CMD_AC_OFF; 
        this->fanState = AC_FAN_OFF; 
        this->compressorState = AC_COMPRESSOR_OFF; 
        this->state = AC_POWER_OFF; 
        this->hostname = hostname; 
        this->set = false; 
      }

      bool isCommand(ac_cmd_t command) { return isSet() && this->command == command; };
      bool isState(ac_state_t state) { return isSet() && this->state == state; }; 
      bool isFanState(fan_state_t fanState) { return isSet() && this->fanState == fanState; }; 

      void parse();

      bool isCooling()
      {
        return isCommand(CMD_AC_COOL_HIGH) || isCommand(CMD_AC_COOL_MED) || isCommand(CMD_AC_COOL_LOW);
      };

      ac_cmd_t getCommand()
      {
        return command; 
      }; 

      fan_state_t getFanState()
      {
        return fanState; 
      }

      void setSet(bool set) { this->set = set; }; 

      bool isSet() { return set; };
      bool isExpired() { return !isSet() || updateTimeMs < millis() - AC_DATA_EXPIRY_TIME_MS; };

      bool isOutletCold()
      {
        return isSet() && outletTempC < AC_OUTLET_IDLE_MIN_TEMP_C; 
      };

      bool isFanRunning() 
      {
        return isFanState(AC_FAN_HIGH) || isFanState(AC_FAN_MED) || isFanState(AC_FAN_LOW); 
      }

      float getEvapTempC()
      {
        return evapTempC; 
      }

      bool isEvapTempValid()
      { 
        return TemperatureSensor::isTempCValid(evapTempC); 
      }

      float getOutletTempC() 
      {
        return outletTempC; 
      }

      bool isOutletTempValid()
      { 
        return TemperatureSensor::isTempCValid(outletTempC); 
      }

      bool isEvapCold()
      {
        return isSet() && outletTempC < AC_EVAP_IDLE_MIN_TEMP_C; 
      };

      bool execute(ac_cmd_t command) 
      {
        char buffer[256]; 
        sprintf(buffer, "?cmd=%c", command);
        return execute(String(buffer));
      };

      bool execute(String params); 
      bool execute() { return execute(String("")); }; 

      void addTo(const char* key, JsonDocument& document) 
      {
        document[key]["evapTempC"] = evapTempC; 
        document[key]["outletTempC"] = outletTempC; 
        document[key]["lastUpdated"] = msToHumanReadableTime(millis() - updateTimeMs).c_str(); 
        document[key]["command"] = String((char)command).c_str(); 
        document[key]["state"] = String((char)state).c_str(); 
        document[key]["fanState"] = String((char)fanState).c_str(); 
        document[key]["compressorState"] = String((char)compressorState).c_str(); 
      }
  }; 

  class SimplicityACResponse
  {
    private: 
      int code; 
      String payload; 
      String url; 
      unsigned long time; 

    public: 
      SimplicityACResponse(String url, int code, unsigned long time) 
      {
        this->code = code; 
        payload = String(""); 
        this->url = String(url); 
        this->time = time; 
      }; 

      unsigned long getTime() { return time; }; 

      bool hasPayload()
      {
        return payload.length() > 0; 
      };

      void setPayload(String payload) 
      {
        payload.replace("\n", "\r\n");
        this->payload = String(payload); 
      }; 

      void parse(SimplicityAC* ac); 
      String getPayload() { return payload; }; 
      void setTime(unsigned long time) { this->time = time; }; 
      void setCode(int code) { this->code = code; }
      int getCode() { return code; }; 
      bool isOK() { return code == HTTP_CODE_OK; }
      bool isInternalServerError() { return code == HTTP_CODE_INTERNAL_SERVER_ERROR; }
      void print()
      {
        if (hasPayload())
        {
          Serial.printf("\r[%d]%s: %d: %s\r\n", time, url.c_str(), getCode(), getPayload().c_str());
        }
        else 
        {
          Serial.printf("\r[%d]%s: %d\r\n", time, url.c_str(), getCode());
        }
      }
  }; 
}