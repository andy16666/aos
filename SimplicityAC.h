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
#include <sys/_intsup.h>
#include <cstdlib>
#include <map>
#include <string>
#include <HTTPClient.h>

#define AC_DATA_EXPIRY_TIME_MS 30 * 1000
#define AC_OUTLET_IDLE_MIN_TEMP_C 15
#define AC_EVAP_IDLE_MIN_TEMP_C 15

// External command to the unit
typedef enum {
  CMD_AC_OFF = '-',
  CMD_AC_COOL_HIGH = 'C',
  CMD_AC_COOL_MED  = 'M',
  CMD_AC_COOL_LOW  = 'L',
  CMD_AC_KILL = 'K',
  CMD_AC_FAN = 'F'
} ac_cmd_t;

// A/C state is changed based on the external command and the current state.
typedef enum {
  AC_POWER_OFF = '-',
  AC_COOL_HIGH = 'H',
  AC_COOL_MED  = 'M',
  AC_COOL_LOW  = 'L',
  AC_STANDBY_HIGH = 'h',
  AC_STANDBY_MED  = 'm',
  AC_STANDBY_LOW  = 'l',
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
      }

      bool isCommand(ac_cmd_t command) { return isSet() && this->command == command; };
      bool isState(ac_state_t state) { return isSet() && this->state == state; }; 

      bool isCooling()
      {
        return isSet() && !isCommand(CMD_AC_KILL) && !isCommand(CMD_AC_OFF) && !isCommand(CMD_AC_FAN);
      };

      bool isSet() { return updateTimeMs; };
      bool isExpired() { return !isSet() || updateTimeMs < millis() - AC_DATA_EXPIRY_TIME_MS; };

      bool isOutletCold()
      {
        return isSet() && outletTempC < AC_OUTLET_IDLE_MIN_TEMP_C; 
      };

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
      bool execute() { return execute(String()); }; 
  }; 

  class SimplicityACResponse
  {
    private: 
      int code; 
      String payload; 
      String url; 

    public: 
      SimplicityACResponse(String url, int code) 
      {
        this->code = code; 
        payload = String(); 
        this->url = url; 
      }; 

      void setPayload(String payload) 
      {
        payload.replace("\n", "\r\n");
        this->payload = payload; 
      }; 

      void parse(SimplicityAC& ac); 
      String getPayload() { return payload; }; 
      int getCode() { return code; }; 
      bool isOK() { return code == HTTP_CODE_OK; }
      bool isInternalServerError() { return code == HTTP_CODE_OK; }
  }; 
}