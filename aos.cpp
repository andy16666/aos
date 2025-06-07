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
//#include "RPi_Pico_TimerInterrupt.h"

using namespace AOS; 
using AOS::Ping; 

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

volatile char* httpResponseString;

static volatile unsigned long core0AliveAt = millis(); 
static volatile unsigned long core1AliveAt = millis();

volatile int core2Start = 0; 

//RPI_PICO_Timer aosWatchdogTimer0(0);
//RPI_PICO_Timer aosWatchdogTimer1(1);

SemaphoreHandle_t networkMutex;

TemperatureSensors TEMPERATURES = TemperatureSensors(TEMP_SENSOR_PIN); 

void setup() 
{
  //rp2040.idleOtherCore(); 
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

  Serial.begin();
  Serial.setDebugOutput(false);
  wifi_connect();

  networkMutex = xSemaphoreCreateRecursiveMutex();
  httpResponseString = (volatile char *)malloc(HTTP_RESPONSE_BUFFER_SIZE * sizeof(char)); 
  httpResponseString[0] = 0;
  
 
  
  cpu.begin(); 

  pinMode(CORE_0_ACT, OUTPUT); 
  pinMode(CORE_1_ACT, OUTPUT); 

  

  if (!MDNS.begin(HOSTNAME))
  {
    Serial.println("Failed to start MDNS");
    reboot(); 
  }
  
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_mdnsUpdate, 1); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testWiFiConnection, 109); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOn, 100); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testPing, PING_INTERVAL_MS); 
  aosSetup(); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOff, 110); 
  //rp2040.resumeOtherCore(); 

  core2Start = 1; 
}

void setup1()
{
  while(!core2Start); 
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOn, 100); 
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_readTemperatures, TemperatureSensor::READ_INTERVAL_MS); 
  aosSetup1();  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_updateHttpResponse, 5000);  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOff, 110);

  // while(!TEMPERATURES.ready())
  // {
  //   Serial.println("Discovering sensors..."); 
  //   //TEMPERATURES.discoverSensors(); 
  //   TEMPERATURES.readSensors(); 
  //   TEMPERATURES.printSensors(); 
  // }

}

void loop() 
{
  core0AliveAt = millis(); 
  CORE_0_KERNEL->run(CORE_0_KERNEL);
  Serial.flush(); 
}

void loop1()
{
  while(!core2Start); 
  core1AliveAt = millis(); 
  CORE_1_KERNEL->run(CORE_1_KERNEL);
  Serial.flush(); 
}

void task_updateHttpResponse() 
{
  unsigned long time = millis(); 
  JsonDocument document; 

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
  document["uptime"]["numRebootsPingFailed"]   = numRebootsPingFailed; 
  document["uptime"]["numRebootsDisconnected"] = numRebootsDisconnected; 
  if (xSemaphoreTakeRecursive( networkMutex, portTICK_PERIOD_MS * 100 ) != pdTRUE)
  {
    return; 
  }
  serializeJson(document, (char *)httpResponseString, HTTP_RESPONSE_BUFFER_SIZE * sizeof(char)); 
  if(httpResponseString[HTTP_RESPONSE_BUFFER_SIZE - 1])
  {
    Serial.println("WARNING: httpResponseString buffer full");
    httpResponseString[HTTP_RESPONSE_BUFFER_SIZE - 1] = 0; 
  }
  xSemaphoreGiveRecursive(networkMutex); 
}

void task_core0ActOn()  { digitalWrite(CORE_0_ACT, 1); }
void task_core1ActOn()  { digitalWrite(CORE_1_ACT, 1); }
void task_core0ActOff() { digitalWrite(CORE_0_ACT, 0); }
void task_core1ActOff() { digitalWrite(CORE_1_ACT, 0); }

void task_readTemperatures()
{
  Serial.println("                             Enter task_readTemperatures");
  TEMPERATURES.readSensors();
  Serial.println("                             Leave task_readTemperatures");
}

void task_mdnsUpdate()
{
  Serial.println("Enter task_mdnsUpdate");
  MDNS.update(); 
  Serial.println("Leave task_mdnsUpdate");
}

void task_testWiFiConnection()
{
  Serial.println("Enter task_testWiFiConnection");
  if (!is_wifi_connected()) 
  {
    Serial.println("Reboot from task_testWiFiConnection");
    Serial.flush(); 
    numRebootsDisconnected++; 
    reboot();  
  }
  Serial.println("Leave task_testWiFiConnection");
}

void task_testPing()
{
  Serial.println("Enter task_testPing");
  Ping ping = Ping::pingGateway(); 

  if (Ping::stats.getConsecutiveFailed() >= MAX_CONSECUTIVE_FAILED_PINGS)
  {
    numRebootsPingFailed++; 
    reboot(); 
  }
  Serial.println("Leave task_testPing");
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

  int attempts = 50;
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