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
//#define LCD_ADDRESS 0x3F
//#define LCD_COLS    16
//#define LCD_ROWS     2

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

			uint8_t address; 
			uint8_t columns; 
			uint8_t rows; 

		public: 
			LCDPrint() : LCDPrint(0x3F, 16, 2) { }; 

			LCDPrint(uint8_t address, uint8_t columns, uint8_t rows) : lcd(address, columns, rows) 
			{
				rasterDirty = false; 
				printing = false; 
				refreshing = false; 
				lcdConnected = false;

				this->address = address; 
				this->columns = columns;  
				this->rows = rows; 

				lcdRaster = (char **)malloc(sizeof(char*) * rows); 

				size_t lineSize = sizeof(char) * (columns + 1); 

				for (int i = 0; i < rows; i++)
				{
					lcdRaster[i] = (char *)calloc(lineSize, lineSize);
					sprintf(lcdRaster[i], "%-*ls", columns, "...");
				}
			}; 

			void init()
			{
				if (testAddress(address))
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
