#include "mission/mission.hpp"

#include <vector>
#include <algorithm>

#include "mission/command.hpp"
#include "mission/query.hpp"
#include "mission/goal.hpp"
#include "mission/config.hpp"

int mission(FILE* in, FILE* out, FILE* config)
{
	std::vector<Goal> goals;

	if (config == NULL) goals = initialGoals;
	else
	{
		int numGoals = 0;
		fscanf(config, " %i", &numGoals);
		if (numGoals < 1) return 0;
		for (size_t i = 0; i < numGoals; i++)
			goals.push_back(Goal(config));
	}

	bool quit = false;
	while (!quit)
	{
		// get current state
		auto state = getState(in, out);
		auto model = getModel(in, out);

		// select goal
		float speed = 1;
		std::sort(goals.begin(), goals.end(), [&](const Goal a, const Goal b)
		{
			auto value = [&](const Goal t)
			{
				return t.value() / (t.time() + state.distanceTo(t.location(model)) / speed);
			};
			return value(a) < value(b); // descending order
		});
		auto goal = goals.back();

		// if close enough, begin goal
		auto goalLoc = goal.location(model);
		if (state.distanceTo(goalLoc) < goal.mindist())
		{
			if (goal.run(in, out)) goals.pop_back();
			// do something here if it fails
		}
		// otherwise approach goal (but continue loop so we can switch later if we want)
		else move(out, state, goalLoc);

		if (goals.empty()) quit = true;
	}

	return 0;
}

