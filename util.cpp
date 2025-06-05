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

// https://www.1728.org/relhum.htm
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
  char buffer[1024]; 

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

