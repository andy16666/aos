#ifndef PING_H
#define PING_H
#include <sys/_intsup.h>
#include <cstdlib>
#include <stdint.h> 

#include <Arduino.h> 
#include <Arduino_JSON.h> 
#include <WiFi.h>

namespace AOS
{
  class PingStats; 

  class Ping
  {
    private: 
      unsigned long pingTimeMicroseconds; 
      int ttl; 
      
    public:
      static PingStats stats; 
      static Ping pingGateway();
      static Ping pingIP(IPAddress ip);

      Ping(IPAddress ip, uint8_t ttl);

      bool isSuccessful()
      {
        return ttl > 0; 
      };

      unsigned long getTimeMicroseconds()
      {
        return pingTimeMicroseconds; 
      };

      float getTimeMilliseconds()
      {
        return pingTimeMicroseconds/1000.0;
      };
  }; 

  class PingStats
  {
    private: 
      unsigned int failed; 
      unsigned int successful; 
      unsigned int consecutiveFailed; 
      unsigned int maxConsecutiveFailed; 
      unsigned long maxPingMicroseconds; 
      unsigned long minPingMicroseconds; 
      unsigned long totalPingTimeMicroseconds; 
      bool lastPingSuccessful; 
      unsigned long lastSuccessfulPingMicroseconds; 

    public: 
      PingStats();
      
      void update(AOS::Ping& ping); 
      void addStats(const char* key, JSONVar& document);
      
      unsigned int getConsecutiveFailed()
      { 
        return consecutiveFailed; 
      };

      unsigned int getMaxConsecutiveFailed()
      {
        return maxConsecutiveFailed; 
      };

      unsigned int getFailed()
      {
        return failed; 
      }; 

      unsigned int getSuccessful()
      {
        return successful; 
      }; 

      double getLast() 
      {
        return lastSuccessfulPingMicroseconds/1000.0; 
      };

      double getAverage()
      {
        return successful > 0 
          ? ((totalPingTimeMicroseconds / (double)successful)/1000.0) 
          : 0.0; 
      };

      double getMin()
      {
        return minPingMicroseconds / 1000.0; 
      };

      double getMax()
      {
        return maxPingMicroseconds / 1000.0; 
      };
  };
}

#endif