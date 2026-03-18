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
	A simple PID control loop using a rolling error window of a specified size. 
	
	Author: Andrew Somerville <andy16666@gmail.com> 
	GitHub: andy16666
*/
#pragma once
#include "Window.h"

namespace AOS
{
	class PIDController
	{
		private: 
			Window<float> errorWindow; 
			float kp, kd, ki; 

		public: 
			PIDController(int windowSize, float kp, float kd, float ki) : errorWindow(windowSize)
			{
				this->kp = kp; 
				this->kd = kd; 
				this->ki = ki; 
			}; 

			float addAndGetError(float error)
			{
				errorWindow.add(error); 

				return kp * error + kd * errorWindow.getAverageChange() + ki * errorWindow.getAverage(); 
			}; 
		
			void reset()
			{
				errorWindow.clear(); 
			}
	};
}