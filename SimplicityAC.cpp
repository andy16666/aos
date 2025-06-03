#include "SimplicityAC.h"

using namespace AOS; 
using AOS::SimplicityAC; 
using AOS::SimplicityACResponse; 

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
    SimplicityACResponse response(url, httpClient.GET()); 

    if (response.isOK())
    {
      response.setPayload(httpClient.getString());
      Serial.printf("%s: %d\r\n", url, response.getCode());
      response.parse(*this);
    }
    else if (response.isInternalServerError())
    {
      response.setPayload(httpClient.getString());
      Serial.printf("%s: %d: %s\r\n", url, response.getCode(), response.getPayload().c_str());
    }
    else 
    {
      Serial.printf("HTTP ERROR: %s: %d\r\n", url, response.getCode());
    }
    httpClient.end(); 
    return response.isOK(); 
  }
  else 
  {
    Serial.printf("%s: %s\r\n", url, "Failed to initialize HTTP client.");
    return false;  
  }
}

void SimplicityACResponse::parse(SimplicityAC& acData)
{
  JSONVar acOutput = JSON.parse(payload);

  if (JSON.typeof(acOutput) == "undefined")
  {
    Serial.println("Parsing input failed!");
  }
  else
  {
    if (acOutput.hasOwnProperty("evapTempC"))
    {
      acData.evapTempC = atoff(acOutput["evapTempC"]);
    }

    if (acOutput.hasOwnProperty("outletTempC"))
    {
      acData.outletTempC = atoff(acOutput["outletTempC"]);
    }

    if (acOutput.hasOwnProperty("command"))
    {
      String command = acOutput["command"]; 
      acData.command = static_cast<ac_cmd_t>(command.c_str()[0]);
    }

    if (acOutput.hasOwnProperty("state"))
    {
      String state = acOutput["state"]; 
      acData.state = static_cast<ac_state_t>(state.c_str()[0]);
    }

    if (acOutput.hasOwnProperty("fanState"))
    {
      String fanState = acOutput["fanState"]; 
      acData.fanState = static_cast<fan_state_t>(fanState.c_str()[0]);
    }

    if (acOutput.hasOwnProperty("compressorState"))
    {
      String compressorState = acOutput["compressorState"]; 
      acData.compressorState = static_cast<compressor_state_t>(compressorState.c_str()[0]);
    }

    acData.updateTimeMs = millis(); 
  }
}
