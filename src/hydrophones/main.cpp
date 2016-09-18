#include <cstdio>
#include <cmath>
#include <cstdint>
#include <thread>
#include <chrono>

#include "common/defs.hpp"
#include "common/config.hpp"
#include "common/observation.hpp"

const size_t num_inputs = 4;

struct Triangle
{
	size_t base;
	size_t leg1;
	size_t leg2;

	float dist1;
	float dist2;

	float angle;
};

Triangle horizontal =
{
	0,
	3,
	2,

	.8f,
	1.f,

	.0f,
};

Triangle vertical =
{
	2,
	1,
	0,

	.28f,
	1.f,

	.5f,
};

int main()
{
	FILE* in = stdin;
	FILE* out = stdout;
	FILE* log = stderr;

	bool quit = false;
	while (!quit)
	{
		// each packet preceded by -2^15
		// ignore 0xFF aligning bytes before header
		int c = fgetc(in);
		if (c == 0x00)
		{
			int c = fgetc(in);
			if (c == 0x80)
			{
				bool bad = false;

				// time between input i and input 0
				// don't read any bytes for toa 0 because it is always 0
				int toa[num_inputs] = {0};
				for (size_t i = 1; i < num_inputs; i++)
				{
					// read data
					int l = fgetc(in);
					int h = fgetc(in);

					// detect eof
					if (l == -1 || h == -1) bad = true;

					// convert to int
					toa[i] = [](int l, int h)
					{
						uint16_t v = static_cast<uint16_t>(l);
						v |= static_cast<uint16_t>(h) << 8;
						return (v & (1U << 15))                 ?
							-static_cast<int>(0xFFFFU - v + 1U) :
							static_cast<int>(v)                 ;
					}(l, h);
					fprintf(log, "%zu %i\n", i, toa[i]);
					fflush(log);
				}

				// time between input i and input j
				int tdoa[num_inputs][num_inputs];
				for (size_t i = 0; i < num_inputs; i++)
				{
					for (size_t j = 0; j < num_inputs; j++)
					{
						tdoa[i][j] = toa[i] - toa[j];

						if (std::abs(tdoa[i][j]) > 500) bad = true;
					}
				}

				if (bad) continue;

				// compute angles
				auto computeAngle = [&](Triangle t)
				{
					return static_cast<float>(std::atan2(
						- tdoa[t.leg1][t.base]  * t.dist2,
						- t.dist1 * tdoa[t.leg2][t.base]
					) / (2 * M_PI));
				};
				float theta = computeAngle(horizontal) + horizontal.angle;
				float phi   = computeAngle(vertical)   + vertical.angle;
				fprintf(log, "%f %f\n", theta, phi);
				fflush(log);

				if (
					theta > -.5f &&
					theta <  .5f &&
					phi   >  .0f &&
					phi   <  .5f &&

					!bad &&

					true
				)
				{
					Observation(
					{
						M_PINGER_THETA,
						0,
						-3,
						theta,
						0,
						.1f
					}).write(out);
					Observation(
					{
						M_PINGER_PHI,
						0,
						-3,
						phi,
						0,
						.1f
					}).write(out);
				}
			}
		}
		else if (c == EOF) std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}

