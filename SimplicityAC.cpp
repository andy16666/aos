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
#include "SimplicityAC.h"
#include <FreeRTOS.h>
#include <semphr.h> 
#include "aos.h"



#define DEBUG_PRINT Serial.println

using namespace AOS; 
using AOS::SimplicityAC; 
using AOS::SimplicityACResponse; 

volatile SimplicityACResponse* responseRef = 0; 
HTTPClient httpClient; 

SemaphoreHandle_t acDataMutex = xSemaphoreCreateRecursiveMutex();

/**
 * Core 1: Parse the response from the air conditioner pointed to by responseRef; 
 */  
void SimplicityAC::parse()
{   
  DPRINTLN("Enter SimplicityAC::parse()"); 
  if (!responseRef)
  {
    DPRINTLN("Leave SimplicityAC::parse(): no responseRef"); 
    return; 
  }

  DPRINTLN("SimplicityAC::parse(): acquire lock"); 
  if(xSemaphoreTakeRecursive( acDataMutex, portTICK_PERIOD_MS * 1 ) != pdTRUE)
  {
    DPRINTLN("Leave SimplicityAC::parse(): lock timeout"); 
    return;
  }

  if (!responseRef)
  {
    DPRINTLN("Leave SimplicityAC::parse(): no responseRef under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }

  DPRINTLN("SimplicityAC::parse(): grab response"); 
  SimplicityACResponse *response = (SimplicityACResponse*)responseRef; 
  if (!response)
  {
    DPRINTLN("Leave SimplicityAC::parse(): no response under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }
  //response->print(); 
  responseRef = nullptr; 
  xSemaphoreGiveRecursive( acDataMutex );

  DPRINTLN("SimplicityAC::parse(): lock dropped"); 

  if (!response)
  {
    DPRINTLN("Leave SimplicityAC::parse(): no response under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }

  DPRINTLN("SimplicityAC::parse(): checking response:"); 
  //response->print();
  if(response->isOK() && response->hasPayload())
  {
    DPRINTLN("SimplicityAC::parse(): initiating parse"); 
    response->parse(this);  
  }

  DPRINTLN("SimplicityAC::parse(): delete response"); 
  delete response; 
  DPRINTLN("Leave SimplicityAC::parse(): response deleted"); 
}

bool SimplicityAC::execute(String params)
{
  DPRINTLN("Enter SimplicityAC::execute(String params)"); 
  if (responseRef)
  {
    DPRINTLN("Leave SimplicityAC::execute(String params): responseRef null"); 
    return true; 
  }
  
  DPRINTLN("SimplicityAC::execute(String params): instantiate HTTPClient"); 

  
  //httpClient.setTimeout(10000);

  char url[256]; 

  DPRINTLN("SimplicityAC::execute(String params): form URL"); 

  if (params.length() > 0)
  {
    DPRINTLN("SimplicityAC::execute(String params): has params"); 
    sprintf(url, "http://%s.local/%s", this->hostname.c_str(), params.c_str());
  }
  else 
  {
    DPRINTLN("SimplicityAC::execute(String params): no params"); 
    sprintf(url, "http://%s.local/", this->hostname.c_str());
  }

  bool success = false; 

  DPRINTLN("\rHTTP connecting");

  if (httpClient.begin(url)) 
  { 
    DPRINTF("\rHTTP Connected: %d\r\n", code);
    SimplicityACResponse *response = new SimplicityACResponse(url, httpClient.GET(), millis()); 
    //response->print(); 

    if (response->isOK())
    {
      response->setPayload(httpClient.getString());
      //response->print(); 
      success = true; 
    }
    else if (response->isInternalServerError())
    {
      response->setPayload(httpClient.getString());
      response->print(); 
    }
    else 
    {
      //response->print(); 
    }

    while (xSemaphoreTakeRecursive( acDataMutex, portMAX_DELAY ) != pdTRUE);
    responseRef = (volatile SimplicityACResponse*)response; 
    xSemaphoreGiveRecursive( acDataMutex );
    httpClient.end();
    MDNS.update(); 
  }
  else 
  {
    DPRINTF("%s: %s\r\n", url, "Failed to initialize HTTP client.");
  }

  DPRINTF("Leave SimplicityAC::execute(String params) %d\r\n", success); 

  return success; 
}

void SimplicityACResponse::parse(SimplicityAC* acData)
{
  JsonDocument acOutput; 
  deserializeJson(acOutput, getPayload().c_str());

  String command = acOutput["command"]; 
  String state = acOutput["state"]; 
  String fanState = acOutput["fanState"]; 
  String compressorState = acOutput["compressorState"]; 

  acData->evapTempC = atoff(acOutput["evapTempC"]);
  acData->outletTempC = atoff(acOutput["outletTempC"]);
  acData->command = static_cast<ac_cmd_t>(command.c_str()[0]);
  acData->state = static_cast<ac_state_t>(state.c_str()[0]); 
  acData->fanState = static_cast<fan_state_t>(fanState.c_str()[0]);
  acData->compressorState = static_cast<compressor_state_t>((int)(compressorState.c_str()[0]));
  acData->setSet(true); 
  acData->updateTimeMs = getTime(); 
}
