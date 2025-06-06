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

using namespace AOS; 
using AOS::SimplicityAC; 
using AOS::SimplicityACResponse; 

volatile SimplicityACResponse* responseRef = 0; 

SemaphoreHandle_t acDataMutex = xSemaphoreCreateRecursiveMutex();

void SimplicityAC::parse()
{ 
  Serial.println("SimplicityAC::parse();"); 
  
  if (!responseRef)
  {
    Serial.println("SimplicityAC::parse() returning before lock"); 
    return; 
  }

  Serial.println("SimplicityAC::parse() getting lock"); 
  if(xSemaphoreTakeRecursive( acDataMutex, portTICK_PERIOD_MS * 1 ) != pdTRUE)
  {
    Serial.println("SimplicityAC::parse() returning failed to lock"); 
    return;
  }

  if (!responseRef)
  {
    Serial.println("SimplicityAC::parse() returning under lock"); 
    xSemaphoreGiveRecursive( acDataMutex );
    return; 
  }

  SimplicityACResponse *response = (SimplicityACResponse*)responseRef; 
  responseRef = nullptr; 
  xSemaphoreGiveRecursive( acDataMutex );

  Serial.println("SimplicityAC::parse() checking response"); 
  if(response->isOK() && response->hasPayload())
  {
    Serial.println("SimplicityAC::parse() parsing"); 
    response->parse(this);  
  }

  Serial.println("SimplicityAC::parse() deleting"); 
  delete response; 
}

bool SimplicityAC::execute(String params)
{
  if (responseRef != nullptr)
  {
    return true; 
  }

  HTTPClient httpClient; 
  httpClient.setTimeout(5000); 

  char url[256]; 

  if (params.length() > 0)
  {
    sprintf(url, "http://%s.local/%s", this->hostname.c_str(), params.c_str());
  }
  else 
  {
    sprintf(url, "http://%s.local/", this->hostname.c_str());
  }

  bool success = false; 

  if (httpClient.begin(url)) 
  { 
    SimplicityACResponse *response = new SimplicityACResponse(url, httpClient.GET(), millis()); 

    if (response->isOK())
    {
      response->setPayload(httpClient.getString());
      Serial.printf("%s: %d %s\r\n", url, response->getCode(), response->getPayload().c_str());
      success = true; 
    }
    else if (response->isInternalServerError())
    {
      response->setPayload(httpClient.getString());
      Serial.printf("%s: %d: %s\r\n", url, response->getCode(), response->getPayload().c_str());
    }
    else 
    {
      Serial.printf("HTTP ERROR: %s: %d\r\n", url, response->getCode());
    }

    while (xSemaphoreTakeRecursive( acDataMutex, portTICK_PERIOD_MS * 1 ) != pdTRUE ) 
    {
      Serial.println("SimplicityAC::execute(String params) waiting"); 
    }

    responseRef = (volatile SimplicityACResponse*)response; 

    xSemaphoreGiveRecursive( acDataMutex );
  }
  else 
  {
    Serial.printf("%s: %s\r\n", url, "Failed to initialize HTTP client.");
  }

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

  Serial.println("Parsed ac payload"); 
  acData->setSet(true); 
  acData->updateTimeMs = getTime(); 
}
