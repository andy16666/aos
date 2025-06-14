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
HTTPClient httpClient; 

MUTEX_T acDataMutex = xSemaphoreCreateRecursiveMutex();

/**
 * Core 1: Parse the response from the air conditioner pointed to by responseRef; 
 */  
void SimplicityAC::parse()
{   
  if (!responseRef)
  {
    return; 
  }

  if(!TRYLOCK(acDataMutex))
  {
    return;
  }

  // Recheck under lock. 
  if (!responseRef)
  {
    UNLOCK( acDataMutex );
    return; 
  }

  SimplicityACResponse *response = (SimplicityACResponse*)responseRef; 
  responseRef = nullptr; 
  UNLOCK( acDataMutex );

  if(response->isOK() && response->hasPayload())
  {
    response->parse(this);  
  }

  delete response; 
}

bool SimplicityAC::execute(String params)
{
  if (responseRef)
  {
    return true; 
  }
  
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
      success = true; 
    }
    else if (response->isInternalServerError())
    {
      response->setPayload(httpClient.getString());
      response->print(); 
    }
    
    LOCK( acDataMutex ); 
    responseRef = (volatile SimplicityACResponse*)response; 
    response = 0; 
    UNLOCK( acDataMutex );

    httpClient.end();
  }
  else 
  {
    DPRINTF("%s: %s\r\n", url, "Failed to initialize HTTP client.");
  }

  delay(20); 
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
