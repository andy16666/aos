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
  for (int i = 0; i < LCD_ROWS; i++)
  {
      if (!i) 
        free(lcdRaster[i]); 
      else 
        lcdRaster[i-1] = lcdRaster[i]; 
  }
  lcdRaster[LCD_ROWS - 1] = (char *)malloc(sizeof(char) * (LCD_COLS + 1));
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

  for (int row = 0; row < LCD_ROWS; row++)
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

  lcdRaster[LCD_ROWS - 1][0] = 0; 
  sprintf(lcdRaster[LCD_ROWS - 1], "%-*.*s", LCD_COLS, LCD_COLS, formatBuffer);
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
    if (strlen(formatBuffer) >= LCD_COLS)
    {
      strncpy(lcdRaster[LCD_ROWS - 1], formatBuffer, LCD_COLS);
      formatBuffer += LCD_COLS; 
    }
    else 
    {
      sprintf(lcdRaster[LCD_ROWS - 1], "%-*.*s", LCD_COLS, LCD_COLS, formatBuffer);
      formatBuffer += strlen(formatBuffer); 
    }
    rasterDirty = true; 
    
    printing = false; 
  }  
}



