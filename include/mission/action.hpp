#ifndef MISSION_ACTION_HPP
#define MISSION_ACTION_HPP

#include <functional>
#include <string>

#include "common/state.hpp"

class Action
{
public:
	template <typename F, typename... A>
	Action(F _function, A... args) :
		m_function(std::bind(_function, std::placeholders::_1, std::placeholders::_2, args...))
	{
	}

	Action(FILE*);
	size_t write(FILE*);

	bool run(FILE*, FILE*);

private:
	std::function<bool(FILE*, FILE*)> m_function;
};

bool lprintf(FILE*, FILE*, const std::string);

bool doTorpedoes(FILE*, FILE*);
bool wait(FILE*, FILE*, float);
bool moveExt(FILE*, FILE*, float, float, int, int, float, float, int, int, float, int, int, float, int, int, float, float, float);
bool turnTo(FILE*, FILE*, int, int);
bool alignWith(FILE*, FILE*, int, int, int, int);
bool moveAbsolute(FILE*, FILE*, const State&, float);
bool moveRelative(FILE*, FILE*, const State&, float);
bool moveDir(FILE*, FILE*, const State&, float);
bool moveModel(FILE*, FILE*, int, int, int, float, float, float, float);
bool moveModelDir(FILE*, FILE*, int, int, int, float, float, float, float);

bool moveUntil(FILE*, FILE*, int, int, float, float);

bool dropInBin(FILE*, FILE*);
bool uncoverBin(FILE*, FILE*);

bool shootInHole(FILE*, FILE*, char);
bool moveToHole(FILE*, FILE*, size_t, size_t, size_t, float, float);

bool flag(FILE*, FILE*, size_t, float);

bool variance(FILE*, FILE*, size_t, float);

bool setMaxThrust(FILE*, FILE*, float);
bool setSpeed(FILE*, FILE*, float);

#endif

