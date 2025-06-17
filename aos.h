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
#define CORE_0_ACT   LED_BUILTIN
#define CORE_1_ACT   19
#define PING_INTERVAL_MS 15000
#define MAX_CONSECUTIVE_FAILED_PINGS 5
#define AOS_WATCHDOG_TIMEOUT_MS 30000
#define HTTP_RESPONSE_BUFFER_SIZE 4096
#define HTML_BUFFER_SIZE 32*1024
#define TIMEZONE "AST4ADT,M3.2.0,M11.1.0"
#define STARTUP_GRACE_PERIOD_MS 200000
#define WATCHDOG_TIMER_REBOOT_MS 60000
#define WATCHDOG_TIMER_IRQ 1

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

#define NPRINTF(format, ...) Serial.print("NOTICE: "); Serial.printf(format, __VA_ARGS__)
#define NPRINTLN(format) Serial.print("NOTICE: "); Serial.println(format)
#define NPRINT(format) Serial.print("NOTICE: "); Serial.print(format)

#define MUTEX_T SemaphoreHandle_t
#define LOCK(mutex) while(xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE)
#define TRYLOCK(mutex) (xSemaphoreTake(mutex, portTICK_PERIOD_MS * 100) == pdTRUE)
#define UNLOCK(mutex) xSemaphoreGive(mutex); 

extern "C" {
#include <threadkernel.h>
};

extern AOS::TemperatureSensors TEMPERATURES; 
extern CPU cpu; 
extern volatile char* httpResponseString; 

extern volatile unsigned long            tempErrors             ;

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
static void afterProcess0(process_t *process);
static void beforeProcess0(process_t *process);
static void afterProcess1(process_t *process);
static void beforeProcess1(process_t *process);
static void readIndexHTML(const char * htmlFilePath);
static void onMillisRollover();

static void startWatchdogTimer(); 
static bool watchdogCallback(repeating_timer*); 

void reboot(); 
const char* generateHostname(); 
double seconds(); 
String getFotmattedRealTime(); 

void aosInitialize(); 
void aosSetup(); 
void aosSetup1();

void populateHttpResponse(JsonDocument &document);  
bool handleHttpArg(String argName, String arg); 
void setupFrontEnd(const char * htmlFilePath);

uint32_t getTotalHeap();
uint32_t getFreeHeap(); 

#endif