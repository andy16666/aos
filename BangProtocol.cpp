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
#include "BangProtocol.h"

using namespace AOS; 
using AOS::BangChannel; 

static std::list<std::function<void()>> BangProtocol_callbackTable; 
static RPI_PICO_TimerInterrupt* BangProtocol_timer = 0;

bool BangProtocol_timerCallback(repeating_timer *t) 
{
  for (auto f : BangProtocol_callbackTable)
  {
    f(); 
  }

  return true; 
}

void BangChannel::begin() 
{
  if (initialized)
  {
    EPRINT("bang channel already initialized.");
  }
  else 
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

    auto callbackFunction = std::mem_fn(&AOS::BangChannel::executeStateMachine);
    auto callback = std::bind(callbackFunction, this); 
    std::function<void()> callbackWrapper = callback;  
    BangProtocol_callbackTable.push_back(callbackWrapper); 
    
    if (!BangProtocol_timer)
    {
      BangProtocol_timer = new RPI_PICO_TimerInterrupt(BC_IRQ); 
      if (BangProtocol_timer->attachInterrupt(BC_BAUD_RATE, BangProtocol_timerCallback))
      {
        DPRINTLN("Starting timer OK, millis() = " + String(millis()));
      }
      else
      {
        EPRINTLN("Can't set timer. Select another freq. or timer");
      }
    }

    goTo(BC_STATE_READY);
    initialized = true; 
  } 
}

/**
 * ISR handler!!! No mutexes or allocation. 
 */
void BangChannel::executeStateMachine()
{
  int previousState = state; 
  PinStatus rx = digitalRead(rxPin); 
  PinStatus rxEnable = digitalRead(rxEnablePin); 
  
  PinStatus rxPreambleSend[] = { HIGH, LOW, HIGH, LOW, HIGH }; 
  PinStatus rxPreambleExpect[] = { LOW, HIGH, LOW, HIGH, LOW }; 
  int rxPreambleLength = 5;

  PinStatus rxPostambleSend[] = { LOW, HIGH, LOW, HIGH }; 
  PinStatus rxPostambleExpect[] = { HIGH, LOW, HIGH, LOW }; 
  int rxPostambleLength = 4;

  switch(state)
  {
      case BC_STATE_NOTINIT: break; 
      case BC_STATE_READY: 
      {
        if (rxEnable && rxPreambleExpect[0] == rx)
        {
          digitalWrite(txPin, rxPreambleSend[0]); 
          clearRXBuffer(); 
          goTo(BC_STATE_RX_START); 
        }
        else if (!rxEnable && txReady) 
        { 
          txInProgress = true;
          digitalWrite(txEnablePin, HIGH); 
          digitalWrite(txPin, rxPreambleExpect[0]);
          goTo(BC_STATE_TX_START); 
        }
        
        break; 
      }
      case BC_STATE_RX_START: 
      {
        if (!rxEnable)
        {
          rxFinish(); 
        }
        if (timeInState < rxPreambleLength)
        {
          if (rx == rxPreambleExpect[timeInState])
          {
            digitalWrite(txPin, rxPreambleSend[timeInState]); 
            if (timeInState == rxPreambleLength - 1)
            {
              goTo(BC_STATE_RX_CHAR);  
            }
          }
          else 
          {
            digitalWrite(txPin, !rxPreambleSend[timeInState]); 
            rxFinish(); 
          }
        }
        break; 
      }
      case BC_STATE_RX_CHAR: 
      {
        if (!rxEnable)
        {
          // The bit number should be 0, otherwise we missed data (or received extra bits).
          if (rx == rxPostambleExpect[0])
          {
            digitalWrite(txPin, rxPostambleSend[0]); 
            goTo(BC_STATE_RX_ACK); 
          }
          else 
          {
            digitalWrite(txPin, !rxPostambleSend[0]); 
            goTo(BC_STATE_RX_ERR); 
          }
        }
        else 
        {
          rxNextBit(rx); 
          int bufferFull = rxBufferPtr >= (rxBuffer + (BC_RX_BUFFER_SIZE - 1)); 

          if (bufferFull)
          {
            digitalWrite(txPin, !rxPostambleSend[0]); 
            goTo(BC_STATE_RX_ABORT); 
          }
        }
        break; 
      }
      case BC_STATE_RX_ABORT: 
      {
        digitalWrite(txPin, rxEnable ? rxPostambleSend[0] : !rxPostambleSend[0]); 
        rxFinish();
        break; 
      }
      case BC_STATE_RX_ACK: 
      {
        if (rxEnable)
        {
          rxFinish(); 
        }
        if (timeInState < rxPostambleLength)
        {
          if (rx == rxPostambleExpect[timeInState])
          {
            // The TX end will block while this is set so we can do the checksum and copy. 
            digitalWrite(txPin, rxPostambleSend[timeInState]); 
            if (timeInState == rxPostambleLength - 1)
            {
              goTo(BC_STATE_RX_CHECKSUM); 
            }
          }
          else 
          {
            digitalWrite(txPin, !rxPostambleSend[timeInState]);
            goTo(BC_STATE_RX_ERR);
          }  
        }
        break; 
      }
      case BC_STATE_RX_CHECKSUM: 
      {
        int bufferLength = strlen(rxBuffer); 
        if (bufferLength > 1)
        {
          bufferLength -= 1; 
          uint8_t computedChecksum = computeParityByte(rxBuffer, bufferLength); 
          uint8_t sentChecksum = rxBuffer[bufferLength];
          rxBuffer[bufferLength] = 0; 

          if (sentChecksum != computedChecksum)
          {
            DPRINTF("Checksum failed: %x vs. %x (%d)\r\n", sentChecksum, computedChecksum, bufferLength); 
            goTo(BC_STATE_RX_ERR); 
          }
          else 
          {
            goTo(BC_STATE_RX_DONE); 
          }
        }
        else 
        {
          DPRINTLN("Checksum failed: not enough characters received."); 
          goTo(BC_STATE_RX_ERR);
        }
        break; 
      }
      case BC_STATE_RX_ERR: 
      {
        goTo(BC_STATE_RX_ABORT); 
        break; 
      }
      case BC_STATE_RX_DONE: 
      {
        // Wait here until the buffer is reset. 
        if (rxBufferPtr == rxBuffer && !rxEnable)
        {
          rxFinish(); 
        }
        break; 
      }
      case BC_STATE_TX_START: 
      {
        if (timeInState < rxPreambleLength)
        {
          if (rx == rxPreambleSend[timeInState - 1])
          {
            digitalWrite(txPin, rxPreambleExpect[timeInState]); 
          
            if (timeInState == rxPreambleLength - 1)
            {
              goTo(BC_STATE_TX_CHAR); 
            }
          }
          else
          {
            digitalWrite(txPin, !rxPreambleExpect[timeInState]); 
            txAbort(); 
          }
        }
        
        break; 
      }
      case BC_STATE_TX_CHAR: 
      {
        if (rx == rxPreambleSend[rxPreambleLength - 1]) // Client listening. 
        {
          bool hasMore = txNextBit(); 

          if (rxEnable) // Bail for incoming message. Retransmit later. 
          {
            txAbort(); 
          }
          else if (!hasMore)
          {
            goTo(BC_STATE_TX_END); 
          }
        }
        else  // Client aborted
        {
          goTo(BC_STATE_TX_ERR); 
        }
      
        break; 
      }
      case BC_STATE_TX_END:
      {
        digitalWrite(txEnablePin, LOW);
        digitalWrite(txPin, rxPostambleExpect[0]); 
        goTo(BC_STATE_TX_ACK); 
        break; 
      }
      case BC_STATE_TX_ACK: 
      {
        if (timeInState < rxPostambleLength)
        {
          if (rx == rxPostambleSend[timeInState - 1])
          {
            digitalWrite(txPin, rxPostambleExpect[timeInState]); 
          }
          else
          {
            digitalWrite(txPin, !rxPostambleExpect[timeInState]); 
            goTo(BC_STATE_TX_ERR);
          }
        }
        // Wait on the last bit of the postamble for the client to forward its data. 
        else if (rx != rxPostambleSend[rxPostambleLength - 1])
        {
          digitalWrite(txPin, LOW); 
          goTo(BC_STATE_TX_DONE); 
        }

        break; 
      }
      case BC_STATE_TX_ERR: 
      {
        // Any error and we'll keep the text to send again. 
        txAbort(); 
        break; 
      }
      case BC_STATE_TX_DONE: 
      { 
        txDone(); 
        break; 
      }
  }
  
  if (state == BC_STATE_RX_ABORT)
  {
    DPRINTLN("BC_STATE_RX_ABORT");
  }

  if (state == BC_STATE_RX_ERR)
  {
    DPRINTLN("BC_STATE_RX_ERR");
  }

  if (state == BC_STATE_TX_ERR)
  {
    DPRINTLN("BC_STATE_TX_ERR");
  }

  timeInState++; 
}

bool BangChannel::put(String data)
{
  if (!initialized)
  {
    EPRINT("bang channel not initialized."); 
    return false; 
  }

  LOCK(txPutLock); 
  lockTxBuffer(); 

  {
    int i;
    for (i = 0; i < data.length() && i < (BC_RX_BUFFER_SIZE - (1 + 1)); i++)
    {
      txBuffer[i] = data.c_str()[i]; 
      txBuffer[i + 1] = 0; 
    }

    txBuffer[i] = computeParityByte(txBuffer, i);
    txBuffer[i + 1] = 0; 
  }

  // Allows TX to proceed. 
  txReady = true; 
  UNLOCK(txPutLock);  

  return true;
}

bool BangChannel::ready()
{
  if (!initialized)
  {
    EPRINT("bang channel not initialized."); 
    return false; 
  }

  return state == BC_STATE_RX_DONE; 
}

String BangChannel::get()
{
  if (!initialized)
  {
    EPRINT("bang channel not initialized.");
    return String("");
  }

  else if (state == BC_STATE_RX_DONE)
  {
    LOCK(rxGetLock); 
    String data = String(rxBuffer); 
    rxBufferPtr = rxBuffer; 
    UNLOCK(rxGetLock); 
    return data;
  }
  
  return String("");
}