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
   A core RTOS library built on Arduino and FreeRTOS for HVAC controllers
   for the Pi Pico W. This library uses core 0 for communication and core 1 
   for controlling and monitoring. 
   
    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */
#ifndef AOS_H
#define AOS_H
#include <FreeRTOS.h> 
#include <semphr.h> 
#include <Arduino.h>
#include <OneWire.h>
#include <ArduinoJson.h>
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

#define TEMP_SENSOR_PIN 2
#define CORE_0_ACT   18
#define CORE_1_ACT   19
#define PING_INTERVAL_MS 15000
#define MAX_CONSECUTIVE_FAILED_PINGS 5
#define AOS_WATCHDOG_TIMEOUT_MS 30000
#define HTTP_RESPONSE_BUFFER_SIZE 4096
#define HTML_BUFFER_SIZE 32*1024

//#define DPRINT_ON
#ifdef DPRINT_ON
    #define DPRINTF(format, ...) Serial.printf(format, __VA_ARGS__)
    #define DPRINTLN(format) Serial.println(format)
    #define DPRINT(format) Serial.print(format)
#else
    #define DPRINTF(format, ...) 
    #define DPRINTLN(format) 
    #define DPRINT(format) 
#endif

#define EPRINTF(format, ...) Serial.print("ERROR: "); Serial.printf(format, __VA_ARGS__)
#define EPRINTLN(format) Serial.print("ERROR: "); Serial.println(format)
#define EPRINT(format) Serial.print("ERROR: "); Serial.print(format)

#define WPRINTF(format, ...) Serial.print("WARNING: "); Serial.printf(format, __VA_ARGS__)
#define WPRINTLN(format) Serial.print("WARNING: "); Serial.println(format)
#define WPRINT(format) Serial.print("WARNING: "); Serial.print(format)

#define MUTEX_T SemaphoreHandle_t
#define LOCK(mutex) while(xSemaphoreTakeRecursive(mutex, portMAX_DELAY) != pdTRUE)
#define TRYLOCK(mutex) (xSemaphoreTakeRecursive(mutex, portTICK_PERIOD_MS * 100) == pdTRUE)
#define UNLOCK(mutex) xSemaphoreGiveRecursive(mutex); 

extern "C" {
#include <threadkernel.h>
};

extern AOS::TemperatureSensors TEMPERATURES; 
extern CPU cpu; 
extern volatile char* httpResponseString; 

extern volatile unsigned long            initialize             ;
extern volatile unsigned long            powerUpTime            ;
extern volatile unsigned long            numRebootsDisconnected ;
extern volatile unsigned long            timeBaseMs             ;
extern volatile unsigned long            numRebootsPingFailed   ;

volatile inline unsigned long startupTime = millis();
volatile inline unsigned long connectTime = millis();

extern MUTEX_T networkMutex;

extern threadkernel_t* CORE_0_KERNEL; 
extern threadkernel_t* CORE_1_KERNEL;

//extern const char* hostname; 

static void task_testPing(); 
static void task_mdnsUpdate(); 
static void wifi_connect();
static bool is_wifi_connected();
static void handleHttpNotFound(); 

static void task_testWiFiConnection(); 
static void task_handleHttpClient(); 
static void task_updateHttpResponse();

static void task_readTemperatures();
static void task_core0ActOn(); 
static void task_core1ActOn(); 
static void task_core0ActOff(); 
static void task_core1ActOff(); 
static void afterProcess(process_t *process);
static void readIndexHTML();

void reboot(); 
const char* generateHostname(); 

void aosInitialize(); 
void aosSetup(); 
void aosSetup1();
void populateHttpResponse(JsonDocument &document);  
bool handleHttpArg(String argName, String arg); 

uint32_t getTotalHeap();
uint32_t getFreeHeap(); 

#endif