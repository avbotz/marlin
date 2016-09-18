#include "interface/functions.hpp"

#include <utility>
#include <queue>
#include <thread>
#include <chrono>
#include <cmath>

#include "common/defs.hpp"
#include "common/matrix.hpp"
#include "common/state.hpp"
#include "interface/config.hpp"
#include "image/image.hpp"
#include "model/evidence.hpp"
#include "model/system.hpp"

FILE* openStream(const std::string name, const char* mode)
{
	FILE* f = NULL;
	while (true)
	{
		f = fopen(name.c_str(), mode);
		if (f) return f;
		else std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

Evidence observation(FILE* in, const State& state)
{
	int x, y, d; // indices
	float theta, phi, rho; // rho is absolute depth if d < 0, 0 phi is horizontal, +phi is down
	fscanf(in, " %i %i %i %f %f %f",
		&x,     &y,   &d,
		&theta, &phi, &rho
	);

	// adjust for existing state
	theta += state.yaw();
	float cx = state.x();
	float cy = state.y();
	float cd = state.depth();

	// transform to radians
	theta *= 2 * M_PI;
	phi *= 2 * M_PI;

	Evidence evidence = {};
	if (x >= 0 && x < NUM_VARS && y >= 0 && y < NUM_VARS)
	{
		if (d >= 0 && d < NUM_VARS)
		{
			float hdist = std::abs(rho*std::cos(phi));
			return
			{{
				{x, cx + hdist*static_cast<float>(std::cos(theta)), 9},
				{y, cy + hdist*static_cast<float>(std::sin(theta)), 9},
				{d, cd + rho*static_cast<float>(std::sin(phi)), 1}
			}};
		}
		else if (d == -1) // rho is absolute depth
		{
			float rd = rho - cd;
			float hdist = std::abs(rd / std::tan(phi));
			return
			{{
				{x, cx + hdist*static_cast<float>(std::cos(theta)), .3},
				{y, cy + hdist*static_cast<float>(std::sin(theta)), .3},
			}};
		}
		else if (d == -2) // rho is hdist, phi is ignored
		{
			return
			{{
				{x, cx + rho*static_cast<float>(std::cos(theta)), 1},
				{y, cy + rho*static_cast<float>(std::sin(theta)), 1},
			}};
		}
		else if (d == -3) // straight evidence: theta = value, rho = variance, phi ignored
		{
			return {{{ x, theta/(2*M_PI) - state.yaw(), rho }}};
		}
		else if (d == -4) // straight evidence: theta = value, rho = variance, phi = add yaw
		{
			return {{{ x, theta/(2*M_PI), rho }}};
		}
		else return {};
	} // otherwise we don't know how to use this observation
	else return {};
}

bool hydrophones(Data* data, const std::string in_name)
{
	FILE* in = openStream(in_name, "r");

	bool quit = false;
	while (!quit)
	{
		State state;
		data->lock();
			state = data->state;
		data->unlock();

		Evidence evidence = observation(in, state);

		data->lock();
			data->evidence.push(evidence);
		data->unlock();
	}

	return true;
}

bool vision(Data* data, const std::string in_name, const std::string out_name, size_t vpid, size_t image_index)
{
	FILE* out = openStream(out_name, "w");
	FILE* in = openStream(in_name, "r");

	uint32_t imageID = {};

	bool quit = false;
	while (!quit)
	{
		// check if we should process this frame
		bool running;
		data->lock();
			running = data->flags.at(vpid) > .5;
		data->unlock();

		if (running)
		{
			bool newImage = false;
			cv::Mat img;
			data->lock();
				if (data->imageID.at(image_index) > imageID)
				{
					img = data->image.at(image_index);
					imageID = data->imageID.at(image_index);
					newImage = true;
				}
			data->unlock();

			if (newImage)
			{
				State state;
				data->lock();
					state = data->state;
				data->unlock();

				imageWrite(out, img);

				size_t num_observations = 0;
				fscanf(in, " %zu", &num_observations);

				for (int i = 0; i < num_observations; i++)
				{
					Evidence evidence = observation(in, state);

					data->lock();
						data->evidence.push(evidence);
					data->unlock();
				}
			}
			else std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		else std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

bool camera(Data* data, const std::string in_name, const std::string out_name, size_t image_index)
{
	FILE* out = openStream(out_name, "w");
	FILE* in = openStream(in_name, "r");

	bool quit = false;
	while (!quit)
	{
		fprintf(out, "i\n"); // request image
		fflush(out);
		auto img = imageRead(in); // read image

		// store image
		data->lock();
			data->image.at(image_index) = std::move(img);
			data->imageID.at(image_index)++;
		data->unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

bool mission(Data* data, const std::string in_name, const std::string out_name)
{
	FILE* out = openStream(out_name, "w");
	FILE* in = openStream(in_name, "r");

	bool quit = false;
	while (!quit)
	{
		char m;
		fscanf(in, " %c", &m);
		switch (m)
		{
			case 'q': // query
			{
				char q;
				fscanf(in, " %c", &q);
				switch (q)
				{
					case 's': // state
					{
						data->lock();
							auto state = data->state;
						data->unlock();

						state.write(out);
						fflush(out);
						break;
					}
					case 'i': // image
					{
						char i;
						fscanf(in, " %c", &i);
						switch (i)
						{
							case 'f': // front
							{
								data->lock();
									auto img = data->image.at(I_FRONT);
								data->unlock();

								imageWrite(out, img);
								fflush(out);
								break;
							}
							case 'd': // down
							{
								data->lock();
									auto img = data->image.at(I_DOWN);
								data->unlock();

								imageWrite(out, img);
								fflush(out);
								break;
							}
						}
						break;
					}
					case 'm': // modeling
					{
						char m;
						fscanf(in, " %c", &m);
						switch (m)
						{
							case 'm': // mode
							{
								data->lock();
									auto model = data->model;
								data->unlock();

								model.write(out);
								fflush(out);
								break;
							}
							case 's': // system
							{
								System().write(out); // currently unsupported
								fflush(out);
								break;
							}
							case 'c': // certainty
							{
								fprintf(out, "0\n"); // currently unsupported
								fflush(out);
								break;
							}
						}
						break;
					}
					case 'f': // flag
					{
						size_t idx;
						fscanf(in, " %zu", &idx);
						float value = 0;
						if (idx < NUM_FLAGS)
						{
							data->lock();
								value = data->flags.at(idx);
							data->unlock();
						}
						fprintf(out, "%f\n", value);
						break;
					}
					case 'a': // kill state
					{
						data->lock();
							bool alive = data->alive;
						data->unlock();
						fprintf(out, "%i\n", alive ? 1 : 0);
						fflush(out);
						break;
					}
				}
				break;
			}
			case 'c': // command
			{
				char c;
				fscanf(in, " %c", &c);
				switch(c)
				{
					case 's': // state
					{
						auto state = State(in);
						data->lock();
							data->desiredState = std::move(state);
							data->setState = true;
						data->unlock();
						break;
					}
					case 'd': // dropper
						data->lock();
							data->drop = true;
						data->unlock();
						break;
					case 'g': // grabber
					{
						char g;
						fscanf(in, " %c", &g);
						switch (g)
						{
							case 'g': // grab
								data->lock();
									data->grab = true;
								data->unlock();
								break;
							case 'r': // release
								data->lock();
									data->release = true;
								data->unlock();
								break;
						}
						break;
					}
					case 't': // torpedo
					{
						char t;
						fscanf(in, " %c", &t);
						data->lock();
							data->shoot = t;
						data->unlock();
						break;
					}
					case 'f': // flag
					{
						size_t idx;
						float value;
						fscanf(in, " %zu %f", &idx, &value);
						if (idx < NUM_FLAGS)
						{
							data->lock();
								data->flags.at(idx) = value;
							data->unlock();
						}
						break;
					}
					case 'v': // variance
					{
						size_t idx;
						float value;
						fscanf(in, " %zu %f", &idx, &value);
						if (idx < NUM_VARS)
						{
							data->lock();
								data->addVariance.push(std::make_tuple(idx, value));
							data->unlock();
						}
						break;
					}
					case 'p': // max thrust / power
					{
						float value;
						fscanf(in, " %f", &value);
						Line l;
						snprintf(l.str, sizeof(l.str), "p %f", value);
						data->lock();
							data->send_line.push(l);
						data->unlock();
						break;
					}
					case 'o': // max thrust / power
					{
						float value;
						fscanf(in, " %f", &value);
						Line l;
						snprintf(l.str, sizeof(l.str), "o %f", value);
						data->lock();
							data->send_line.push(l);
						data->unlock();
						break;
					}
				}
				break;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

bool modeling(Data* data, const std::string in_name, const std::string out_name)
{
	FILE* out = openStream(out_name, "w");
	FILE* in = openStream(in_name, "r");

	bool alive_pre = false;

	bool quit = false;
	while (!quit)
	{
		bool alive;
		data->lock();
			alive = data->alive;
		data->unlock();
		if (alive && !alive_pre)
			// reset model
			fprintf(out, "r\n");
		alive_pre = alive;

		fprintf(out, "m\n"); // request model
		fflush(out);
		auto model = Matrix(in); // read model

		std::queue<Evidence> evidence;
		std::queue<std::tuple<size_t, float> > variance;
		// store model and get new evidences
		data->lock();
			data->model = std::move(model);
			data->modelID++;
			std::swap(evidence, data->evidence);
			std::swap(variance, data->addVariance);
		data->unlock();

		// update variance
		while (!variance.empty())
		{
			auto v = variance.front();
			variance.pop();
			fprintf(out, "v %zu %f\n", std::get<0>(v), std::get<1>(v));
		}
		fflush(out);


		// send new evidence
		while (!evidence.empty())
		{
			fprintf(out, "e\n");
			evidence.front().write(out);
			evidence.pop();
		}
		fflush(out);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

bool control(Data* data, const std::string in_name, const std::string out_name)
{
	FILE* out = openStream(out_name, "w");
	FILE* in = openStream(in_name, "r");

	bool quit = false;
	while (!quit)
	{
		fprintf(out, "c\n"); // request state
		fflush(out);
		auto state = State(in); // read state

		fprintf(out, "a\n"); // request kill state
		fflush(out);
		int kill_state;
		fscanf(in, " %i", &kill_state);
		data->lock();
			data->alive = (kill_state != 0);
		data->unlock();

		// compute movement
		State oldState;
		data->lock();
			oldState = data->state;
		data->unlock();
		float dist = state.distanceTo(oldState);
		if (dist > .01)
		{
			data->lock();
				data->addVariance.push(std::make_tuple(static_cast<size_t>(NUM_VARS), dist));
			data->unlock();
		}

		State desiredState;
		bool setState, drop, grab, release;
		char shoot;
		// store state and get new commands
		data->lock();
			data->state = std::move(state);
			data->stateID++;

			std::swap(desiredState, data->desiredState);
			setState = data->setState;
			data->setState = false;
			shoot = data->shoot;
			data->shoot = false;
			drop = data->drop;
			data->drop = false;
			grab = data->grab;
			data->grab = false;
			release = data->release;
			data->release = false;
		data->unlock();

		// send commands to arduino
		if (setState)
		{
			fprintf(out, "s ");
			desiredState.write(out);
			fflush(out);
		}

		if (shoot == 'r') fprintf(out, "r 1\n");
		if (shoot == 'l') fprintf(out, "r 2\n");

		if (drop) fprintf(out, "r 3\n");
		if (grab) fprintf(out, "g\n");
		if (release) fprintf(out, "r\n");

		std::queue<Line> lines;
		data->lock();
			std::swap(lines, data->send_line);
		data->unlock();

		while (!lines.empty())
		{
			fprintf(out, "%s\n", lines.front().str);
			lines.pop();
		}

		fflush(out);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

