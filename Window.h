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
	A simple numeric window which keeps a running sum. 
	
	Author: Andrew Somerville <andy16666@gmail.com> 
    GitHub: andy16666
 */
#pragma once 
#include <queue>
#include <list>

namespace AOS
{
	template <typename T>
	class Window
	{
		private: 
			std::list<T> window; 
			int size; 
			int count; 
			T sum; 

			T computeSum()
			{
				T sum = 0; 
				for (const T value : window)
				{
					sum += value; 
				}

				return sum; 
			}

		public: 
			Window(int size)
			{
				this->size = size; 
				this->count = 0; 
				this->sum = 0; 
			}; 

			void add(float value)
			{
				window.push_back(value); 
				if (count >= size)
					window.pop_front(); 
				else 
					count++; 

				this->sum = computeSum(); 
			}

			void clear()
			{
				window.clear(); 
				count = 0; 
				sum = 0; 
			}

			T getSum()
			{
				return sum; 
			}

			T getAverage()
			{
				return count > 0 ? getSum() / count : 0; 
			}

			T getAverageChange()
			{
				return window.back() - getAverage(); 
			} 
	}; 

}