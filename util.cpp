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

#include "util.h"

double calculate_wet_bulb_temp(double dry_bulb_temp, double relative_humidity) {
    double rh = relative_humidity;
    double T = dry_bulb_temp;
    
    double Tw = T * atan(0.151977 * sqrt(rh + 8.313659)) + atan(T + rh) - atan(rh - 1.676331) + 0.00391838 * pow(rh, 1.5) * atan(0.023101 * rh) - 4.686035;
    
    return Tw;
}

/**
 * Computes a wet bulb humidity reading from a temp sensor covered with a wet 
 * wick compared to a dry one in the same environment, similar to a psychrometer. 
 * 
 * See: https://www.1728.org/relhum.htm
 */
double calculate_relative_humidity(double td, double tw) 
{        
    double N = 0.6687451584; 
    double ed = 6.112 * exp((17.502 * td) / (240.97 + td)); 
    double ew = 6.112 * exp((17.502 * tw) / (240.97 + tw));

    if (ed <= 0)
      return 0; 

    double rh = (
      ( ew - N * (1+ 0.00115 * tw) * (td - tw) ) 
      / ed ) 
        * 100.0; 

    return rh; 
}

String msToHumanReadableTime(long timeMs)
{
  char buffer[256]; 

  if (timeMs < 1000)
  {
    sprintf(buffer, "%dms", timeMs);
  }
  else if (timeMs < 60 * 1000)
  {
    sprintf(buffer, "%5.2fs", timeMs / 1000.0);
  }
  else if (timeMs < 60 * 60 * 1000)
  {
    sprintf(buffer, "%5.2fm", timeMs / (60.0 * 1000.0));
  }
  else if (timeMs < 24 * 60 * 60 * 1000)
  {
    sprintf(buffer, "%5.2fh", timeMs / (60.0 * 60.0 * 1000.0));
  }
  else
  {
    sprintf(buffer, "%5.2fd", timeMs / (24.0 * 60.0 * 60.0 * 1000.0));
  }

  return String(buffer); 
}

String secondsToHumanReadableTime(double seconds)
{
  char buffer[256]; 

  if (seconds < 1E-3)
  {
    double unitTime = seconds / 1E6; 
    sprintf(buffer, "%dµs", unitTime);
  }
  else if (seconds < 1.0)
  {
    double unitTime = seconds / 1E3; 
    sprintf(buffer, "%dms", unitTime);
  }
  else if (seconds < 60)
  {
    double unitTime = seconds; 
    sprintf(buffer, "%5.2fs", unitTime);
  }
  else if (seconds < 60 * 60)
  {
    double unitTime = seconds/60.0; 
    sprintf(buffer, "%5.2fm", unitTime);
  }
  else if (seconds < 24 * 60 * 60)
  {
    double unitTime = seconds/(60.0 * 60.0); 
    sprintf(buffer, "%5.2fh", unitTime);
  }
  else
  {
    double unitTime = seconds/(24.0 * 60.0 * 60.0); 
    sprintf(buffer, "%5.2fd", unitTime);
  }

  return String(buffer); 
}

String secondsToHMS(double timeSeconds)
{
  char buffer[256]; 

  if (timeSeconds < 60)
  {
    double unitTime = timeSeconds; 
    
  }
  else if (timeSeconds < 60 * 60)
  {
    double unitTime = timeSeconds/60.0; 
    sprintf(buffer, "%5.2fm", unitTime);
  }
  else if (timeSeconds < 24 * 60 * 60)
  {
    double unitTime = timeSeconds/(60.0 * 60.0); 
    sprintf(buffer, "%5.2fh", unitTime);
  }
  else
  {
    double unitTime = timeSeconds/(24.0 * 60.0 * 60.0); 
    sprintf(buffer, "%5.2fd", unitTime);
  }

  int days  = (int)(timeSeconds / (24.0 * 60.0 * 60.0)); 
  int hours = (int)(timeSeconds / (60.0 * 60.0)) - days * 24; 
  int minutes = (int)(timeSeconds / (60.0)) - (days * 24 * 60  + hours * 60); 
  int seconds = (int)(timeSeconds) - (days * 24 * 60 * 60 + hours * 60 * 60 + minutes * 60 ); 

  sprintf(buffer, "%dd %2dh %2dm %2ds", days, hours, minutes, seconds);

  return String(buffer); 
}


void showbits( char x )
{
    char bits[9]; 
    bits[8] = 0; 
    int i=0;
    for (i = 0; i < 8; i++)
    {
        bits[i] = (x & (0x01 << i) ? '1' : '0');
    }
    Serial.printf("%s\r\n", bits);
}

/**
 * Braindead parity check designed to detect single bit 
 * errors in transmissions. 
 */
uint8_t computeParityByte(char * buffer, int length)
{
    uint8_t checksum = 0; 
    for (int i = 0; i < length; i++)
    {
        checksum ^= (uint8_t)(buffer[i]); 
    }

    return checksum; 
}

