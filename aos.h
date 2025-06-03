#ifndef AOS_H
#define AOS_H
#include <sys/_intsup.h>
#include <cstdlib>
#include <stdint.h> 
#include <float.h>
#include <math.h>
#include <string.h>

#include <Arduino.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <CPU.h>
#include <LEAmDNS.h>

#include "TemperatureSensors.h"
#include "Ping.h"

extern "C" {
#include <threadkernel.h>
};

extern AOS::TemperatureSensors TEMPERATURES; 
extern CPU cpu; 

extern volatile long            initialize             ;
extern volatile long            powerUpTime            ;
extern volatile long            numRebootsDisconnected ;
extern volatile long            timeBaseMs             ;
extern volatile long            tempErrors             ;
extern volatile long            numRebootsPingFailed   ;

volatile inline unsigned long startupTime = millis();
volatile inline unsigned long connectTime = millis();

extern threadkernel_t* CORE_0_KERNEL; 
extern threadkernel_t* CORE_1_KERNEL;

extern const char* HOSTNAME; 

void reboot(); 
const char* generateHostname(); 

void aosInitialize(); 
void aosSetup(); 
void aosSetup1(); 

void task_core0ActOn(); 
void task_core1ActOn(); 
void task_core0ActOff(); 
void task_core1ActOff(); 

void task_testPing(); 
void task_readTemperatures();

void task_mdnsUpdate(); 
void wifi_connect();
bool is_wifi_connected();
void task_testWiFiConnection(); 

#endif