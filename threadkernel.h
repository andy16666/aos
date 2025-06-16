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

/*
    A simple real time thread kernel for the Pi Pico RP2040

    Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */
#ifndef THREADKRNEL_HH
#define THREADKRNEL_HH
#define threadkernel_t struct threadkernel_t_t
#define process_t      struct process_t_t
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct threadkernel_t_t 
{
  unsigned int lastPid; 

  process_t*    processes;
  process_t*    immediateProcesses;

  unsigned long totalExecutions; 

  double totalExecutionTimeSeconds; 

  unsigned long (*millis)(); 
  unsigned long (*micros)(); 

  process_t*    (*add)              (threadkernel_t *k, void (*f)(), unsigned long periodMilliseconds);
  process_t*    (*addImmediate)     (threadkernel_t *k, void (*f)());
  
  void          (*run)              (threadkernel_t *k);

  process_t*    (*getProcessByPid)  (threadkernel_t *k, unsigned int pid);

  void          (*afterProcess)     (process_t *);
  void          (*beforeProcess)    (process_t *);
  void          (*onMillisRollover) ();
};

struct process_t_t 
{
  unsigned int pid; 

  unsigned long periodMilliseconds; 
  unsigned long nextRunMilliseconds; 

  unsigned long skippedExecutions; 
  unsigned long totalExecutions; 

  double totalExecutionTimeSeconds; 

  void (*f)(); 

  process_t* next; 
}; 

// Constructor 
threadkernel_t* create_threadkernel(
  unsigned long (*millis)(), 
  void (*onMillisRollover)(), 
  unsigned long (*micros)(), 
  void (*beforeProcess)(process_t *), 
  void (*afterProcess)(process_t *)
);

static process_t* __threadkernel_addImmediate(threadkernel_t *k, void (*f)());
static process_t* __threadkernel_add(threadkernel_t *k, void (*f)(), unsigned long periodMilliseconds);
static void __threadkernel_run(threadkernel_t *k);
static process_t* __threadkernel_get_process_by_pid(threadkernel_t *k, unsigned int pid);

#endif 