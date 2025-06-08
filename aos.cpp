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
#include <WebServer.h>

using namespace AOS; 
using AOS::Ping; 

CPU cpu = CPU(); 
WebServer server(80);

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

SemaphoreHandle_t networkMutex;

TemperatureSensors TEMPERATURES = TemperatureSensors(TEMP_SENSOR_PIN); 

void setup() 
{
  if (initialize)
  {
    timeBaseMs = 0; 
    powerUpTime = millis(); 
    numRebootsPingFailed = 0;  
    numRebootsDisconnected = 0; 
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
    Serial.println("Failed to start MDNS. Rebooting.");
    reboot(); 
  }

  server.on("/", []() {
    bool success = true; 
    DPRINTLN("Enter / handler"); 

    for (uint8_t i = 0; i < server.args() && success; i++) 
    {
      String argName = server.argName(i);
      String arg = server.arg(i);

      if (!handleHttpArg(argName, arg))
      {
        success = false;  
      }
      else
      {
        DPRINTLN("/ handler: parse bootloader"); 

        if (argName.equals("bootloader") && arg.length() == 1) 
        {
          switch(arg.charAt(0))
          {
            case 'A':  rp2040.rebootToBootloader();  break;
            default:   success = false;  
          }
        }

        DPRINTLN("/ handler: parse reboot"); 

        if (argName.equals("reboot") && arg.length() == 1) 
        {
          switch(arg.charAt(0))
          {
            case 'A':  reboot(); break;
            default:   success = false;  
          }
        } 
      }
    }

    if (success)
    {
      DPRINTLN("/ handler: success, get lock"); 

      while(xSemaphoreTakeRecursive( networkMutex, portMAX_DELAY ) != pdTRUE)
      {
        DPRINTLN("httpAccept waiting");  Serial.flush(); 
      }
      String responseString = httpResponseString[0] ? String((char *)httpResponseString) : String("Loading..."); 
      
      DPRINTLN("/ handler: drop lock"); 
      xSemaphoreGiveRecursive(networkMutex); 

      DPRINTLN("/ handler: send 200"); 

      server.send(200, "text/json", responseString.length() > 0 ? responseString.c_str() : "{ 'status':\"Loading...\" }");

      DPRINTLN("/ handler: success sent 200"); 
    }
    else 
    {
      DPRINTLN("/ handler: fail send 500"); 

      server.send(500, "text/plain", "Failed to parse request.");

      DPRINTLN("/ handler: fail sent 500"); 
    }

    DPRINTLN("Leave / handler"); 
  });

  server.onNotFound(handleHttpNotFound);
  server.begin();
  DPRINTLN("HTTP server started");

  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_mdnsUpdate); 
  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_testWiFiConnection); 
  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_handleHttpClient); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOn, 1000); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testPing, PING_INTERVAL_MS); 
  aosSetup(); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOff, 1100); 

  core2Start = 1; 
}

void setup1()
{
  while(!core2Start); 
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOn, 1000); 
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_readTemperatures, TemperatureSensor::READ_INTERVAL_MS); 
  aosSetup1();  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_updateHttpResponse, 5000);  
  CORE_1_KERNEL->add(CORE_1_KERNEL, task_core1ActOff, 1100);
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

void task_handleHttpClient()
{
  DPRINTLN("Enter task_handleHttpClient");
  server.handleClient(); 
  DPRINTLN("Leave task_handleHttpClient");
  delay(15); 
}

void task_updateHttpResponse() 
{
  unsigned long time = millis(); 
  JsonDocument document; 

  TEMPERATURES.addTo(document); 
  document["cpuTempC"] = cpu.getTemperature(); 

  populateHttpResponse(document); 
  
  Ping::stats.addStats("ping", document); 
  
  document["freeHeapB"] = getFreeHeap(); 
  document["uptime"]["powered"] = msToHumanReadableTime((timeBaseMs + time) - powerUpTime).c_str();
  document["uptime"]["booted"] = msToHumanReadableTime(time - startupTime).c_str();
  document["uptime"]["connected"] = msToHumanReadableTime(time - connectTime).c_str();
  document["uptime"]["core0AliveAt"]   = msToHumanReadableTime(time - core0AliveAt).c_str();
  document["uptime"]["core1AliveAt"]   = msToHumanReadableTime(time - core1AliveAt).c_str();
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
  DPRINTLN("                             Enter task_readTemperatures");
  TEMPERATURES.readSensors();
  DPRINTLN("                             Leave task_readTemperatures");
}

void task_mdnsUpdate()
{
  DPRINTLN("Enter task_mdnsUpdate");
  MDNS.update(); 
  DPRINTLN("Leave task_mdnsUpdate");
}

void task_testWiFiConnection()
{
  DPRINTLN("Enter task_testWiFiConnection");
  if (!is_wifi_connected()) 
  {
    Serial.println("Reboot from task_testWiFiConnection"); Serial.flush(); 
    numRebootsDisconnected++; 
    reboot();  
  }
  DPRINTLN("Leave task_testWiFiConnection");
}

void task_testPing()
{
  DPRINTLN("Enter task_testPing");
  Ping ping = Ping::pingGateway(); 

  if (Ping::stats.getConsecutiveFailed() >= MAX_CONSECUTIVE_FAILED_PINGS)
  {
    Serial.println("Reboot from task_testPing"); Serial.flush(); 
    numRebootsPingFailed++; 
    reboot(); 
  }
  DPRINTLN("Leave task_testPing");
}

void handleHttpNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
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

  DPRINTLN("Connected");
  DPRINT("Connected to ");
  DPRINTLN(WiFi.SSID());

  DPRINT("IP: ");
  DPRINTLN(WiFi.localIP());

  DPRINT("Hostname: ");
  DPRINT(HOSTNAME);
  DPRINTLN(".local");

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