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
#include "threadkernel.h"

threadkernel_t* create_threadkernel
(
  unsigned long(*millis)(), 
  void (*onMillisRollover)(), 
  unsigned long(*micros)(), 
  void (*beforeProcess)(process_t*), 
  void (*afterProcess)(process_t*)
) 
{
  threadkernel_t* k = (threadkernel_t *)malloc(sizeof(threadkernel_t));
  
  k->millis                    = millis; 
  k->micros                    = micros; 

  k->afterProcess              = afterProcess;
  k->beforeProcess             = beforeProcess; 
  k->onMillisRollover          = onMillisRollover; 

  k->processes                 = 0; 
  k->immediateProcesses        = 0; 
  k->add                       = __threadkernel_add; 
  k->addImmediate              = __threadkernel_addImmediate; 
  k->run                       = __threadkernel_run; 
  k->getProcessByPid           = __threadkernel_get_process_by_pid; 

  k->totalExecutions           = 0; 
  k->totalExecutionTimeSeconds = 0; 

  return k;
}

static inline process_t* create_process(threadkernel_t *k, void(*f)()) 
{
  static unsigned int nextPid = 1; // PID 0 is a non-process. 

  process_t* process = (process_t*)malloc(sizeof(process_t)); 
  process->pid = nextPid++;

  process->nextRunMilliseconds = 0;

  process->skippedExecutions = 0; 
  process->totalExecutions = 0; 

  process->totalExecutionTimeSeconds = 0; 

  process->next = 0; 
  process->f = f; 

  return process; 
}

process_t* __threadkernel_add(threadkernel_t *k, void(*f)(), unsigned long periodMilliseconds) 
{
  process_t** process_ptr = &(k->processes); 
  while(*process_ptr) process_ptr = &((*process_ptr)->next); 
  *process_ptr = create_process(k, f); 
  (*process_ptr)->periodMilliseconds = periodMilliseconds; 
  (*process_ptr)->nextRunMilliseconds = k->millis() + periodMilliseconds; 
  
  return *process_ptr; 
}

process_t* __threadkernel_addImmediate(threadkernel_t *k, void(*f)()) 
{
  process_t** process_ptr = &(k->immediateProcesses); 
  while(*process_ptr) process_ptr = &((*process_ptr)->next); 
  *process_ptr = create_process(k, f); 
  (*process_ptr)->periodMilliseconds = 0; 
  (*process_ptr)->nextRunMilliseconds = 0; 

  return *process_ptr; 
}

static inline process_t* get_by_pid(process_t *process, unsigned int pid)
{
  while(process) 
  {
    if (process->pid == pid)
      break; 

    process = process->next;  
  }

  return process; 
}

process_t* __threadkernel_get_process_by_pid(threadkernel_t *k, unsigned int pid)
{
  process_t *p = get_by_pid(k->immediateProcesses, pid); 
  return p ? p : get_by_pid(k->processes, pid); 
}

static inline void run_process(threadkernel_t *k, process_t *process)
{
  k->lastPid = process->pid; 
  k->beforeProcess(process); 

  unsigned long startMicros = k->micros(); 
  process->f(); 
  unsigned long endMicros   = k->micros(); 

  // Handle timer rollover. 
  if (endMicros < startMicros)
    endMicros = startMicros; 

  process->totalExecutionTimeSeconds += (endMicros - startMicros) / 1E6; 

  // Handle rollover by not incrementing the counter anymore. 
  unsigned long previousTotalExecutions = process->totalExecutions; 
  unsigned long newTotalExecutions = previousTotalExecutions + 1; 
  if (previousTotalExecutions < newTotalExecutions)
  {
    process->totalExecutions = newTotalExecutions; 
  }

  k->afterProcess(process); 
}

static inline void run_immediate(threadkernel_t *k)
{
  process_t *process = k->immediateProcesses; 
  
  while(process)
  {
    run_process(k, process); 
    process = process->next; 
  }
}

void __threadkernel_run(threadkernel_t *k)
{
  process_t* process = k->processes; 
  unsigned long startMicros = k->micros(); 

  if (!process)
  {
    run_immediate(k); 
  }
  
  while(process)
  {
    unsigned long startTimeMillis = k->millis(); 
    if(startTimeMillis >= process->nextRunMilliseconds)
    {
      process->nextRunMilliseconds += process->periodMilliseconds;

      run_process(k, process); 
      
      unsigned long endTimeMillis = k->millis(); 

      // Handle timer rollover. 
      if(endTimeMillis >= startTimeMillis)
      {
        while(endTimeMillis >= process->nextRunMilliseconds)
        {
          process->skippedExecutions++; 
          process->nextRunMilliseconds += process->periodMilliseconds;
        }
      }
      else 
      {
        k->onMillisRollover(); 
      }
    }
    
    process = process->next; 

    run_immediate(k); 
  }

  unsigned long endMicros   = k->micros(); 
  
  // Handle timer rollover. 
  if (endMicros < startMicros)
    endMicros = startMicros; 

  k->totalExecutionTimeSeconds += (endMicros - startMicros) / 1E6;

  // Handle rollover by not incrementing the counter anymore. 
  unsigned long previousTotalExecutions = k->totalExecutions; 
  unsigned long newTotalExecutions = previousTotalExecutions + 1; 
  if (previousTotalExecutions < newTotalExecutions)
  {
    k->totalExecutions = newTotalExecutions; 
  }
}
