#include "RandomNumber.h"

RandomNumber::RandomNumber() : 
	m_mt(m_rand())
{
}

int RandomNumber::GenerateInt(int min, int max)
{
	std::uniform_int_distribution<int> num(min, max);
	return num(m_mt);
}

int RandomNumber::GenerateNegInt(int min, int max, bool nonZero)
{
	int posmin = 0;
	int minmax = 0;

	if (min < 0) {
		posmin = fabs(min);
		minmax = max + posmin;
	}

	int gen = 0;
	std::uniform_int_distribution<int> num(0, minmax);
	gen = num(m_mt) - posmin;

	if (nonZero) {
		while (gen == 0) {
			std::uniform_int_distribution<int> num(0, minmax);
			gen = num(m_mt) - posmin;
		}
	}
	return gen;
}

bool RandomNumber::RandomChance(int percentage)
{
	int max = percentage / 100;
	std::uniform_int_distribution<int> num(1, max);
	return num(m_mt) == 1;
}

float RandomNumber::GaussRandomNumber(float max)
{
	std::uniform_real_distribution<float> num(0, max);
	float gen1 = num(m_mt);
	float gen2 = num(m_mt);
	gen1 /= RAND_MAX;
	gen2 /= RAND_MAX;
	if (gen1 < 0.000001) {
		gen1 = 0.000001;
	}
	return sqrt(-2 * logf(gen1)) * cosf(2 * PI * gen2);

}