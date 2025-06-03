#include "Ping.h"
#include "config.h"

using namespace AOS; 
using AOS::PingStats; 
using AOS::Ping; 

PingStats Ping::stats = PingStats(); 

Ping Ping::pingGateway()
{
  return Ping::pingIP(WiFi.gatewayIP());
};

Ping Ping::pingIP(IPAddress ip)
{
  Ping ping(ip, 64); 
  Ping::stats.update(ping); 
  return ping; 
};

PingStats::PingStats()
{
  failed = 0; 
  successful = 0; 
  consecutiveFailed = 0; 
  maxConsecutiveFailed = 0; 
  maxPingMicroseconds = 0; 
  minPingMicroseconds = 0; 
  totalPingTimeMicroseconds = 0; 
  lastPingSuccessful = true; 
  lastSuccessfulPingMicroseconds = 0; 
};

Ping::Ping(IPAddress ip, uint8_t ttl) 
{
  unsigned long timeMicros = micros(); 
  this->ttl = WiFi.ping(ip, ttl); 
  this->pingTimeMicroseconds = micros() - timeMicros; 
};


void PingStats::update(AOS::Ping& ping)
{
  if (ping.isSuccessful())
  {
    unsigned int previousSuccessful = PingStats::successful++; 
    PingStats::consecutiveFailed = 0; 
    PingStats::lastSuccessfulPingMicroseconds = ping.getTimeMicroseconds(); 
    PingStats::totalPingTimeMicroseconds += ping.getTimeMicroseconds(); 
    PingStats::maxPingMicroseconds = ping.getTimeMicroseconds() > PingStats::maxPingMicroseconds ? ping.getTimeMicroseconds() : PingStats::maxPingMicroseconds; 
    PingStats::minPingMicroseconds = 
      previousSuccessful > 0 
        ? ping.getTimeMicroseconds() < PingStats::minPingMicroseconds ? ping.getTimeMicroseconds() : PingStats::minPingMicroseconds
        : ping.getTimeMicroseconds(); 
    PingStats::lastPingSuccessful = true;
  }
  else 
  {
    PingStats::lastPingSuccessful = false; 
    PingStats::failed++; 
    PingStats::consecutiveFailed++; 
  }

  if (PingStats::consecutiveFailed > PingStats::maxConsecutiveFailed)
  {
    PingStats::maxConsecutiveFailed++; 
  }
}

void PingStats::addStats(const char* key, JSONVar& document)
{
  document[key]["last"] = getLast(); 
  document[key]["average"] = getAverage(); 
  document[key]["min"] = getMin(); 
  document[key]["max"] = getMax(); 
  document[key]["successful"] = getSuccessful(); 
  document[key]["failed"] = getFailed(); 
  document[key]["failedConsecutive"] = getConsecutiveFailed(); 
  document[key]["failedMaxConsecutive"] = getMaxConsecutiveFailed(); 
};