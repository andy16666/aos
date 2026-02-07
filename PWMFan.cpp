#include "PWMFan.h"

using namespace AOS; 
using AOS::PWMFan; 
using AOS::PWMFans; 

void PWMFans::add(const char *jsonName, pin_size_t pin)
{
	PWMFan fan(jsonName, pin, offBelow, minStart, 100); 
	add(fan); 
}; 

void PWMFans::add(const char *jsonName, pin_size_t pin, bool activeLow)
{
	PWMFan fan(jsonName, pin, activeLow, offBelow, minStart, 100); 
	add(fan); 
}; 