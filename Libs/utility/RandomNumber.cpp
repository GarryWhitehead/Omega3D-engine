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

#include "RandomNumber.h"

RandomNumber::RandomNumber()
    : m_mt(m_rand())
{
}

int RandomNumber::GenerateInt(const int min, const int max)
{
	std::uniform_int_distribution<int> num(min, max);
	return num(m_mt);
}

int RandomNumber::GenerateNegInt(const int min, const int max, bool nonZero)
{
	int posmin = 0;
	int minmax = 0;

	if (min < 0)
	{

		posmin = std::abs(min);
		minmax = max + posmin;
	}

	int gen = 0;
	std::uniform_int_distribution<int> num(0, minmax);
	gen = num(m_mt) - posmin;

	if (nonZero)
	{

		while (gen == 0)
		{

			std::uniform_int_distribution<int> num(0, minmax);
			gen = num(m_mt) - posmin;
		}
	}
	return gen;
}

bool RandomNumber::RandomChance(const int percentage)
{
	int max = percentage / 100;
	std::uniform_int_distribution<int> num(1, max);

	return num(m_mt) == 1;
}

float RandomNumber::GaussRandomNumber(const float max)
{
	std::uniform_real_distribution<float> num(0, max);

	float gen1 = num(m_mt);
	float gen2 = num(m_mt);

	gen1 /= RAND_MAX;
	gen2 /= RAND_MAX;

	if (gen1 < 0.000001f)
	{
		gen1 = 0.000001f;
	}

	return sqrt(-2.0f * std::logf(gen1)) * cosf(2.0f * PI * gen2);
}
