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
   
    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */
#ifndef AOS_H
#define AOS_H
#include <FreeRTOS.h> 
#include <semphr.h> 
#include <Arduino.h>
#include <OneWire.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <CPU.h>
#include <LEAmDNS.h>

#include <sys/_intsup.h>
#include <cstdlib>
#include <stdint.h> 
#include <float.h>
#include <math.h>
#include <string.h>



#include "TemperatureSensors.h"
#include "Ping.h"

#define AC_HOSTNAME "ac1"
#define TEMP_SENSOR_PIN 2
#define CORE_0_ACT   18
#define CORE_1_ACT   19
#define PING_INTERVAL_MS 30000
#define MAX_CONSECUTIVE_FAILED_PINGS 5
#define AOS_WATCHDOG_TIMEOUT_MS 60000

extern "C" {
#include <threadkernel.h>
};

extern AOS::TemperatureSensors TEMPERATURES; 
extern CPU cpu; 
extern String& httpResponseString; 

extern volatile unsigned long            initialize             ;
extern volatile unsigned long            powerUpTime            ;
extern volatile unsigned long            numRebootsDisconnected ;
extern volatile unsigned long            timeBaseMs             ;
extern volatile unsigned long            tempErrors             ;
extern volatile unsigned long            numRebootsPingFailed   ;

volatile inline unsigned long startupTime = millis();
volatile inline unsigned long connectTime = millis();

extern SemaphoreHandle_t networkMutex;

//bool networkMutexTryAcquire();
//void networkMutexRelease();

extern threadkernel_t* CORE_0_KERNEL; 
extern threadkernel_t* CORE_1_KERNEL;

extern const char* HOSTNAME; 

void reboot(); 
const char* generateHostname(); 

void aosInitialize(); 
void aosSetup(); 
void aosSetup1();
void populateHttpResponse(JSONVar &document);  

void task_core0ActOn(); 
void task_core1ActOn(); 
void task_core0ActOff(); 
void task_core1ActOff(); 

void task_testPing(); 
void task_readTemperatures();
void task_updateHttpResponse();

void task_mdnsUpdate(); 
void wifi_connect();
bool is_wifi_connected();
void task_testWiFiConnection(); 

uint32_t getTotalHeap();
uint32_t getFreeHeap(); 




#endif