/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <random>

class RandomNumber
{
public:
	const float PI = 3.1415926535897932384626433832795f;

	RandomNumber();

	// generates a user-defined variabel type between the min and max values
	template <typename T>
	T Generate(const T min, const T max);

	// generates a random integer between min and max values
	int GenerateInt(const int min, const int max);

	// generates a random inetger, allowing the geneartion of negative numbers
	// nonZero - if set to true, if the random number generated is zero, will loop until
	// a non-zero number is generated
	int GenerateNegInt(const int min, const int max, bool nonZero);

	// returns true if a random number of one is generated with the percentage chance of this
	// occuring define by the user
	bool RandomChance(const int percentage);

	// Gaussian random number genertor with mean = 0  and SD of 1
	float GaussRandomNumber(const float max);

private:
	std::random_device m_rand;
	std::mt19937 m_mt;
};

template <typename T>
T RandomNumber::Generate(const T min, const T max)
{
	std::uniform_real_distribution<T> num(min, max);
	return num(m_mt);
}
