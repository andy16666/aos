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
#include <LittleFS.h> 
#include <WiFiNTP.h>
#include <RPi_Pico_TimerInterrupt.h>

using namespace AOS; 
using AOS::Ping; 

CPU cpu = CPU(); 
WebServer server(80);

static RPI_PICO_TimerInterrupt* watchdogTimer = 0; 

/* The following are located in uninitialized memory, and so are preserved across a reboot. */
/*
  Used to indicate if the system has just been powered on. If so, initialize will consist of 
  uninitialized bits which are most likely not all zero. If the system has not been power cycled 
  but has instead been rebooted, this will preserve its value of zero, indicating we can also 
  trust the rest of the data in uninitialized memory. 
*/ 
volatile unsigned long            initialize               __attribute__((section(".uninitialized_data")));

// Total time between power up and the last reboot. 
volatile double                   timeBaseSeconds          __attribute__((section(".uninitialized_data")));

// Total time since power up. 
volatile unsigned long            powerUpTime              __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsMillisRollover __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsDisconnected   __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsPingFailed     __attribute__((section(".uninitialized_data")));
volatile unsigned long            numRebootsWDT            __attribute__((section(".uninitialized_data")));

volatile unsigned long            lastProcess0             __attribute__((section(".uninitialized_data"))); 
volatile unsigned long            lastProcess1             __attribute__((section(".uninitialized_data")));

volatile unsigned long            tempErrors               __attribute__((section(".uninitialized_data")));

threadkernel_t* CORE_0_KERNEL = create_threadkernel(&millis, &onMillisRollover, &micros, &beforeProcess0, &afterProcess0); 
threadkernel_t* CORE_1_KERNEL = create_threadkernel(&millis, &onMillisRollover, &micros, &beforeProcess1, &afterProcess1); 

static const char* hostname = generateHostname();

volatile char* httpResponseString;

volatile int core2Start = 0; 

MUTEX_T networkMutex;

TemperatureSensors TEMPERATURES = TemperatureSensors(TEMP_SENSOR_PIN); 

char indexHTML[HTML_BUFFER_SIZE]; 

volatile unsigned int lastRebootCausedBy = 0;
volatile unsigned long            core0AliveAt; 
volatile unsigned long            core1AliveAt;


void setup() 
{
  if (initialize)
  {
    timeBaseSeconds = 0; 
    
    powerUpTime = millis(); 
    numRebootsPingFailed = 0;  
    numRebootsDisconnected = 0; 
    numRebootsMillisRollover = 0; 
    numRebootsWDT = 0; 
    lastProcess0 = 0; 
    lastProcess1 = 0; 
    tempErrors = 0; 
    aosInitialize(); 
    initialize = 0; 
  }

  lastRebootCausedBy = lastProcess0 
          ? lastProcess0 
          : (lastProcess1 ? lastProcess1 : lastRebootCausedBy); 

  Serial.begin();
  Serial.setDebugOutput(false);

  if (rp2040.getResetReason() == RP2040::WDT_RESET)
  {
    timeBaseSeconds += core0AliveAt / 1E3;
    numRebootsWDT++; 
    
    WPRINTF("watchdog timer caused a reboot. Last PIDs: (%d, %d)\r\n", lastProcess0, lastProcess1); 
  }

  core0AliveAt = millis(); 
  core1AliveAt = millis();

  setenv("TZ", TIMEZONE, 1); 
  tzset(); 

  wifi_connect();

  networkMutex = xSemaphoreCreateRecursiveMutex();

  httpResponseString = (volatile char *)malloc(HTTP_RESPONSE_BUFFER_SIZE * sizeof(char)); 
  httpResponseString[0] = 0;
  
  cpu.begin(); 

  pinMode(CORE_0_ACT, OUTPUT); 
  pinMode(CORE_1_ACT, OUTPUT); 

  if (!MDNS.begin(hostname))
  {
    EPRINTLN("Failed to start MDNS. Rebooting.");
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
            case 'A': 
            {
              rp2040.rebootToBootloader();  
              break;
            }
            default:   success = false;  
          }
        }

        DPRINTLN("/ handler: parse reboot"); 

        if (argName.equals("reboot") && arg.length() == 1) 
        {
          switch(arg.charAt(0))
          {
            case 'A': 
            { 
              server.sendHeader("Location", "/");
              server.send(302, "text/plain", "Redirecting...");
              reboot(); 
              break;
            }
            default:   success = false;  
          }
        } 
      }
    }

    if (success)
    {
      DPRINTLN("/ handler: success, get lock"); 

      LOCK(networkMutex); 
      String responseString = httpResponseString[0] ? String((char *)httpResponseString) : String("Loading..."); 
      DPRINTLN("/ handler: drop lock"); 
      UNLOCK(networkMutex); 

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

  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_mdnsUpdate); 
  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_testWiFiConnection); 
  CORE_0_KERNEL->addImmediate(CORE_0_KERNEL, task_handleHttpClient); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOn, 1000); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_testPing, PING_INTERVAL_MS); 
  aosSetup(); 
  CORE_0_KERNEL->add(CORE_0_KERNEL, task_core0ActOff, 1100); 

  server.onNotFound(handleHttpNotFound);
  server.begin();
  NPRINTLN("HTTP server started");

  startWatchdogTimer(); 

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

void startWatchdogTimer()
{
  watchdogTimer = new RPI_PICO_TimerInterrupt(WATCHDOG_TIMER_IRQ); 
  if (watchdogTimer->attachInterrupt(1, watchdogCallback))
  {
    NPRINTLN("Starting timer OK, millis() = " + String(millis()));
  }
  else
  {
    EPRINTLN("Can't set timer. Select another freq. or timer");
  }
}

bool watchdogCallback(repeating_timer*)
{
  unsigned long timeMs = millis(); 

  if (!core2Start || timeMs < STARTUP_GRACE_PERIOD_MS)
  {
    return true; 
  }

  if (core0AliveAt < timeMs - WATCHDOG_TIMER_REBOOT_MS) 
  {
    numRebootsWDT++; 
    reboot();   
  }

  if (core2Start && core1AliveAt < timeMs - WATCHDOG_TIMER_REBOOT_MS) 
  {
    numRebootsWDT++; 
    reboot(); 
  }

  return true; 
}

void loop() 
{
  core0AliveAt = millis(); 
  CORE_0_KERNEL->run(CORE_0_KERNEL);
}

void loop1()
{
  while(!core2Start); 
  core1AliveAt = millis(); 
  CORE_1_KERNEL->run(CORE_1_KERNEL);
}

void beforeProcess0(process_t *p)
{
  core0AliveAt = millis(); 
  lastProcess0 = p->pid; 
}

void afterProcess0(process_t *p)
{
  core0AliveAt = millis(); 
  lastProcess0 = 0; 
}

void beforeProcess1(process_t *p)
{
  core1AliveAt = millis(); 
  lastProcess1 = p->pid; 
}

void afterProcess1(process_t *p)
{
  core1AliveAt = millis(); 
  lastProcess1 = 0; 
}

double seconds()
{
  return timeBaseSeconds + (millis()/1E3); 
}

void onMillisRollover()
{
  reboot();
}

void setupFrontEnd(const char * htmlFilePath)
{
  Serial.println("Opening FS"); 

  if(!LittleFS.begin())
  {
    EPRINTLN("Failed to mount filesystem."); 
    reboot(); 
  }

  Serial.println("Reading FS"); 
  readIndexHTML(htmlFilePath); 

  Serial.println("Read FS"); 

  server.on("/index.html", []() {
    server.send(200, "text/html", indexHTML);
  });
}

void readIndexHTML(const char * htmlFilePath)
{
  File file = LittleFS.open(htmlFilePath, "r");
  if (!file)
  {
    EPRINTLN("Failed to mount file."); 
    reboot(); 
  }

  char *indexHTMLPtr = indexHTML; 
  unsigned int count = 0; 
  while(file.available())
  {
    *(indexHTMLPtr++) = file.read();
    *indexHTMLPtr = 0; 

    if ((++count) > HTML_BUFFER_SIZE - 2)
    {
      EPRINTLN("Failed to read HTML: buffer full."); 
      reboot(); 
    }
  }

  Serial.printf("Read %d/%d chars from file into buffer\r\n", count, HTML_BUFFER_SIZE); 

  file.close(); 
}

void task_handleHttpClient()
{
  server.handleClient(); 
  // Won't boot without at least 10ms delay here. handleClient() has 
  // a "_nulldelay" which is 1ms. It's weird, but apparently.
  delay(50); 
}

static inline void addUptimeStats(const char *prefix, JsonDocument& document)
{
  //document[prefix]["time"] = getFotmattedRealTime();
  document[prefix]["powered"] = secondsToHMS(seconds()).c_str();
  document[prefix]["booted"] = msToHumanReadableTime(millis() - startupTime).c_str();
  document[prefix]["connected"] = msToHumanReadableTime(millis() - connectTime).c_str();
  document[prefix]["numRebootsPingFailed"] = numRebootsPingFailed; 
  document[prefix]["numRebootsDisconnected"] = numRebootsDisconnected; 
  document[prefix]["numRebootsMillisRollover"] = numRebootsMillisRollover; 
  document[prefix]["numRebootsWDT"] = numRebootsWDT; 
  document[prefix]["lastRebootCausedBy"] = lastRebootCausedBy; 
  document[prefix]["core0AliveAt"] = msToHumanReadableTime(millis() - core0AliveAt).c_str();
  document[prefix]["core1AliveAt"] = msToHumanReadableTime(millis() - core1AliveAt).c_str();
}

void task_updateHttpResponse() 
{
  unsigned long time = millis(); 
  JsonDocument document; 

  TEMPERATURES.addTo(document); 
  document["cpuTempC"] = cpu.getTemperature(); 
  document["tempErrors"]   = tempErrors; 

  populateHttpResponse(document); 
  
  Ping::stats.addStats("ping", document); 
  document["freeHeapB"] = getFreeHeap(); 
  addUptimeStats("uptime", document); 

  if (!TRYLOCK(networkMutex))
  {
    return; 
  }
  httpResponseString[HTTP_RESPONSE_BUFFER_SIZE - 1] = 0; 
  serializeJson(document, (char *)httpResponseString, HTTP_RESPONSE_BUFFER_SIZE * sizeof(char)); 
  if(httpResponseString[HTTP_RESPONSE_BUFFER_SIZE - 1])
  {
    WPRINTLN("httpResponseString buffer full");
    httpResponseString[HTTP_RESPONSE_BUFFER_SIZE - 1] = 0; 
  }

  UNLOCK(networkMutex);
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
    WPRINTLN("Reboot from task_testWiFiConnection"); 
    numRebootsDisconnected++; 
    reboot();  
  }
}

void task_testPing()
{
  Ping ping = Ping::pingGateway(); 

  if (Ping::stats.getConsecutiveFailed() >= MAX_CONSECUTIVE_FAILED_PINGS)
  {
    WPRINTLN("Reboot from task_testPing"); 
    numRebootsPingFailed++; 
    reboot(); 
  }
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

  for (uint8_t i = 0; i < server.args(); i++) 
  {
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
  //NTP.begin("pool.ntp.org", "time.nist.gov");

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

  //DPRINTF("Setting Clock"); 
  //NTP.waitSet();

  DPRINTLN("Connected");
  DPRINT("Connected to ");
  DPRINTLN(WiFi.SSID());

  DPRINT("IP: ");
  DPRINTLN(WiFi.localIP());

  DPRINT("Hostname: ");
  DPRINT(hostname);
  DPRINTLN(".local");

  connectTime = millis();
}

String getFotmattedRealTime()
{
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return String(asctime(&timeinfo));
}

void reboot()
{
  timeBaseSeconds += millis() / 1E3; 
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