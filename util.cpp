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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> 
#include <Arduino.h> 

char *msToHumanReadableTime(long timeMs)
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

  int allocChars = strlen(buffer) + 1; 
  char *formattedString = (char *)malloc(allocChars * sizeof(char)); 
  strcpy(formattedString, buffer); 

  return formattedString; 
}

uint32_t getTotalHeap(void) 
{
  extern char __StackLimit, __bss_end__;

  return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeap(void) 
{
  struct mallinfo m = mallinfo();

  return getTotalHeap() - m.uordblks;
}