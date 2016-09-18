#ifndef INTERFACE_DATA_HPP
#define INTERFACE_DATA_HPP

#include <queue>
#include <tuple>
#include <mutex>
#include <array>

#include <cv.h>
#include <cstdint>

#include "common/defs.hpp"
#include "common/matrix.hpp"
#include "common/state.hpp"
#include "model/evidence.hpp"

struct Line
{
	char str[256];
};

class Data
{
public:
	Data();

	bool lock();
	bool unlock();

	State state;
	uint32_t stateID;

	std::array<cv::Mat, NUM_IMAGES> image;
	std::array<uint32_t, NUM_IMAGES> imageID;

	std::array<float, NUM_FLAGS> flags;

	Matrix model;
	uint32_t modelID;

	std::queue<Evidence> evidence;
	std::queue<std::tuple<size_t, float> > addVariance;
	std::queue<Line> send_line;

	bool alive;

	State desiredState;
	bool setState;
	bool drop;
	bool grab;
	bool release;
	char shoot;

private:
	std::mutex mutex;
};

#endif

