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
#pragma once 
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <Wire.h>

//#define LCD_ADDRESS 0x27
#define LCD_ADDRESS 0x3F
#define LCD_COLS    16
#define LCD_ROWS     2

#define STR_(X) #X
#define STR(X) STR_(X)

namespace AOS
{
	class LCDPrint; 

	class LCDPrint
	{
		private: 
			volatile bool rasterDirty; 
			char** lcdRaster;
			LiquidCrystal_I2C lcd;

			bool lcdConnected; 

			bool refreshing; 
			bool printing; 

			void scroll(); 
			bool testAddress(uint8_t addr); 

		public: 
			LCDPrint() : lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS) 
			{
				rasterDirty = false; 
				printing = false; 
				refreshing = false; 
				lcdConnected = false; 

				lcdRaster = (char **)malloc(sizeof(char*) * LCD_ROWS); 

				for (int i = 0; i < LCD_ROWS; i++)
				{
					lcdRaster[i] = (char *)malloc(sizeof(char) * (LCD_COLS + 1));
					sprintf(lcdRaster[i], "%-*ls", LCD_COLS, "...");
				}
			}; 

			void init()
			{
				if (testAddress(LCD_ADDRESS))
				{
					lcdConnected = true; 
					lcd.init();           // LCD driver initialization
					lcd.backlight();  
				}
				else 
				{
					lcdConnected = false; 
				}
			}; 

			bool connected()
			{
				return lcdConnected; 
			}; 

			void refresh(); 
			void printLn(const char * formatBuffer); 
			void printfLn(const char *format, ...);
			void reprintLn(const char * formatBuffer);  
			void reprintfLn(const char *format, ...); 
	}; 
}
