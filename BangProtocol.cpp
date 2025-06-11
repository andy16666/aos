#include "BangProtocol.h"

using namespace AOS; 
using AOS::BangChannel; 

std::map<uint8_t, std::function<bool(repeating_timer *)>> BangProtocol_callbackTable; 

bool BangProtocol_timerCallback(repeating_timer *t) 
{
  for (auto &[i, f] : BangProtocol_callbackTable)
  {
    f(t); 
  }

  return true; 
}

void BangChannel::init() 
{
  if (state != BC_STATE_NOTINIT)
  {
    EPRINTLN("Error: channel already initialized"); 
  }

  pinMode(txPin, OUTPUT); 
  pinMode(rxPin, INPUT); 
  pinMode(txEnablePin, OUTPUT); 
  pinMode(rxEnablePin, INPUT); 

  digitalWrite(txEnablePin, LOW); 
  digitalWrite(txPin, LOW); 

  clearRXBuffer(); 
  clearTXBuffer(); 

  auto callbackFunction = std::mem_fn(&AOS::BangChannel::timerHandler);
  auto callback = std::bind(callbackFunction, this, std::placeholders::_1); 
  std::function<bool(repeating_timer *)> callbackWrapper = callback;  
  BangProtocol_callbackTable.insert({irq, callbackWrapper}); 
  
  if (timer.attachInterrupt(TIMER_FREQ_HZ, BangProtocol_timerCallback))
  {
    DPRINTLN("Starting timer OK, millis() = " + String(millis()));
  }
  else
  {
    EPRINTLN("Can't set timer. Select another freq. or timer");
  }

  goTo(BC_STATE_READY); 
}