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
#include "LCDPrint.h"

using namespace AOS; 
using AOS::LCDPrint; 

bool LCDPrint::testAddress(uint8_t addr) 
{
  Wire.begin();
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) 
  {
    return true;
  }
  return false;
}

void LCDPrint::scroll()
{
  for (int i = 0; i < rows; i++)
  {
      if (!i) 
        free(lcdRaster[i]); 
      else 
        lcdRaster[i-1] = lcdRaster[i]; 
  }
  size_t lineLength = sizeof(char) * (columns + 1); 
  lcdRaster[rows - 1] = (char *)calloc(lineLength, lineLength);
}

void LCDPrint::refresh()
{
  if (!rasterDirty) 
    return; 

  refreshing = true; 
  if (printing)
  {
    refreshing = false; 
    return; 
  }

  for (int row = 0; row < rows; row++)
  {
      lcd.setCursor(0, row); 
      lcd.print(lcdRaster[row]); 
  }

  rasterDirty = false; 
  refreshing = false; 
}

void LCDPrint::printfLn(const char *format, ...)
{
  char formatBuffer[1024];   

  va_list arg;
  va_start (arg, format);
  vsprintf (formatBuffer, format, arg);
  va_end (arg);

  LCDPrint::printLn(formatBuffer); 
}

void LCDPrint::reprintfLn(const char *format, ...)
{
  char formatBuffer[1024];   

  va_list arg;
  va_start (arg, format);
  vsprintf (formatBuffer, format, arg);
  va_end (arg);

  reprintLn(formatBuffer); 
}

void LCDPrint::reprintLn(const char * formatBuffer)
{
  printing = true; 
  while (refreshing); 

  lcdRaster[rows - 1][0] = 0; 
  sprintf(lcdRaster[rows - 1], "%-*.*s", columns, columns, formatBuffer);
  rasterDirty = true; 

  printing = false; 
}

void LCDPrint::printLn(const char * formatBuffer)
{
  while (strlen(formatBuffer) > 0)
  {
    printing = true; 
    while (refreshing); 

    scroll(); 
    if (strlen(formatBuffer) >= columns)
    {
      strncpy(lcdRaster[rows - 1], formatBuffer, columns);
      formatBuffer += columns; 
    }
    else 
    {
      sprintf(lcdRaster[rows - 1], "%-*.*s", columns, columns, formatBuffer);
      formatBuffer += strlen(formatBuffer); 
    }
    rasterDirty = true; 
    
    printing = false; 
  }  
}



