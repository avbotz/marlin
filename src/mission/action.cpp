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

bool wait(FILE* in, FILE* out, float time)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(time * 1000)));
	return true;
}

bool moveAbsolute(FILE* in, FILE* out, const State& target, float minDistance)
{
	setState(out, target);

	bool close = false;
	while (!close)
	{
		State state = getState(in, out);

		if (state.distanceTo(target) < minDistance)
			close = true;
		else if (!alive(in, out)) return false;
		else std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	return true;
}

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

bool moveModelDir(FILE* in, FILE* out, int xi, int yi, int zi, float xo, float yo, float zo, float minDistance)
{
	auto target = State(xo, yo, zo, 0, 0, 0);
	return (moveModel(in, out, xi, yi, zi, 0, 0, 0, minDistance) 
		&& moveDir(in, out, target, minDistance));
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

		float theta = std::atan2(model.get(M_TORP_L_Y) - model.get(M_TORP_R_Y), model.get(M_TORP_L_X) - model.get(M_TORP_R_X)) + M_PI/2;

		auto target = State(
			model.get(xi) - offset*std::cos(theta),
			model.get(yi) - offset*std::sin(theta),
			model.get(di),
			theta / (2*M_PI),
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

