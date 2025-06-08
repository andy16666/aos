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

using namespace AOS; 
using AOS::SimplicityAC; 
using AOS::SimplicityACResponse; 

volatile SimplicityACResponse* responseRef = 0; 

SemaphoreHandle_t acDataMutex = xSemaphoreCreateRecursiveMutex();

void SimplicityAC::parse()
{   
  //Serial.println("Enter SimplicityAC::parse()"); 
  if (!responseRef)
  {
    //Serial.println("Leave SimplicityAC::parse(): no responseRef"); 
    return; 
  }

  //Serial.println("SimplicityAC::parse(): acquire lock"); 
  if(xSemaphoreTakeRecursive( acDataMutex, portTICK_PERIOD_MS * 1 ) != pdTRUE)
  {
    //Serial.println("Leave SimplicityAC::parse(): lock timeout"); 
    return;
  }

  if (!responseRef)
  {
    //Serial.println("Leave SimplicityAC::parse(): no responseRef under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }

  //Serial.println("SimplicityAC::parse(): grab response"); 
  SimplicityACResponse *response = (SimplicityACResponse*)responseRef; 
  if (!response)
  {
    //Serial.println("Leave SimplicityAC::parse(): no response under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }
  //response->print(); 
  responseRef = nullptr; 
  xSemaphoreGiveRecursive( acDataMutex );

  //Serial.println("SimplicityAC::parse(): lock dropped"); 

  if (!response)
  {
    //Serial.println("Leave SimplicityAC::parse(): no response under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }

  //Serial.println("SimplicityAC::parse(): checking response:"); 
  //response->print();
  if(response->isOK() && response->hasPayload())
  {
    //Serial.println("SimplicityAC::parse(): initiating parse"); 
    response->parse(this);  
  }

  //Serial.println("SimplicityAC::parse(): delete response"); 
  delete response; 
  //Serial.println("Leave SimplicityAC::parse(): response deleted"); 
}

bool SimplicityAC::execute(String params)
{
  //Serial.println("Enter SimplicityAC::execute(String params)"); 
  if (responseRef)
  {
    //Serial.println("Leave SimplicityAC::execute(String params): responseRef null"); 
    return true; 
  }
  
  //Serial.println("SimplicityAC::execute(String params): instantiate HTTPClient"); 

  HTTPClient httpClient; 
  //httpClient.setTimeout(10000);

  char url[256]; 

  //Serial.println("SimplicityAC::execute(String params): form URL"); 

  if (params.length() > 0)
  {
    //Serial.println("SimplicityAC::execute(String params): has params"); 
    sprintf(url, "http://%s.local/%s", this->hostname.c_str(), params.c_str());
  }
  else 
  {
    //Serial.println("SimplicityAC::execute(String params): no params"); 
    sprintf(url, "http://%s.local/", this->hostname.c_str());
  }

  bool success = false; 

  //Serial.println("\rHTTP connecting");

  int code; 
  if (httpClient.begin(url) && (code = httpClient.GET()) > 0) 
  { 
    //Serial.printf("\rHTTP Connected: %d\r\n", code);
    SimplicityACResponse *response = new SimplicityACResponse(url, code, millis()); 
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
      response->print(); 
    }

    while (xSemaphoreTakeRecursive( acDataMutex, portMAX_DELAY ) != pdTRUE);
    responseRef = (volatile SimplicityACResponse*)response; 
    xSemaphoreGiveRecursive( acDataMutex );
    //httpClient.end();
  }
  else 
  {
    //Serial.printf("%s: %s\r\n", url, "Failed to initialize HTTP client.");
  }

  //Serial.printf("Leave SimplicityAC::execute(String params) %d\r\n", success); 

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
