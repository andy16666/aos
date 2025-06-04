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
#include "aos.h"
#include "config.h"
#include "util.h"

using namespace AOS; 
using AOS::Ping; 

TemperatureSensors TEMPERATURES = TemperatureSensors(TEMP_SENSOR_PIN);
CPU cpu = CPU(); 

volatile long            initialize             __attribute__((section(".uninitialized_data")));
volatile long            powerUpTime            __attribute__((section(".uninitialized_data")));
volatile long            numRebootsDisconnected __attribute__((section(".uninitialized_data")));
volatile long            timeBaseMs             __attribute__((section(".uninitialized_data")));
volatile long            tempErrors             __attribute__((section(".uninitialized_data")));
volatile long            numRebootsPingFailed   __attribute__((section(".uninitialized_data")));

threadkernel_t* CORE_0_KERNEL = create_threadkernel(&millis); 
threadkernel_t* CORE_1_KERNEL = create_threadkernel(&millis); 

const char* HOSTNAME = generateHostname();

String LOADING_STRING = String("Loading..."); 
String& httpResponseString = LOADING_STRING;

void setup() 
{
  if (initialize)
  {
    timeBaseMs = 0; 
    powerUpTime = millis(); 
    tempErrors = 0;    
    numRebootsPingFailed = 0;  
    numRebootsDisconnected = 0; 
    aosInitialize(); 
    initialize = 0; 
  }

  pinMode(CORE_0_ACT, OUTPUT); 
  pinMode(CORE_1_ACT, OUTPUT); 

  cpu.begin(); 
  
  Serial.begin();
  Serial.setDebugOutput(false);

  wifi_connect();

  if (MDNS.begin(HOSTNAME)) 
  {
    Serial.println("MDNS responder started");
  }

  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_mdnsUpdate); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOn, 1000); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testPing, PING_INTERVAL_MS); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testWiFiConnection, 5000); 
  aosSetup(); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOff, 1100); 
}

void setup1()
{
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOn, 1000); 
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_readTemperatures, TemperatureSensor::READ_INTERVAL_MS); 
  aosSetup1();  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_updateHttpResponse, 2000);  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOff, 1100);

  TEMPERATURES.discoverSensors();
}

void loop() 
{
  CORE_0_KERNEL->run(CORE_0_KERNEL); 
}

void loop1()
{
  CORE_1_KERNEL->run(CORE_1_KERNEL); 
}

void task_updateHttpResponse() 
{
  JSONVar document; 
  
  TEMPERATURES.addTo(document); 
  document["cpuTempC"] = cpu.getTemperature(); 
  document["tempErrors"] = tempErrors; 

  populateHttpResponse(document); 
  
  Ping::stats.addStats("ping", document); 
  document["ping"]["numRebootsPingFailed"] = numRebootsPingFailed; 

  document["freeHeapB"] = getFreeHeap(); 

  document["uptime"]["powered"] = msToHumanReadableTime((timeBaseMs + millis()) - powerUpTime).c_str();
  document["uptime"]["booted"] = msToHumanReadableTime(millis() - startupTime).c_str();
  document["uptime"]["connected"] = msToHumanReadableTime(millis() - connectTime).c_str();
  
  httpResponseString = document.stringify(document); 
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
    Serial.println("WiFi disconnected, rebooting.");
    numRebootsDisconnected++; 
    reboot();  
  }
}

void task_testPing()
{
  Ping ping = Ping::pingGateway(); 

  if (Ping::stats.getConsecutiveFailed() >= MAX_CONSECUTIVE_FAILED_PINGS)
  {
    Serial.println("Gateway unresponsive, rebooting.");
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
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.noLowPowerMode();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 20;
  while (!is_wifi_connected() && (attempts--)) 
  {
    delay(1000);
  }

  if (!is_wifi_connected()) {
    Serial.print("Connect failed! Rebooting.");
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