#include "mission/goal.hpp"

#include "mission/action.hpp"

Goal::Goal(float _value, float _time, int _xi, int _yi, int _di, float _xo, float _yo, float _do, float _mindist, std::vector<Action> _actions) :
	m_value(_value),
	m_time(_time),
	m_xi(_xi),
	m_yi(_yi),
	m_di(_di),
	m_xo(_xo),
	m_yo(_yo),
	m_do(_do),
	m_mindist(_mindist),
	m_actions(_actions)
{
}

Goal::Goal(FILE* in)
{
	int numActions = 0;

	if (fscanf(in, " %f %f %i %i %i %f %f %f %f %i",
		&m_value, &m_time,
		&m_xi, &m_yi, &m_di,
		&m_xo, &m_yo, &m_do,
		&m_mindist,
		&numActions
	) < 9)
		throw 1;

	for (int i = 0; i < numActions; i++)
		m_actions.push_back(Action(in));
}

size_t Goal::write(FILE* out)
{
	size_t numBytes = 0;

	numBytes += fprintf(out, "%f %f %i %i %i %f %f %f %f %zu\n",
		m_value, m_time,
		m_xi, m_yi, m_di,
		m_xo, m_yo, m_do,
		m_mindist,
		m_actions.size()
	);

	 for (auto a : m_actions)
	 	numBytes += a.write(out);

	fflush(out);
	
	return numBytes;
}

bool Goal::run(FILE* in, FILE* out)
{
	for (auto a : m_actions)
		if (!a.run(in, out))
			return false;

	return true;
}

State Goal::location(const Matrix& model) const
{
	return State(
		model.get(m_xi) + m_xo,
		model.get(m_yi) + m_yo,
		model.get(m_di) + m_do
	);
}

float Goal::value() const
{
	return m_value;
}

float Goal::time() const
{
	return m_time;
}

float Goal::mindist() const
{
	return m_mindist;
}

