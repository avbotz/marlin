#include "mission/action.hpp"

#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>

#include "common/defs.hpp"
#include "common/config.hpp"
#include "mission/command.hpp"
#include "mission/query.hpp"

Action::Action(FILE* in)
{
	// TODO: implement
}

size_t Action::write(FILE* out)
{
	// TODO: implement
	return 0;
}

bool Action::run(FILE* in, FILE* out)
{
	return m_function(in, out);
}

bool lprintf(FILE*, FILE*, const std::string str)
{
	std::cerr << str;
	return true;
}

/**
	Sleeps for an amount of time.

	@param in the file that mission writes to with
		commands. Should be connected to interface.
	@param out the file that mission reads from for
		information. Also should be interface.

	@param time the time it should sleep, in seconds.

	@return true.
*/
bool wait(FILE* in, FILE* out, float time)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(time * 1000)));
	return true;
}

/**
	Moves to a specified State.

	Does not depend on the sub's current location.
	Moves to an absolute point in modeling.

	@param in the file that mission writes to with
		commands. Should be connected to interface.
	@param out the file that mission reads from for
		information. Also should be interface.

	@param target the State the sub should move to.
	@param minDistance how close to the target the sub
		should be before it stops moving.

	@return true unless sub was killed in the process of moving.
*/
bool doTorpedoes(FILE* in, FILE* out)
{
	float vertDist = .1;
	float horDir = 1;

	setSpeed(in, out, .1);

	FILE* sin = fopen("pipe/sonar_out", "r");
	FILE* sout = fopen("pipe/sonar_in", "w");

	int idx;
	float dist;

	fprintf(sout, "f\n");
	fflush(sout);
	fscanf(sin, " %i, %f\n", &idx, &dist);

	while (true)
	{
		fprintf(sout, "f\n");
		fflush(sout);
		fscanf(sin, " %i, %f\n", &idx, &dist);

		if (dist > 6) moveDir(in, out, State(1, 0, 0), 10);
		else
		{
			moveDir(in, out, State(0, 0, 0), 10);
			break;
		}
	}

	moveDir(in, out, State(0, 0, vertDist), .01);

	while (true)
	{
		int prev = idx;

		fprintf(sout, "f\n");
		fflush(sout);
		fscanf(sin, " %i, %f\n", &idx, &dist);

		if (idx == prev) moveDir(in, out, State(0, horDir, 0), 10);
		else
		{
			moveDir(in, out, State(0, 0, 0), 10);
			break;
		}
	}

	shoot(out, 'l');

	int hole = idx;

	while (true)
	{
		int prev = idx;

		fprintf(sout, "f\n");
		fflush(sout);
		fscanf(sin, " %i, %f\n", &idx, &dist);

		if (idx <= hole + 1) moveDir(in, out, State(0, -2*horDir, 0), 10);
		else
		{
			moveDir(in, out, State(0, 0, 0), 10);
			break;
		}
	}

	moveDir(in, out, State(0, 0, -2*vertDist), .01);
	shoot(out, 'r');

	return true;
}

bool moveAbsolute(FILE* in, FILE* out, const State& target, float minDistance)
{
	setState(out, target);

	bool close = false;
	while (!close)
	{
		State state = getState(in, out);

		if (
			state.distanceTo(target) < minDistance     &&
			std::abs(state.yaw() - target.yaw()) < .02 &&
			true
		)
			close = true;
		else if (!alive(in, out)) return false;
		else std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	return true;
}

/**
	Move relative to the sub's current location.

	Essentially adds the sub's current State to the
	input state.

	@param in the file that mission writes to with
		commands. Should be connected to interface.
	@param out the file that mission reads from for
		information. Also should be interface.

	@param diff the relative state to move to.
	@param minDistance how close to the target the sub
		should be before it stops moving.

	@return true unless sub was killed in the process of moving.
*/
bool moveRelative(FILE* in, FILE* out, const State& diff, float minDistance)
{
	auto target = getState(in, out);

	target.setX(target.x() + diff.x());
	target.setY(target.y() + diff.y());
	target.setDepth(target.depth() + diff.depth());
	target.setYaw(target.yaw() + diff.yaw());
	target.setPitch(target.pitch() + diff.pitch());
	target.setRoll(target.roll() + diff.roll());

	return moveAbsolute(in, out, target, minDistance);
}

/**
	Move relative to the sub's current location, in a certain direction.

	Depends on sub's heading.

	@param in the file that mission writes to with
		commands. Should be connected to interface.
	@param out the file that mission reads from for
		information. Also should be interface.

	@param change the State denoting how to move, with
		x and y being forward/backward and left/right.
	@param minDistance how close to the target the sub
		should be before it stops moving.

	@return true unless sub was killed in the process of moving.
*/
bool moveDir(FILE* in, FILE* out, const State& change, float minDistance)
{
	auto state = getState(in, out);
	
	auto dx = change.x() * std::cos((state.yaw() + change.yaw()) * 2 * M_PI)
			- change.y() * std::sin((state.yaw() + change.yaw()) * 2 * M_PI);
	auto dy = change.x() * std::sin((state.yaw() + change.yaw()) * 2 * M_PI)
			+ change.y() * std::cos((state.yaw() + change.yaw()) * 2 * M_PI);
	
	auto target = State(dx, dy, change.depth(), change.yaw(), change.pitch(), change.roll());
	
	return moveRelative(in, out, target, minDistance);
}

/**
	Move to a location in modeling.

	Moves to the location of an object in modeling,
	provided the indices of the objects to move to.

	@param in the file that mission writes to with
		commands. Should be connected to interface.
	@param out the file that mission reads from for
		information. Also should be interface.
*/
bool moveModel(FILE* in, FILE* out, int xi, int yi, int zi, float xo, float yo, float zo, float minDistance)
{
	bool close = false;
	while (!close)
	{
		State state = getState(in, out);
		float cx = state.x();
		float cy = state.y();
		float cz = state.depth();

		auto model = getModel(in, out);

		auto target = State(
			model.get(xi) + xo,
			model.get(yi) + yo,
			model.get(zi) + zo
		);

		move(out, state, target);

		if (state.distanceTo(target) < minDistance)
			close = true;
		else if (!alive(in, out)) return false;
		else std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	stop(in, out);

	return true;
}

// x/y/d/t/p/r = X / Y / Depth / Yaw / Pitch / Roll
// a/d/i/r = Absolute Offset / Directional Offset / Index of Model / Current Value Multiplier
bool moveExt(FILE* in, FILE* out, float xa, float xd, int xi, int xr, float ya, float yd, int yi, int yr, float da, int di, int dr, float ta, int ti, int tr, float pa, float ra, float minDistance)
{
	State state = getState(in, out);
	float cx = state.x();
	float cy = state.y();
	float cd = state.depth();
	float ct = state.yaw();

	bool close = false;
	while (!close)
	{
		auto model = getModel(in, out);

		auto dt = ta + model.get(ti) + tr*ct;
		auto dtr = dt * 2*M_PI;

		auto dx = xd * std::cos(dtr) - yd * std::sin(dtr);
		auto dy = xd * std::sin(dtr) + yd * std::cos(dtr);

		auto target = State(
		{
			static_cast<float>(xa + dx + model.get(xi) + xr*cx),
			static_cast<float>(ya + dy + model.get(yi) + yr*cy),
			da + model.get(di) + dr*cd,
			dt,
			pa,
			ra,
		});

		setState(out, target);

		auto state = getState(in, out);

		if (state.distanceTo(target) < minDistance && std::abs(state.yaw() - dt) < .03)
			close = true;
		else if (!alive(in, out)) return false;
		else std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	return true;
}

bool turnTo(FILE* in, FILE* out, int xi, int yi)
{
	auto state = getState(in, out);
	auto model = getModel(in, out);

	auto dx = model.get(xi) - state.x();
	auto dy = model.get(yi) - state.y();

	float theta = std::atan2(dy, dx) / (2 * M_PI);

	return moveExt(in, out, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, theta, 0, 0, 0, 0, .1);
}

bool alignWith(FILE* in, FILE* out, int hi, int vi, int di, int si)
{
	bool close = false;
	while (!close)
	{
		auto state = getState(in, out);
		auto model = getModel(in, out);

		float h = model.get(hi);
		float v = model.get(vi);
		float theta = si >= 0 ? model.get(si) - .25f : state.yaw();

		if (
			std::abs(theta - state.yaw()) < .05 &&
			std::abs(h) < .05 &&
			std::abs(v) < .05 &&
			true
		)
		{
			close = true;
			setState(out, state);
		}
		else moveExt(in, out, 0, 0, di, 1, 0, h * 1, 0, 1, v * 1, 0, 1, theta, 0, 0, 0, 0, .03);
	}

	return true;
}

bool moveModelDir(FILE* in, FILE* out, int xi, int yi, int zi, float xo, float yo, float zo, float minDistance)
{
	auto target = State(xo, yo, zo, 0, 0, 0);
	return (moveModel(in, out, xi, yi, zi, 0, 0, 0, minDistance) 
		&& moveDir(in, out, target, minDistance));
}

bool moveUntil(FILE* in, FILE* out, int yaw, int condition, float min, float max)
{
	bool close = false;
	while (!close)
	{
		auto state = getState(in, out);
		auto model = getModel(in, out);

		auto theta = model.get(yaw);
		auto value = model.get(condition);

		float r = 1.f;

		if (value > min && value < max)
			close = true;
		else
			setState(out, State(state.x() + r * std::cos(theta), state.y() + r * std::sin(theta), state.depth(), theta, 0, 0));
	}

	return true;
}

bool dropInBin(FILE* in, FILE* out)
{
	if (!alive(in, out)) return false;
	drop(out);
	return true;
}

bool uncoverBin(FILE* in, FILE* out)
{
	auto state = getState(in, out);
	auto target = state;

	target.setDepth(constants.get(C_BIN_D));
	if (!moveAbsolute(in, out, target, .1)) return false;

	grab(out);
	wait(in, out, 1);

	target.setX(state.x() + .6);
	if (!moveAbsolute(in, out, target, .1)) return false;

	release(out);
	wait(in, out, 1);

	if (!moveAbsolute(in, out, state, .1)) return false;

	return true;
}

bool shootInHole(FILE* in, FILE* out, char side)
{
	if (!alive(in, out)) return false;
	shoot(out, side);
	return true;
}

bool moveToHole(FILE* in, FILE* out, size_t xi, size_t yi, size_t di, float offset, float mindist)
{
	bool close = false;
	while (!close)
	{
		State state = getState(in, out);
		float cx = state.x();
		float cy = state.y();
		float cz = state.depth();

		auto model = getModel(in, out);

		float tx = model.get(xi);
		float ty = model.get(yi);
		float td = model.get(di);

		float theta = model.get(M_TORP_SKEW);

		auto target = State(
			model.get(xi) - offset*std::cos(theta * 2*M_PI),
			model.get(yi) - offset*std::sin(theta * 2*M_PI),
			model.get(di),
			theta,
			0,
			0
		);

		setState(out, target);

		if (state.distanceTo(target) < mindist)
			close = true;
		else if (!alive(in, out)) return false;
		else std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	stop(in, out);

	return true;
}

bool flag(FILE* in, FILE* out, size_t idx, float value)
{
	if (!alive(in, out)) return false;
	setFlag(out, idx, value);
	return true;
}

bool variance(FILE* in, FILE* out, size_t idx, float value)
{
	if (!alive(in, out)) return false;
	addVariance(out, idx, value);
	return true;
}

bool setMaxThrust(FILE* in, FILE* out, float value)
{
	if (!alive(in, out)) return false;
	maxThrust(out, value);
	return true;
}

bool setSpeed(FILE* in, FILE* out, float value)
{
	if (!alive(in, out)) return false;
	speed(out, value);
	return true;
}

