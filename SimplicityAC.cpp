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

SimplicityACResponse EMPTY_RESPONSE("", -1, millis()); 
SimplicityACResponse& responseRef = EMPTY_RESPONSE;

void SimplicityAC::parse()
{ 
  Serial.println("SimplicityAC::parse();"); 
  if(responseRef.isOK() && responseRef.hasPayload())
  {
    responseRef.parse(this);  
  }
}

bool SimplicityAC::hasUnparsedResponse()
{
  return responseRef.getTime() != EMPTY_RESPONSE.getTime(); 
}

bool SimplicityAC::execute(String params)
{
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

  if (httpClient.begin(url)) {  // HTTP
    responseRef = SimplicityACResponse(url, httpClient.GET(), millis()); 

    if (responseRef.isOK())
    {
      responseRef.setPayload(httpClient.getString());
      Serial.printf("%s: %d\r\n", url, responseRef.getCode());
      httpClient.end(); 
      return true; 
    }
    else if (responseRef.isInternalServerError())
    {
      responseRef.setPayload(httpClient.getString());
      Serial.printf("%s: %d: %s\r\n", url, responseRef.getCode(), responseRef.getPayload().c_str());
      httpClient.end(); 
      return false; 
    }
    else 
    {
      Serial.printf("HTTP ERROR: %s: %d\r\n", url, responseRef.getCode());
      httpClient.end(); 
      return false; 
    }
    
  }
  else 
  {
    Serial.printf("%s: %s\r\n", url, "Failed to initialize HTTP client.");
    httpClient.end(); 
    return false;  
  }
}

void SimplicityACResponse::parse(SimplicityAC* acData)
{
  JSONVar acOutput = JSON.parse(getPayload().c_str());

  if (JSON.typeof(acOutput) == "undefined")
  {
    Serial.println("Parsing input failed!");
    return; 
  }
  else
  {
    Serial.println("Parsing "); 
    if (acOutput.hasOwnProperty("evapTempC"))
    {
      acData->evapTempC = atoff(acOutput["evapTempC"]);
    }

    Serial.println("Parsing "); 
    if (acOutput.hasOwnProperty("outletTempC"))
    {
      acData->outletTempC = atoff(acOutput["outletTempC"]);
    }

    Serial.println("Parsing "); 
    if (acOutput.hasOwnProperty("command"))
    {
      String command = acOutput["command"]; 
      acData->command = static_cast<ac_cmd_t>(command.c_str()[0]);
    }

    Serial.println("Parsing "); 
    if (acOutput.hasOwnProperty("state"))
    {
      String state = acOutput["state"]; 
      acData->state = static_cast<ac_state_t>(state.c_str()[0]);
    }

    Serial.println("Parsing "); 
    if (acOutput.hasOwnProperty("fanState"))
    {
      String fanState = acOutput["fanState"]; 
      acData->fanState = static_cast<fan_state_t>(fanState.c_str()[0]);
    }

    Serial.println("Parsing "); 
    if (acOutput.hasOwnProperty("compressorState"))
    {
      String compressorState = acOutput["compressorState"]; 
      acData->compressorState = static_cast<compressor_state_t>((int)(compressorState.c_str()[0]));
    }

    Serial.println("Parsed ac payload"); 
    acData->setSet(true); 
    acData->updateTimeMs = getTime(); 
  }
}
