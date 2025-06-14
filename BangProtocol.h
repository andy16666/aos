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
   A simple 4 wire half duplex communication 
   protocol using GPIO. 
   
    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */
#pragma once
#include <Arduino.h> 
#include <Arduino_CRC32.h>
#include <RPi_Pico_TimerInterrupt.h>
#include <aos.h>
#include <map>
#include <list>
#include <util.h> 

#define BC_RX_BUFFER_SIZE 4096
#define BC_IRQ 1
// Raw bits per second is roughly this. 
#define BC_BAUD_RATE 5555.555

typedef enum {
  BC_STATE_NOTINIT, 
  BC_STATE_READY, 
  BC_STATE_RX_START, 
  BC_STATE_RX_CHAR, 
  BC_STATE_RX_ACK,
  BC_STATE_RX_CHECKSUM,
  BC_STATE_RX_ABORT, 
  BC_STATE_RX_ERR, 
  BC_STATE_RX_DONE, 
  BC_STATE_TX_START, 
  BC_STATE_TX_CHAR,
  BC_STATE_TX_END, 
  BC_STATE_TX_ACK, 
  BC_STATE_TX_ERR, 
  BC_STATE_TX_DONE
} bang_channel_state_t; 

extern bool BangProtocol_timerCallback(repeating_timer *); 

namespace AOS 
{ 
  class BangChannel
  {
    private: 
      uint8_t txPin; 
      uint8_t txEnablePin; 
      uint8_t rxPin;
      uint8_t rxEnablePin; 

      volatile bang_channel_state_t state; 

      char rxBuffer[BC_RX_BUFFER_SIZE];
      char *rxBufferPtr;
      char rxChar; 
      int rxBit;

      char txBuffer[BC_RX_BUFFER_SIZE];
      char *txBufferPtr;
      char txChar; 
      int txBit;

      // txReady and txInProgress comprise the primary protection mechanism for 
      // txBuffer between put() and the ISR. txPutLock only protects multiple 
      // callers to put() from one another. 
      volatile bool txReady;
      volatile bool txInProgress; 
      bool initialized; 
      MUTEX_T txPutLock; 

      // The number of ISR calls we've been in the current state, including this one. 
      unsigned long timeInState = 0; 

      // Protects the RX buffer against multiple callers to get(). 
      MUTEX_T rxGetLock; 

      void clearRXBuffer()
      {
        rxBufferPtr = rxBuffer; 
        *rxBufferPtr = 0; 
        rxChar = 0; 
        rxBit = 0; 
      };

      void rewindTXBuffer()
      {
        txBufferPtr = txBuffer; 
        txBit = 0; 
        txChar = 0; 
      };

      void clearTXBuffer()
      {
        txBufferPtr = txBuffer; 
        txBuffer[0] = 0; 
        txBit = 0; 
        txChar = 0; 
      };

      void rxNextBit(int rx)
      {
        rxChar = rxChar | ((rx ? 0x01 : 0x00) << (rxBit++)); 
        if (rxBit >= 8)
        {
          *(rxBufferPtr++) = rxChar; 
          // Move the null forward after each character is read. 
          *rxBufferPtr = 0; 
          rxBit = 0; 
          rxChar = 0; 
        }
      };

      bool txNextBit()
      {
        if (!(*(txBufferPtr)))
          return false; 

        txChar = *(txBufferPtr); 
        digitalWrite(txPin, txChar & (0x01 << txBit++)? HIGH : LOW);

        if (txBit < 8)
        {
          return true; 
        }
        else
        {
          int bufferEnd = txBufferPtr >= (txBuffer + (BC_RX_BUFFER_SIZE - 1));
          txBit = 0; 
          txBufferPtr++; 
          if (bufferEnd || !(*(txBufferPtr)))
            return false; 
          return true; 
        }
      };

      void  rxFinish()
      {
        clearRXBuffer(); 
        digitalWrite(txEnablePin, LOW); 
        digitalWrite(txPin, LOW);
        txInProgress = false;  
        goTo(BC_STATE_READY); 
      };

      void txAbort()
      {
        rewindTXBuffer(); 
        digitalWrite(txEnablePin, LOW); 
        digitalWrite(txPin, LOW); 
        txInProgress = false; 
        goTo(BC_STATE_READY); 
      };

      void txDone()
      {
        clearTXBuffer();
        digitalWrite(txEnablePin, LOW); 
        digitalWrite(txPin, LOW); 
        txInProgress = false; 
        txReady = false;  
        goTo(BC_STATE_READY); 
      };

      void lockTxBuffer()
      {
        // Prevent a TX from starting. 
        txReady = false; 

        // txInProgress is set after entering the BC_STATE_READY where txReady 
        // is checked. If txReady was checked before setting it above but txInProgress 
        // has not yet been set, it will be by the time the ISR exits this state. Therefore 
        // we block on both. 
        while (state == BC_STATE_READY || txInProgress);
      }; 

      /**
       * Change to the given state and reset timeInState. 
       */
      void goTo(bang_channel_state_t state)
      {
        timeInState = 0; 
        this->state = state; 
      };

    public:
      BangChannel(
        uint8_t txPin,
        uint8_t txEnablePin,
        uint8_t rxPin,
        uint8_t rxEnablePin
      ) {
        this->txPin = txPin; 
        this->rxPin = rxPin; 
        this->txEnablePin = txEnablePin; 
        this->rxEnablePin = rxEnablePin; 
        this->state = BC_STATE_NOTINIT; 
        this->rxBufferPtr = rxBuffer; 
        this->rxChar = 0; 
        this->rxBit = 0; 

        this->txBufferPtr = txBuffer; 
        this->txChar = 0; 
        this->txBit = 0; 

        this->txReady = false; 
        this->initialized = false; 
      }; 

      void begin(); 

      void executeStateMachine(); 

      bool ready();

      String get();

      bool put(String data); 
  };
}