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