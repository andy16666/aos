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
#include "aos.h"
#include "config.h"
#include "util.h"
#include "RPi_Pico_TimerInterrupt.h"

using namespace AOS; 
using AOS::Ping; 

TemperatureSensors TEMPERATURES = TemperatureSensors(TEMP_SENSOR_PIN);
CPU cpu = CPU(); 

/* The following are located in uninitialized memory, and so are preserved across a reboot. */
/*
  Used to indicate if the system has just been powered on. If so, initialize will consist of 
  uninitialized bits which are most likely not all zero. If the system has not been power cycled 
  but has instead been rebooted, this will preserve its value of zero, indicating we can also 
  trust the rest of the data in uninitialized memory. 
*/ 
volatile unsigned long            initialize             __attribute__((section(".uninitialized_data")));
// Total time between power up and the last reboot. 
volatile unsigned long            timeBaseMs             __attribute__((section(".uninitialized_data")));
// Total time since power up. 
volatile unsigned long            powerUpTime            __attribute__((section(".uninitialized_data")));

volatile unsigned long            tempErrors             __attribute__((section(".uninitialized_data")));

volatile unsigned long            numRebootsDisconnected __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsPingFailed   __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsCore0WDT     __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsCore1WDT     __attribute__((section(".uninitialized_data")));

threadkernel_t* CORE_0_KERNEL = create_threadkernel(&millis); 
threadkernel_t* CORE_1_KERNEL = create_threadkernel(&millis); 

const char* HOSTNAME = generateHostname();

String LOADING_STRING = String("Loading..."); 
String& httpResponseString = LOADING_STRING;

static volatile unsigned long core0AliveAt = millis(); 
static volatile unsigned long core1AliveAt = millis();

RPI_PICO_Timer aosWatchdogTimer0(0);
RPI_PICO_Timer aosWatchdogTimer1(1);

SemaphoreHandle_t networkMutex;

bool aosWatchdogISR(struct repeating_timer *t)
{  
  (void) t;

  unsigned long time = millis(); 
  if (core0AliveAt < time - AOS_WATCHDOG_TIMEOUT_MS)
  {
    numRebootsCore0WDT++; 
    reboot(); 
  }

  if (core1AliveAt < time - AOS_WATCHDOG_TIMEOUT_MS)
  {
    numRebootsCore1WDT++; 
    reboot(); 
  }

  return true; 
}

void setup() 
{
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  cpu.begin(); 

  networkMutex = xSemaphoreCreateRecursiveMutex();

  pinMode(CORE_0_ACT, OUTPUT); 

  if (initialize)
  {
    timeBaseMs = 0; 
    powerUpTime = millis(); 
    tempErrors = 0;    
    numRebootsPingFailed = 0;  
    numRebootsDisconnected = 0; 
    numRebootsCore0WDT = 0; 
    numRebootsCore1WDT = 0; 
    aosInitialize(); 
    initialize = 0; 
  }

  if (!aosWatchdogTimer0.attachInterruptInterval(AOS_WATCHDOG_TIMEOUT_MS * 1000, aosWatchdogISR))
  {
    Serial.println("Failed to start core0 watchdog");
    reboot(); 
  }

  wifi_connect();

  if (!MDNS.begin(HOSTNAME))
  {
    Serial.println("Failed to start MDNS");
    reboot(); 
  }

  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_mdnsUpdate); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testWiFiConnection, 10000); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOn, 1000); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testPing, PING_INTERVAL_MS); 
  aosSetup(); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOff, 1100); 
}

void setup1()
{
  pinMode(CORE_1_ACT, OUTPUT); 

  if (!aosWatchdogTimer1.attachInterruptInterval(AOS_WATCHDOG_TIMEOUT_MS * 1000, aosWatchdogISR))
  {
    Serial.println("Failed to start core1 watchdog");
    reboot(); 
  }
  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOn, 1000); 
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_readTemperatures, TemperatureSensor::READ_INTERVAL_MS); 
  aosSetup1();  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_updateHttpResponse, 2000);  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOff, 1100);
}

void loop() 
{
  core0AliveAt = millis(); 
  CORE_0_KERNEL->run(CORE_0_KERNEL); 
}

void loop1()
{
  core1AliveAt = millis(); 
  CORE_1_KERNEL->run(CORE_1_KERNEL); 
}

void task_updateHttpResponse() 
{
  unsigned long time = millis(); 
  JSONVar document; 

  TEMPERATURES.addTo(document); 
  document["cpuTempC"] = cpu.getTemperature(); 
  document["tempErrors"] = tempErrors; 

  populateHttpResponse(document); 
  
  Ping::stats.addStats("ping", document); 
  
  document["freeHeapB"] = getFreeHeap(); 
  document["uptime"]["powered"] = msToHumanReadableTime((timeBaseMs + time) - powerUpTime).c_str();
  document["uptime"]["booted"] = msToHumanReadableTime(time - startupTime).c_str();
  document["uptime"]["connected"] = msToHumanReadableTime(time - connectTime).c_str();
  document["uptime"]["core0AliveAt"]   = msToHumanReadableTime(time - core0AliveAt).c_str();
  document["uptime"]["core1AliveAt"]   = msToHumanReadableTime(time - core1AliveAt).c_str();
  document["uptime"]["numRebootsCore0WDT"]   = numRebootsCore0WDT;
  document["uptime"]["numRebootsCore1WDT"]   = numRebootsCore1WDT;
  document["uptime"]["numRebootsPingFailed"] = numRebootsPingFailed; 

  if (xSemaphoreTakeRecursive( networkMutex, portTICK_PERIOD_MS * 10 ) != pdTRUE)
  {
    return; 
  }
  httpResponseString = document.stringify(document); 
  xSemaphoreGiveRecursive(networkMutex); 
}

void task_core0ActOn()  { digitalWrite(CORE_0_ACT, 1); }
void task_core1ActOn()  { digitalWrite(CORE_1_ACT, 1); }
void task_core0ActOff() { digitalWrite(CORE_0_ACT, 0); }
void task_core1ActOff() { digitalWrite(CORE_1_ACT, 0); }

void task_readTemperatures()
{
  TEMPERATURES.readSensors();
}

void task_mdnsUpdate()
{
  MDNS.update(); 
}

void task_testWiFiConnection()
{
  if (!is_wifi_connected()) 
  {
    numRebootsDisconnected++; 
    reboot();  
  }
}

void task_testPing()
{
  Ping ping = Ping::pingGateway(); 

  if (Ping::stats.getConsecutiveFailed() >= MAX_CONSECUTIVE_FAILED_PINGS)
  {
    numRebootsPingFailed++; 
    reboot(); 
  }
}

bool is_wifi_connected() 
{
  return WiFi.status() == WL_CONNECTED;
}

void wifi_connect() 
{
  WiFi.noLowPowerMode();
  WiFi.mode(WIFI_STA);
  WiFi.noLowPowerMode();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 20;
  while (!is_wifi_connected() && (attempts--)) 
  {
    delay(1000);
  }

  if (!is_wifi_connected()) 
  {
    numRebootsDisconnected++; 
    reboot(); 
  }

  Serial.println("Connected");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Hostname: ");
  Serial.print(HOSTNAME);
  Serial.println(".local");

  connectTime = millis();
}

void reboot()
{
  timeBaseMs += millis(); 
  rp2040.reboot(); 
}

uint32_t getTotalHeap() 
{
  extern char __StackLimit, __bss_end__;

  return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeap() 
{
  struct mallinfo m = mallinfo();

  return getTotalHeap() - m.uordblks;
}