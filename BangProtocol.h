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

#include <Arduino.h> 
#include <RPi_Pico_TimerInterrupt.h>
#include <aos.h>
#include <map>

#define BC_RX_BUFFER_SIZE 4096

typedef enum {
  BC_STATE_NOTINIT, 
  BC_STATE_READY, 
  BC_STATE_RX_START, 
  BC_STATE_RX_CHAR, 
  BC_STATE_RX_ACK,
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
extern std::map<uint8_t, std::function<bool(repeating_timer *)>> BangProtocol_callbackTable;

namespace AOS 
{
  class BangChannel
  {
    private: 
      uint8_t txPin; 
      uint8_t txEnablePin; 
      uint8_t rxPin;
      uint8_t rxEnablePin; 
      uint8_t irq; 
      RPI_PICO_TimerInterrupt timer;
      bang_channel_state_t state; 
      char rxBuffer[BC_RX_BUFFER_SIZE];
      char *rxBufferPtr;
      char rxChar; 
      int rxBit;
      char txBuffer[BC_RX_BUFFER_SIZE];
      char *txBufferPtr;
      char txChar; 
      int txBit;
      bool txReady;
      unsigned long timeInState = 0; 
    
      void showbits( char x )
      {
          char bits[9]; 
          bits[8] = 0; 
          int i=0;
          for (i = 0; i < 8; i++)
          {
            bits[i] = (x & (0x01 << i) ? '1' : '0');
          }
          Serial.printf("%s\r\n", bits);
      }

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
        goTo(BC_STATE_READY); 
      };

      void txAbort()
      {
        rewindTXBuffer(); 
        digitalWrite(txEnablePin, LOW); 
        digitalWrite(txPin, LOW); 
        goTo(BC_STATE_READY); 
      };

      void txDone()
      {
        clearTXBuffer();
        digitalWrite(txEnablePin, LOW); 
        digitalWrite(txPin, LOW); 
        txReady = false;  
        goTo(BC_STATE_READY); 
      };


      void goTo(bang_channel_state_t state)
      {
        timeInState = 0; 
        this->state = state; 
      };

    public: 
      static inline const float TIMER_FREQ_HZ = 5555.555; 
      
      BangChannel(
        uint8_t txPin,
        uint8_t txEnablePin,
        uint8_t rxPin,
        uint8_t rxEnablePin, 
        uint8_t irq
      ) : timer(irq) {
        this->txPin = txPin; 
        this->rxPin = rxPin; 
        this->txEnablePin = txEnablePin; 
        this->rxEnablePin = rxEnablePin; 
        this->irq = irq; 
        this->state = BC_STATE_NOTINIT; 
        this->rxBufferPtr = rxBuffer; 
        this->rxChar = 0; 
        this->rxBit = 0; 
        this->txBufferPtr = txBuffer; 
        this->txChar = 0; 
        this->txBit = 0; 
        this->txReady = false; 
      }; 

      bool timerHandler(struct repeating_timer *t)
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


        // RX: 
        //  0  READY     rxEnable,!rx : !txEnable,  tx     : 
        //  1  RX_START  rxEnable, rx : !txEnable,  tx     : 
        //  2  RX_CHAR   rxEnable,*rx : !txEnable,  tx -> receive message until !rxEnable, 
        //     RX_ABORT  rxEnable,*rx : !txEnable,  !tx    : Abort transmission
        //  3  RX_CHAR; !rxEnable,*rx : 
        // 
        //     RX_ACK;  !txEnable,  tx ack, !txEnable,!tx err
        // 10,01, 11,01, 1x,01, 
        
        // TX: 
        //  0 !rxEnable,!rx,txReady,txMutex :  txEnable,!tx
        //  1 !rxEnable, rx                 :  txEnable, tx
        //  2 !rxEnable, rx                 :  txEnable,*tx 
        //  3 !rxEnable, rx                 : !txEnable,*tx -> done
        //  e !rxEnable, !rx, txEnable      : Abort
        //  a !rxEnable, rx, !txEnable      : Sccess 
        //            Send : !txEnable, tx ack, !txEnable, !tx err
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
                  digitalWrite(txPin, rxPostambleSend[timeInState]); 
                  if (timeInState == rxPostambleLength - 1)
                  {
                    digitalWrite(txPin, rxPostambleSend[timeInState]); 
                    goTo(BC_STATE_RX_DONE); 
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
                else if (timeInState < rxPreambleLength)
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
              if (timeInState < rxPostambleLength + 1)
              {
                if (rx == rxPostambleSend[timeInState - 1])
                {
                  if (timeInState < rxPostambleLength)
                  {
                    digitalWrite(txPin, rxPostambleExpect[timeInState]); 
                  }
                }
                else if (timeInState < rxPostambleLength)
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
          EPRINTLN("BC_STATE_RX_ABORT");
        }

        if (state == BC_STATE_RX_ERR)
        {
          EPRINTLN("BC_STATE_RX_ERR");
        }

        if (state == BC_STATE_TX_ERR)
        {
          EPRINTLN("BC_STATE_TX_ERR");
        }

        timeInState++; 

        return true; 
      };

      bool rxReady()
      {
        return state == BC_STATE_RX_DONE; 
      };

      String getString()
      {
        if (state == BC_STATE_RX_DONE)
        {
          String data = String(rxBuffer); 
          rxBufferPtr = rxBuffer; 
          return data; 
        }
        else 
        {
          return String("");
        }
      };

      bool putString(String data)
      {
        if (txReady)
        {
          return false; 
        }
        else
        {
          for (int i = 0; i < data.length() && i < (BC_RX_BUFFER_SIZE - 1); i++)
          {
            txBuffer[i] = data.c_str()[i]; 
            txBuffer[i+1] = 0; 
          }
          txReady = true; 
          
          return true; 
        }
      };

      void init(); 
  };

  
}